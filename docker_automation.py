import subprocess

def build_docker(image_name, context_path, dockerfile):
    cmd = f"docker build -t {image_name} -f {dockerfile} {context_path}"
    subprocess.run(cmd, shell=True, check=True)

def run_docker(image_name, container_name, number):
    cmd = f"docker run --rm --name {container_name} {image_name} /usr/src/app/multiply {number}"
    result = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
    return int(result)


image_name = "docki-multiply-cpp"
container_name = "docki-multiply-cpp-container"
context_path = "."
dockerfile = "c++/Dockerfile_cpp"

build_docker(image_name, context_path, dockerfile)

number = 5
result = run_docker(image_name, container_name, number)
print(f"Result: {result}")
