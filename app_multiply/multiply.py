import argparse
import csv
import os

def multiply(number):
    return number * 5

def complex_multiply(number, iterations=10**7):
    result = number
    for _ in range(iterations):
        result = (result * 5) % (10**9 + 7)  # Modulo to prevent overflow
        for _ in range(10):  # Nested loop to add more complexity
            result = (result + 1) % (10**9 + 7)
            result = (result - 1) % (10**9 + 7)
    return result


def save_to_csv(result):
    output_dir = "/app/output"
    os.makedirs(output_dir, exist_ok=True)  # Create the output directory if it doesn't exist
    filename = os.path.join(output_dir, 'result.csv')
    mode = 'w'  # Always open in write mode

    with open(filename, mode, newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Result'])
        writer.writerow([result])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--number', required=True, type=int,
                        help='The number to be multiplied')
    args = parser.parse_args()
    result = multiply(args.number)
    print(result)

    save_to_csv(result)