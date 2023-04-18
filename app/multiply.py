import argparse
import time

def multiply(number):
    array_2d = [[0 for j in range(4)] for i in range(3)]
    start_time = time.time()
    # A loop that runs for 1-2 seconds
    while time.time() - start_time < 2:
        pass

    return number * 5


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--number', required=True, type=int,
                        help='The number to be multiplied')
    args = parser.parse_args()
    result = multiply(args.number)
    print(result)
