app_name = "python_multiply"
image_name = "docki-multiply-python"
container_name = "docki-multiply-python-container"
context_path = "../app_multiply"
dockerfile = "app_multiply/Dockerfile"
entrypoint = "python /usr/src/app/multiply.py --number {}"
arguments = [3]
