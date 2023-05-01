import csv
import json

def csv_to_json(csv_filename):
    data = []
    with open(csv_filename, newline='') as csvfile:
        reader = csv.reader(csvfile)
        header = next(reader)

        for row in reader:
            data.append({header[i]: row[i] for i in range(len(header))})

    return data

if __name__ == '__main__':
    result_data = csv_to_json('results/results.csv')
    execution_time_data = csv_to_json('results/execution_times.csv')

    json_data = {
        'result': result_data,
        'execution_time': execution_time_data
    }

    with open('results/data.json', 'w') as f:
        json.dump(json_data, f, indent=4)