#include <iostream>
#include <cstdlib>

int multiply(int number) {
    return number * 10;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;
        return 1;
    }

    int number = std::atoi(argv[1]);
    int result = multiply(number);
    std::cout << result << std::endl;

    return 0;
}
