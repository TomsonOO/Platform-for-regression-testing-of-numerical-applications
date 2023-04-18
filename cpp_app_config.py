app_name = "cpp_multiply"
image_name = "docki-multiply-cpp"
container_name = "docki-multiply-cpp-container"
context_path = "."
dockerfile = "c++/Dockerfile_cpp"
entrypoint = "/usr/src/app/multiply {}"
arguments = [4]