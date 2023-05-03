import subprocess
import csv

def get_result_from_csv():
    with open('results/result.csv', newline='') as csvfile:
        reader = csv.reader(csvfile)
        next(reader)  # skip header
        results = [row[1] for row in reader]

    return results

def test_result_consistency():
    # Run docker_automation.py 5 times
    for _ in range(5):
        subprocess.run(["python", "docker_automation.py", "multiply_py.json"])

    results = get_result_from_csv()

    # Check if all results are the same
    assert len(set(results)) == 1, f"Results are not the same: {results}"

if __name__ == '__main__':
    test_result_consistency()
