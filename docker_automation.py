import subprocess
from check_consistency import check_consistency
import sys
import json
import sqlite3
import time
import requests
import pandas as pd
import os


def process_results(results, app_name, config):
    output_file = config["app_output_file"]
    output_columns = config["output_columns"]

    data = [row.split(',') for row in results.split('\n') if row]
    df = pd.DataFrame(data, columns=output_columns)
    df.to_csv(output_file, index=False)
    print(f"Results saved to {output_file}")

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


def initialize_database(database_name):
    conn = sqlite3.connect(database_name)
    cur = conn.cursor()

    cur.execute('''
        CREATE TABLE IF NOT EXISTS results (
            run_number INTEGER PRIMARY KEY,
            result TEXT,
            arguments TEXT
        )
    ''')

    cur.execute('''
        CREATE TABLE IF NOT EXISTS execution_times (
            run_number INTEGER PRIMARY KEY,
            execution_time REAL
        )
    ''')

    conn.commit()
    conn.close()


def save_to_database(database_name, run_number, result, execution_time, arguments):
    conn = sqlite3.connect(database_name)
    cur = conn.cursor()

    cur.execute('INSERT INTO results (run_number, result, arguments) VALUES (?, ?, ?)',
                (run_number, result, json.dumps(arguments)))
    cur.execute('INSERT INTO execution_times (run_number, execution_time) VALUES (?, ?)', (run_number, execution_time))

    conn.commit()
    conn.close()


def get_data_as_json(database_name):
    conn = sqlite3.connect(database_name)
    cur = conn.cursor()

    results = cur.execute('SELECT * FROM results').fetchall()
    execution_times = cur.execute('SELECT * FROM execution_times').fetchall()

    conn.close()

    data = {
        "results": [dict(zip(["run_number", "result", "arguments"], row)) for row in results],
        "execution_times": [dict(zip(["run_number", "execution_time"], row)) for row in execution_times],
    }

    return json.dumps(data)


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
    initialize_database(database_name)

    # Calculate the run number
    last_run_number = max([row[0] for row in sqlite3.connect(database_name).cursor().execute('SELECT run_number FROM '
                                                                                             'results')]) if \
        sqlite3.connect(database_name).cursor().execute('SELECT   run_number FROM results').fetchall() else 0
    run_number = last_run_number + 1

    # Save the result and execution time to the database
    save_to_database(database_name, run_number, result, execution_time, config["arguments"])

    # Test the consistency of the result
    is_consistent = check_consistency(database_name, 'results', 'result')
    print(f"Result consistency: {is_consistent}")

    # Get the data as JSON
    data_json = get_data_as_json(database_name)

    # Send the JSON data to the Flask app
    flask_url = 'http://127.0.0.1:5000/'  # Update the URL if the Flask app is running on a different address or port
    response = requests.post(flask_url, data={'data': data_json})

    if response.status_code == 200:
        print("Data sent to the Flask app successfully.")
    else:
        print(f"Failed to send data to the Flask app. Response status code: {response.status_code}")
