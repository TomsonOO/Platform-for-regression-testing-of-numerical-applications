from flask import Flask, render_template
import json
import matplotlib.pyplot as plt
import io
import base64

app = Flask(__name__)

@app.route('/', methods=['GET', 'POST'])
def index():
    with open('results/data.json', 'r') as f:
        data = json.load(f)

    # Generate histogram
    # plt.hist([float(et['execution_time']) for et in data['execution_times']])
    # plt.xlabel('Execution Time (seconds)')
    # plt.ylabel('Frequency')
    # plt.title('Execution Time Distribution')
    #
    # # Convert plot to image and encode in base64
    # buffer = io.BytesIO()
    # plt.savefig(buffer, format='png')
    # buffer.seek(0)
    # img_base64 = base64.b64encode(buffer.getvalue()).decode()

    # Pass the base64 encoded image to the HTML template    img_base64=img_base64
    return render_template('index.html', data=data)

if __name__ == '__main__':
    app.run(debug=True)
