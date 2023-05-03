import argparse
import csv
import os

def multiply(number):
    return number * 5


def save_to_csv(result):
    output_dir = "/app/output"
    filename = os.path.join(output_dir, 'result.csv')
    mode = 'x' if not os.path.exists(filename) else 'w'

    with open(filename, mode, newline='') as csvfile:
        writer = csv.writer(csvfile)
        if mode == 'x':
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