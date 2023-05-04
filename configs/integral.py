app_name = "python_integral"
image_name = "docki-integral-python"
container_name = "docki-integral-python-container"
context_path = "../app_integral"
dockerfile = "app_integral/Dockerfile"
entrypoint = "python /usr/src/app/integral.py --a {} --b {} --n {}"
a = 0  # Lower limit of integration
b = 1  # Upper limit of integration
n = 100  # Number of trapezoids

arguments = [a, b, n]
