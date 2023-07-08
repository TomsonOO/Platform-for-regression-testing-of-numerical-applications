from flask import Flask, render_template, request, jsonify
from database_manager import DatabaseManager
import glob
import os

app = Flask(__name__)

def get_config_names():
    db_files = glob.glob('databases/*.db')
    config_names = [os.path.splitext(os.path.basename(db))[0] for db in db_files]
    return config_names

def get_data_for_config_from_database(config_name):
    db_manager = DatabaseManager(f'{config_name}.db', config_name)
    data = db_manager.get_data_as_json(config_name)
    return data

@app.route('/get_configs', methods=['GET'])
def get_configs():
    config_names = get_config_names()
    return jsonify(config_names)

@app.route('/get_data', methods=['GET'])
def get_data():
    config_name = request.args.get('config_name')
    print("KKKKKKKKKKKKK" + config_name)
    data = get_data_for_config_from_database(config_name)
    return jsonify(data)

@app.route('/')
def index():
    database_files = glob.glob('databases/*.db')
    database_names = [os.path.splitext(os.path.basename(db))[0] for db in database_files]
    default_config = database_names[0] if database_names else None
    default_data = get_data_for_config_from_database(default_config) if default_config else []
    return render_template('index.html', databases=database_names, default_data=default_data)

@app.route('/update_data', methods=['POST'])
def update_data():
    data = request.get_json()
    selected_db = data.get('selected_db')
    config_name = request.args.get('config_name')
    db_manager = DatabaseManager(f'{config_name}.db', config_name)
    data = db_manager.get_data_as_dataframe(selected_db)
    response = jsonify(data.to_dict(orient='records'))
    response.headers.add('Access-Control-Allow-Origin', '*')
    return response


@app.route('/run_details/<int:run_number>', methods=['GET'])
def run_details(run_number):
    database_files = glob.glob('databases/*.db')
    database_names = [os.path.splitext(os.path.basename(db))[0] for db in database_files]
    default_config = database_names[0] if database_names else None
    db_manager = DatabaseManager(f'{default_config}.db', default_config)
    data = db_manager.get_usage_data(default_config, run_number)
    print(data)
    return render_template('details.html', data=data, run_number=run_number)

if __name__ == '__main__':
    app.run(debug=True)