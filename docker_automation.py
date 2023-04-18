import subprocess
import sys
import importlib
import csv

def build_docker(image_name, context_path, dockerfile):
    cmd = f"docker build -t {image_name} -f {dockerfile} {context_path}"
    subprocess.run(cmd, shell=True, check=True)

def run_docker(image_name, container_name, entrypoint, number):
    cmd = f"docker run --rm --name {container_name} {image_name} {entrypoint} {number}"
    result = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
    return int(result)

def save_result_to_csv(number, result, filename="results/result.csv"):
    with open(filename, "w", newline='') as result_file:
        csv_writer = csv.writer(result_file)
        csv_writer.writerow(['Number', 'Result'])
        csv_writer.writerow([number, result])

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_file>")
        sys.exit(1)

    config_file = sys.argv[1].replace('.py', '')
    config = importlib.import_module(config_file)

    build_docker(config.image_name, config.context_path, config.dockerfile)

    result = run_docker(config.image_name, config.container_name, config.entrypoint, config.number)
    print(f"Result: {result}")
    save_result_to_csv(config.number, result)
