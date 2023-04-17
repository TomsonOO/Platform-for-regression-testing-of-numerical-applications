import os
import subprocess
import csv

def build_docker(image_name, context_path):
    os.system(f"docker build -t {image_name} {context_path}")


def run_docker(image_name, container_name, number):
    result = subprocess.check_output(
        f"docker run --rm --name {container_name} {image_name} python /usr/src/app/multiply.py --number {number}",
        shell=True, text=True
    )
    return int(result.strip())


def save_result_to_csv(number, result, filename="results/result.csv"):
    with open(filename, "w", newline='') as result_file:
        csv_writer = csv.writer(result_file)
        csv_writer.writerow(['Number', 'Result'])
        csv_writer.writerow([number, result])


if __name__ == "__main__":
    image_name = "docki-multiply"
    container_name = "docki-multiply-container"
    number = 6
    context_path = "app"

    # Create the results directory if it doesn't exist
    os.makedirs("results", exist_ok=True)

    build_docker(image_name, context_path)
    result = run_docker(image_name, container_name, number)
    save_result_to_csv(number, result)
