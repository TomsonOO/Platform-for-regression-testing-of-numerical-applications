import subprocess
import sys
import importlib
import csv
import time
import os

def build_docker(image_name, context_path, dockerfile):
    cmd = f"docker build -t {image_name} -f {dockerfile} {context_path}"
    subprocess.run(cmd, shell=True, check=True)

def run_docker(image_name, container_name, entrypoint, number):
    cmd = f"docker run --rm --name {container_name} {image_name} {entrypoint} {number}"
    start_time = time.time()
    result = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
    end_time = time.time()
    execution_time = end_time - start_time
    return result, execution_time


def save_to_csv(csv_filename, argument, run_number, result, execution_time):
    fieldnames = ['run_number', 'argument', 'result', 'execution_time']

    with open(os.path.join('results', csv_filename), mode='a', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        if csvfile.tell() == 0:  # Check if the file is empty
            writer.writeheader()

        writer.writerow({'run_number': run_number, 'argument': argument, 'result': result, 'execution_time': execution_time})



if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_file>")
        sys.exit(1)

    config_file = sys.argv[1].replace('.py', '')
    config = importlib.import_module(config_file)

    build_docker(config.image_name, config.context_path, config.dockerfile)

    result, execution_time = run_docker(config.image_name, config.container_name, config.entrypoint, config.number)
    print(f"Result: {result}")
    print(f"Execution time: {execution_time:.4f} seconds")

    csv_filename = f"{config.app_name}.csv"

    # If the CSV file already exists, read the last run number and increment it by 1
    if os.path.exists(os.path.join('results', csv_filename)):
        with open(os.path.join('results', csv_filename), mode='r', newline='') as csvfile:
            last_run_number = sum(1 for row in csvfile) - 1  # Subtract 1 to exclude the header
    else:
        last_run_number = 0

    run_number = last_run_number + 1
    save_to_csv(csv_filename, run_number, config.number, result, execution_time)