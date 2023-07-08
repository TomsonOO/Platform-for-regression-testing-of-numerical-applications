import sys
import json
import sqlite3
import os
from database_manager import DatabaseManager
import docker
import time

client = docker.from_env()


def build_docker(image_name, context_path, dockerfile):
    client = docker.from_env()

    print("Building Docker image...")

    # Create full Dockerfile path
    dockerfile_path = os.path.join(dockerfile)

    # If the Dockerfile path is not valid, raise an error
    if not os.path.isfile(dockerfile_path):
        raise ValueError(f"Dockerfile does not exist: {dockerfile_path}")

    # Ensure the Dockerfile path is relative to the context path
    dockerfile_relative_path = os.path.relpath(dockerfile_path, context_path)

    try:
        image, build_logs = client.images.build(path=context_path, tag=image_name, dockerfile=dockerfile_relative_path,
                                                rm=True, forcerm=True)
        print("Docker image built.")
    except docker.errors.BuildError as e:
        print('Image build failed.')
        for log in e.build_log:
            print(log)
        sys.exit(1)


def calculate_cpu_percent(d, previous_cpu, previous_system):
    cpu_total = float(d["cpu_stats"]["cpu_usage"]["total_usage"])
    cpu_delta = cpu_total - previous_cpu

    cpu_system = float(d["cpu_stats"].get("system_cpu_usage", 0))
    # cpu_system = float(d["cpu_stats"]["system_cpu_usage"])
    if cpu_system == 0:
        return 0.0, cpu_total, cpu_system

    system_delta = cpu_system - previous_system
    # online_cpus = d["cpu_stats"].get("online_cpus", len(d["cpu_stats"]["cpu_usage"].get("percpu_usage", [None])))
    online_cpus = d["cpu_stats"].get("online_cpus", len(d["cpu_stats"]["cpu_usage"].get("percpu_usage", [None])))

    # cpu_percent = (cpu_delta / system_delta) * online_cpus * 100.0 if system_delta > 0.0 else 0.0

    if system_delta > 0.0:
        cpu_percent = (cpu_delta / system_delta) * online_cpus * 100.0

    return cpu_percent, cpu_total, cpu_system


def run_docker(db_manager, run_number, image_name, container_name, command):
    client = docker.from_env()
    previous_cpu = previous_system = 0.0

    try:
        container = client.containers.get(container_name)
        container.stop()
        container.remove()
        print(f"Stopped and removed container {container_name}")
    except docker.errors.NotFound:
        print(f"Container {container_name} not found. It may not have been created or may have already been removed.")


    # Define the volume
    volume_path_host = os.getcwd()  # Current directory
    volume_path_container = "/data"  # Directory in the container
    volumes = {volume_path_host: {'bind': volume_path_container, 'mode': 'rw'}}

    container = client.containers.create(image_name, name=container_name, command=command, detach=True)
    container.start()

    total_cpu_usage = total_memory_usage = samples_count = 0
    start_time = time.time()

    for stat in container.stats(stream=True):
        stat = json.loads(stat)

        cpu_percent, previous_cpu, previous_system = calculate_cpu_percent(stat, previous_cpu, previous_system)

        if 'usage' in stat['memory_stats'] and 'limit' in stat['memory_stats']:
            memory_usage = stat['memory_stats']['usage'] / stat['memory_stats']['limit']
        else:
            memory_usage = 0

        print("\nCPU usage: ", cpu_percent)
        print("Memory usage: ", memory_usage * 100)
        total_cpu_usage += cpu_percent
        total_memory_usage += memory_usage
        if cpu_percent != 0.0:
            samples_count += 1  # ??

        db_manager.insert_usage(config["app_name"], run_number, cpu_percent, memory_usage * 100)

        if stat['read'] == stat['preread']:  # 'read' and 'preread' are the same when the container stops
            break

    if samples_count > 0:
        average_cpu_usage = (total_cpu_usage / samples_count)
        average_memory_usage = (total_memory_usage / samples_count) * 100
    else:
        average_cpu_usage = 0
        average_memory_usage = 0

    logs = get_container_logs(container)
    result = container.logs()
    # print("eeeeeeee", result)
    container.remove()

    end_time = time.time()
    execution_time = end_time - start_time

    return result.decode(), execution_time, average_cpu_usage, average_memory_usage


def get_container_logs(container):
    return container.logs()


def load_config(config_file):
    with open(config_file) as f:
        config = json.load(f)
    return config


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_file>")
        sys.exit(1)

    # Load config file
    config_file = sys.argv[1]
    with open(config_file) as f:
        config = json.load(f)

    ### Database

    # Initialize the database
    database_name = f'{config["app_name"]}.db'
    config_name = config["app_name"]
    db_manager = DatabaseManager(database_name, config_name)
    db_manager.create_usage_table(config["app_name"])

    # Calculate the run number
    database_path = os.path.join("databases/", database_name)
    last_run_number = max([row[0] for row in sqlite3.connect(database_path).cursor().execute(
        f'SELECT run_number FROM {config_name}_results')]) if \
        sqlite3.connect(database_path).cursor().execute(
            f'SELECT run_number FROM {config_name}_results').fetchall() else 0
    run_number = last_run_number + 1

    build_docker(config["image_name"], config["context_path"], config["dockerfile"])
    output_path = config["output_path"]
    formatted_entrypoint = config["entrypoint"].format(*config["arguments"])
    result, execution_time, cpu_percent, memory_percent = run_docker(db_manager, run_number, config["image_name"], config["container_name"], formatted_entrypoint)


    print(f"Result: {result}")
    print(f"Execution time: {execution_time:.4f} seconds")
    print(f"Average CPU usage: {cpu_percent:.2f}%")
    print(f"Average memory usage: {memory_percent:.2f}%")

    # Create a table for the config_name if it doesn't exist
    # db_manager.create_table_for_config(config_name)

    # run_number = 0
    # Save the result and execution time to the database
    db_manager.insert_result(config_name, run_number, result, execution_time, config["arguments"], cpu_percent,
                             memory_percent)
    db_manager.update_data_json_file(config_name)
    db_manager.perform_regression_test(config_name, result, config["arguments"])
