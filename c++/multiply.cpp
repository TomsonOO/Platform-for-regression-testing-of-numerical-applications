#include <iostream>
#include <cstdlib>
#include <fstream>

using namespace std;

int multiply(int number) {
    return number * 5;
}

void save_to_csv(int result) {
    string filename = "result.csv";
    string header = "Result";
    bool file_exists = ifstream(filename).good();

    ofstream csvfile(filename, ios::app);
    if (!file_exists) {
        csvfile << header << endl;
    }
    csvfile << result << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <number>" << endl;
        return 1;
    }

    int number = atoi(argv[1]);
    int result = multiply(number);
    cout << result << endl;

    save_to_csv(result);

    return 0;
}
