import subprocess
from check_consistency import check_consistency
import sys
import json
import sqlite3
import time
import requests
import pandas as pd
import os
from database_manager import DatabaseManager


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
    cmd = f"docker build -t {image_name} -f {dockerfile} {context_path}"
    subprocess.run(cmd, shell=True, check=True)


def run_docker(image_name, container_name, entrypoint, *args):
    host_output_path = os.path.abspath('.')
    container_output_path = "/app/output"

    cmd = f"docker run --rm --name {container_name} -v {host_output_path}:{container_output_path} {image_name} {entrypoint} {' '.join(map(str, args))}"
    start_time = time.time()
    result = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
    end_time = time.time()
    execution_time = end_time - start_time
    return result, execution_time


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
    result, execution_time = run_docker(config["image_name"], config["container_name"], formatted_entrypoint,
                                        output_path)

    print(f"Result: {result}")
    print(f"Execution time: {execution_time:.4f} seconds")

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
    db_manager.insert_result(config_name, run_number, result, execution_time, config["arguments"])
    db_manager.update_data_json_file(config_name)

    # Test the consistency of the result
    # is_consistent = check_consistency(database_name, 'results', 'result')
    # print(f"Result consistency: {is_consistent}")

    db_manager.update_data_json_file(config_name)

    # Get the data as JSON
    data_json = db_manager.get_data_as_json(config_name)

    # Send the JSON data to the Flask app
    flask_url = 'http://127.0.0.1:5000/'  # Update the URL if the Flask app is running on a different address or port
    response = requests.get(flask_url, json={'data': data_json})

    if response.status_code == 200:
        print("Data sent to the Flask app successfully.")
    else:
        print(f"Failed to send data to the Flask app. Response status code: {response.status_code}")
