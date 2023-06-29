import argparse
import csv
import os
import multiprocessing


def multiply(number):
    return number * 5

def memory_consuming_function(iterations=10**7):
    data = []
    for i in range(iterations):
        data.append(str(i) * 1000)  # Each string takes around 1KB of memory
    return sum(len(x) for x in data)  # Return total memory used in KB



def complex_multiply(number, iterations=10 ** 7):
    result = number
    for _ in range(iterations):
        result = (result * 5) % (10 ** 9 + 7)  # Modulo to prevent overflow
        for _ in range(10):  # Nested loop to add more complexity
            result = (result + 1) % (10 ** 9 + 7)
            result = (result - 1) % (10 ** 9 + 7)
    return result


def worker(number, iterations=10 ** 7):
    result = number
    for _ in range(iterations):
        result = (result * 5) % (10 ** 9 + 7)  # Modulo to prevent overflow
        for _ in range(10):  # Nested loop to add more complexity
            result = (result + 1) % (10 ** 9 + 7)
            result = (result - 1) % (10 ** 9 + 7)
    return result


def power_consuming_function_multiprocessing(number, iterations=10 ** 7):
    pool = multiprocessing.Pool(processes=multiprocessing.cpu_count())
    results = [pool.apply_async(worker, (number, iterations)) for _ in range(multiprocessing.cpu_count())]
    return [result.get() for result in results]


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
    # result = multiply(args.number)
    # result = complex_multiply(args.number)
    result = power_consuming_function_multiprocessing(args.number)
    # result = memory_consuming_function()
    # print(result)

    save_to_csv(result)
