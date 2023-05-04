from flask import Flask, jsonify, request
from database_manager import DatabaseManager
from flask_socketio import SocketIO, emit

app = Flask(__name__)
# app.config['SECRET_KEY'] = 'mysecretkey'
socketio = SocketIO(app, cors_allowed_origins="*")
db_manager = DatabaseManager('results.db')


def get_config_names_from_database():
    config_names = db_manager.get_config_names()
    return config_names


def get_data_for_config_from_database(config_name):
    data = db_manager.get_data_as_json(config_name)
    return data


@app.route('/get_configs', methods=['GET'])
def get_configs():
    config_names = get_config_names_from_database()
    return jsonify(config_names)


@app.route('/get_data', methods=['GET'])
def get_data():
    config_name = request.args.get('config_name')
    data = get_data_for_config_from_database(config_name)
    return jsonify(data)


@app.route('/')
def index():
    return app.send_static_file('index.html')


@socketio.on('get_configs')
def handle_get_configs():
    config_names = get_config_names_from_database()
    emit('configs', config_names)


@socketio.on('trigger_update')
def handle_trigger_update(config_name):
    emit('update_data', config_name, broadcast=True)


if __name__ == '__main__':
    socketio.run(app)
