import subprocess
import sys
import json
import sqlite3
import time
import requests
import pandas as pd
import os
from database_manager import DatabaseManager
import psutil
import docker
import time

client = docker.from_env()


def process_results(results, app_name, config):
    output_file = config["app_output_file"]
    output_columns = config["output_columns"]

    data = [row.split(',') for row in results.split('\n') if row]
    df = pd.DataFrame(data, columns=output_columns)
    df.to_csv(output_file, index=False)
    print(f"Results saved to {output_file}")
    try:
        requests.get('http://localhost:5000/trigger_update')
    except requests.exceptions.RequestException:
        pass

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


def run_docker(image_name, container_name, command):
    client = docker.from_env()
    previous_cpu = previous_system = 0.0

    try:
        container = client.containers.get(container_name)
        container.stop()
        container.remove()
        print(f"Stopped and removed container {container_name}")
    except docker.errors.NotFound:
        print(f"Container {container_name} not found. It may not have been created or may have already been removed.")

    container = client.containers.create(image_name, name=container_name, command=command, detach=True)
    container.start()

    total_cpu_usage = total_memory_usage = samples_count = 0
    start_time = time.time()

    for stat in container.stats(stream=True):
        stat = json.loads(stat)

        cpu_percent, previous_cpu, previous_system = calculate_cpu_percent(stat, previous_cpu, previous_system)

        memory_usage = stat['memory_stats']['usage'] / stat['memory_stats']['limit'] if 'usage' in stat[
            'memory_stats'] and 'limit' in stat['memory_stats'] else 0

        print("eee", cpu_percent)
        total_cpu_usage += cpu_percent
        total_memory_usage += memory_usage
        samples_count += 1

        if stat['read'] == stat['preread']:  # 'read' and 'preread' are the same when the container stops
            break

    if samples_count > 0:
        average_cpu_usage = (total_cpu_usage / samples_count)
    else:
        average_cpu_usage = 0  # Percentage

    average_memory_usage = (total_memory_usage / samples_count) * 100 if samples_count > 0 else 5  # Percentage

    logs = get_container_logs(container)
    result = container.logs()
    # print("eeeeeeee", result)
    container.remove()

    end_time = time.time()
    execution_time = end_time - start_time

    return result.decode(), execution_time, average_cpu_usage, average_memory_usage


def stop_and_remove_container(container_name):
    client = docker.from_env()

    try:
        container = client.containers.get(container_name)
        container.stop()
        container.remove()
        print(f"Stopped and removed container {container_name}")
    except docker.errors.NotFound:
        print(f"Container {container_name} not found. It may not have been created or may have already been removed.")
    except docker.errors.APIError as e:
        print(f"An unexpected Docker API error occurred: {e}")
        sys.exit(1)

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

    build_docker(config["image_name"], config["context_path"], config["dockerfile"])
    output_path = config["output_path"]
    formatted_entrypoint = config["entrypoint"].format(*config["arguments"])
    result, execution_time, cpu_percent, memory_percent = run_docker(config["image_name"], config["container_name"], formatted_entrypoint)

    print(f"Result: {result}")
    print(f"Execution time: {execution_time:.4f} seconds")
    print(f"Average CPU usage: {cpu_percent:.2f}%")
    print(f"Average memory usage: {memory_percent:.2f}%")

    # Save the results to the specified format
    if config["output_format"] == "csv":
        process_results(result, config["app_name"], config)

    # Initialize the database
    database_name = 'results.db'
    db_manager = DatabaseManager(database_name)
    config_name = config["app_name"]

    # Create a table for the config_name if it doesn't exist
    db_manager.create_table_for_config(config_name)

    # Calculate the run number
    last_run_number = max([row[0] for row in sqlite3.connect(database_name).cursor().execute(
        f'SELECT run_number FROM {config_name}_results')]) if \
        sqlite3.connect(database_name).cursor().execute(
            f'SELECT run_number FROM {config_name}_results').fetchall() else 0
    run_number = last_run_number + 1

    # Save the result and execution time to the database
    db_manager.insert_result(config_name, run_number, result, execution_time, config["arguments"], cpu_percent, memory_percent)
    db_manager.update_data_json_file(config_name)
    db_manager.perform_regression_test(config_name, result, config["arguments"])

    # Get the data as JSON
    data_json = db_manager.get_data_as_json(config_name)

    # Send the JSON data to the Flask app
    flask_url = 'http://127.0.0.1:5000/'  # Update the URL if the Flask app is running on a different address or port
    response = requests.get(flask_url, json={'data': data_json})

    if response.status_code == 200:
        print("Data sent to the Flask app successfully.")
    else:
        print(f"Failed to send data to the Flask app. Response status code: {response.status_code}")
