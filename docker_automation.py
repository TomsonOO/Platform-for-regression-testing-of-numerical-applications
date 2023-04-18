import subprocess
import sys
import importlib
import csv
import time
import os

def build_docker(image_name, context_path, dockerfile):
    cmd = f"docker build -t {image_name} -f {dockerfile} {context_path}"
    subprocess.run(cmd, shell=True, check=True)

def run_docker(image_name, container_name, entrypoint, *args):
    cmd = f"docker run --rm --name {container_name} {image_name} {entrypoint} {' '.join(map(str, args))}"
    start_time = time.time()
    result = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
    end_time = time.time()
    execution_time = end_time - start_time
    return result, execution_time


def save_to_csv(csv_filename, run_number, result, execution_time, *args):
    fieldnames = ['run_number', 'result', 'execution_time']
    fieldnames.extend([f'arg_{i}' for i in range(1, len(args) + 1)])

    with open(os.path.join('results', csv_filename), mode='a', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        if csvfile.tell() == 0:  # Check if the file is empty
            writer.writeheader()

        row_data = {'run_number': run_number, 'result': result, 'execution_time': execution_time}
        row_data.update({f'arg_{i}': arg for i, arg in enumerate(args, start=1)})

        writer.writerow(row_data)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_file>")
        sys.exit(1)

    config_file = sys.argv[1].replace('.py', '')
    config = importlib.import_module(config_file)

    build_docker(config.image_name, config.context_path, config.dockerfile)

    formatted_entrypoint = config.entrypoint.format(*config.arguments)
    result, execution_time = run_docker(config.image_name, config.container_name, formatted_entrypoint)

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
    save_to_csv(csv_filename, run_number, result, execution_time, *config.arguments)
