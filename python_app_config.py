app_name = "python_multiply"
image_name = "docki-multiply-python"
container_name = "docki-multiply-python-container"
context_path = "app"
dockerfile = "app/Dockerfile"
entrypoint = "python /usr/src/app/multiply.py --number"
number = 10
