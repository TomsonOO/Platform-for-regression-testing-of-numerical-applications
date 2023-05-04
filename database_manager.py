import sqlite3
import json


class DatabaseManager:
    def __init__(self, database_name):
        self.database_name = database_name

    def create_table_for_config(self, config_name):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        cur.execute(f'''
            CREATE TABLE IF NOT EXISTS {table_name} (
                run_number INTEGER,
                result TEXT,
                execution_time REAL,
                arguments TEXT
            )
        ''')

        conn.commit()
        conn.close()

    def insert_result(self, config_name, run_number, result, execution_time, arguments):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        cur.execute(f"INSERT INTO {table_name} (run_number, result, execution_time, arguments) VALUES (?, ?, ?, ?)",
                    (run_number, result, execution_time, json.dumps(arguments)))

        conn.commit()
        conn.close()

    def get_data_as_json(self, config_name):
        conn = sqlite3.connect(self.database_name)
        cur = conn.cursor()

        table_name = f"{config_name}_results"
        results = cur.execute(f'SELECT * FROM {table_name}').fetchall()

        conn.close()

        data = [dict(zip(["run_number", "result", "execution_time", "arguments"], row)) for row in results]

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
