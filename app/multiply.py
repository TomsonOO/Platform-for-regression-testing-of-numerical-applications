import argparse

def multiply(number):
    return number * 10

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--number', required=True, type=int,
                        help='The number to be multiplied')
    args = parser.parse_args()
    result = multiply(args.number)
    print(result)
