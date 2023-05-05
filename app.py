from flask import Flask, jsonify, request
from database_manager import DatabaseManager
from flask_socketio import SocketIO, emit
import plotly.graph_objects as go
import plotly
import json
from typing import List

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


@app.route('/get_chart_data', methods=['GET'])
def get_chart_data():
    config_name = request.args.get('config_name')
    y_value = request.args.get('y_value', default='execution_time')
    y_title = request.args.get('y_title', default='Execution Time (s)')

    data = db_manager.get_data_as_dataframe(config_name)
    chart_data = [
        go.Scatter(
            x=data['run_number'],
            y=data[y_value],
            mode='lines+markers',
            name=y_title
        )
    ]

    layout = go.Layout(
        title=f"{y_title} vs Run Number",
        xaxis=dict(
            title="Run Number",
            showgrid=True,
            gridcolor='lightgray'
        ),
        yaxis=dict(
            title=y_title,
            showgrid=True,
            gridcolor='lightgray'
        ),
        plot_bgcolor='white'
    )

    response_data = {
        "data": chart_data,
        "layout": layout
    }

    return json.dumps(response_data, cls=plotly.utils.PlotlyJSONEncoder)


@app.route('/get_all_chart_data', methods=['GET'])
def get_all_chart_data():
    config_name = request.args.get('config_name')
    data = db_manager.get_data_as_dataframe(config_name)

    chart_data_execution_time = go.Scatter(
        x=data['run_number'],
        y=data['execution_time'],
        mode='lines+markers',
        name='Execution Time'
    )

    chart_data_cpu_percent = go.Scatter(
        x=data['run_number'],
        y=data['cpu_percent'],
        mode='lines+markers',
        name='CPU Percentage'
    )

    chart_data_memory_percent = go.Scatter(
        x=data['run_number'],
        y=data['memory_percent'],
        mode='lines+markers',
        name='Memory Percentage'
    )

    layout_execution_time = go.Layout(
        title="Execution Time vs Run Number",
        xaxis=dict(
            title="Run Number",
            showgrid=True,
            gridcolor='lightgray'
        ),
        yaxis=dict(
            title="Execution Time (s)",
            showgrid=True,
            gridcolor='lightgray'
        ),
        plot_bgcolor='white'
    )

    layout_cpu_percent = go.Layout(
        title="CPU Percentage vs Run Number",
        xaxis=dict(
            title="Run Number",
            showgrid=True,
            gridcolor='lightgray'
        ),
        yaxis=dict(
            title="CPU Percentage",
            showgrid=True,
            gridcolor='lightgray'
        ),
        plot_bgcolor='white'
    )

    layout_memory_percent = go.Layout(
        title="Memory Percentage vs Run Number",
        xaxis=dict(
            title="Run Number",
            showgrid=True,
            gridcolor='lightgray'
        ),
        yaxis=dict(
            title="Memory Percentage",
            showgrid=True,
            gridcolor='lightgray'
        ),
        plot_bgcolor='white'
    )

    response_data = {
        "execution_time_chart": {
            "data": [chart_data_execution_time],
            "layout": layout_execution_time
        },
        "cpu_percent_chart": {
            "data": [chart_data_cpu_percent],
            "layout": layout_cpu_percent
        },
        "memory_percent_chart": {
            "data": [chart_data_memory_percent],
            "layout": layout_memory_percent
        }
    }

    return json.dumps(response_data, cls=plotly.utils.PlotlyJSONEncoder)


if __name__ == '__main__':
    socketio.run(app)
