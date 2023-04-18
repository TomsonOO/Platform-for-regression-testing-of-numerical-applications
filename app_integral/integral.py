import argparse

def f(x):
    return x ** 2  # Define the function to integrate here.

def trapezoidal_rule(a, b, n):
    h = (b - a) / n
    integral = 0.5 * (f(a) + f(b))

    for i in range(1, n):
        integral += f(a + i * h)

    integral *= h
    return integral

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--a', required=True, type=float, help='Lower limit of integration')
    parser.add_argument('--b', required=True, type=float, help='Upper limit of integration')
    parser.add_argument('--n', required=True, type=int, help='Number of trapezoids')
    args = parser.parse_args()

    result = trapezoidal_rule(args.a, args.b, args.n)
    print(result)
