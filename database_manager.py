import sqlite3
import json
from datetime import datetime
import pandas as pd
import os

class DatabaseManager:
    def __init__(self, database_name, config_name):
        # specify directory
        self.directory = 'databases/'
        # self.conn = sqlite3.connect(db_file)
        # self.cursor = self.conn.cursor()

        # make sure directory exists
        if not os.path.exists(self.directory):
            os.makedirs(self.directory)

        # full path to database
        self.database_name = os.path.join(self.directory, database_name)

        if not os.path.isfile(self.database_name):
            self.create_table(config_name)

    def create_table(self, config_name):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        cur.execute(f'''
            CREATE TABLE IF NOT EXISTS {table_name} (
                run_number INTEGER,
                result TEXT,
                execution_time REAL,
                arguments TEXT,
                test TEXT DEFAULT "not tested",
                run_date TEXT,
                cpu_percent REAL,
                memory_percent REAL
            )
        ''')

        conn.commit()
        conn.close()

    def insert_result(self, config_name, run_number, result, execution_time, arguments, cpu_percent, memory_percent):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        run_date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        cur.execute(
            f"INSERT INTO results (run_number, result, execution_time, arguments, test, run_date, cpu_percent, memory_percent, config_name) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (run_number, result, execution_time, json.dumps(arguments), "not tested", run_date, cpu_percent, memory_percent, config_name))

        conn.commit()
        conn.close()

    def switch_database(self, db_file):
        self.conn.close()  # Close the current connection
        self.conn = sqlite3.connect(db_file)
        self.cursor = self.conn.cursor()

    def insert_result(self, config_name, run_number, result, execution_time, arguments, cpu_percent, memory_percent):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        run_date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        cur.execute(
            f"INSERT INTO {table_name} (run_number, result, execution_time, arguments, test, run_date, cpu_percent, memory_percent) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            (run_number, result, execution_time, json.dumps(arguments), "not tested", run_date, cpu_percent, memory_percent))

        conn.commit()
        conn.close()

    def create_usage_table(self, config_name):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_usage"
        cur.execute(f'''
            CREATE TABLE IF NOT EXISTS {table_name} (
                run_number INTEGER,
                cpu_usage REAL,
                memory_usage REAL,
                timestamp TEXT
            )
        ''')

        conn.commit()
        conn.close()

    def insert_usage(self, config_name, run_number, cpu_usage, memory_usage):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_usage"
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        cur.execute(
            f"INSERT INTO {table_name} (run_number, cpu_usage, memory_usage, timestamp) VALUES (?, ?, ?, ?)",
            (run_number, cpu_usage, memory_usage, timestamp))

        conn.commit()
        conn.close()

    def get_data_as_json(self, config_name):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        results = cur.execute(f'SELECT * FROM {table_name}').fetchall()

        conn.close()

        data = [dict(zip(["run_number", "result", "execution_time", "arguments", "test", "run_date", "cpu_percent",
                          "memory_percent"], row)) for row in results]

        return json.dumps(data)

    def update_data_json_file(self, config_name):
        data = self.get_data_as_json(config_name)

        with open(f'results/{config_name}.json', 'w') as f:
            f.write(data)

    def get_config_names(self):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        cur.execute("SELECT name FROM sqlite_master WHERE type='table'")
        table_names = cur.fetchall()

        config_names = [name[0].replace('_results', '') for name in table_names if name[0].endswith('_results')]

        conn.close()

        return config_names

    def perform_regression_test(self, config_name, latest_result, arguments):
        conn = sqlite3.connect(self.database_name)
        cursor = conn.cursor()

        table_name = f"{config_name}_results"

        # Find all results with the same arguments in the database
        cursor.execute(f"SELECT result FROM {table_name} WHERE arguments = ?",
                       (json.dumps(arguments),))
        results = cursor.fetchall()

        if len(results) >= 2:
            first_result = results[0]

            # Compare the first result with the latest result
            test_passed = first_result[0] == latest_result

            if test_passed:
                label = "passed"
            else:
                label = "failed"

            # Retrieve the run_number of the latest result
            cursor.execute(
                f"SELECT run_number FROM {table_name} WHERE arguments = ? AND result = ? ORDER BY run_number DESC LIMIT 1",
                (json.dumps(arguments), latest_result))
            latest_run_number = cursor.fetchone()[0]

            # Update the latest result with the test label
            cursor.execute(f"UPDATE {table_name} SET test = ? WHERE run_number = ?",
                           (label, latest_run_number))
            conn.commit()

            print(
                f"Test complete. The latest result is {'the same as' if test_passed else 'different from'} the first result.")
        else:
            print("Less than two matching results found in the database for the given arguments. Test not performed.")

        conn.close()

    def get_data_as_dataframe(self, config_name):
        conn = sqlite3.connect(self.database_name)
        table_name = f"{config_name}_results"
        df = pd.read_sql_query(f"SELECT * FROM {table_name}", conn)
        conn.close()
        return df

    def get_usage_data(self, config_name, run_number):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_usage"
        cur.execute(f"SELECT * FROM {table_name} WHERE run_number = ?", (run_number,))
        usage_data = cur.fetchall()

        conn.close()

        # Convert the raw data to a list of dictionaries for easier processing
        data = [dict(zip(["run_number", "cpu_usage", "memory_usage", "timestamp"], row)) for row in usage_data]

        return data

