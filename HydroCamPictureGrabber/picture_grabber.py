from flask import Flask, render_template, make_response
from flask import redirect, request, jsonify, url_for

import sqlite3
from datetime import datetime
import time
import json
import os

HOSTNAME = "10.0.0.80"
PORT = 2023

app = Flask(__name__)
app.secret_key = "ESP32_FUN"
app.debug = True
app._static_folder = os.path.abspath("templates/static/")

@app.route('/', methods=['GET'])
def index():
    print("HOME GET RECEIVED")
    title = "HydroCam"
    return render_template('layouts/index.html', title=title)

@app.route('/postimg', methods=['POST'])
def post_image():
    print("POST RECEIVED")
    print(request.headers['Content-Length'])

    with open("photos/" + time.strftime("%Y%m%d-%H%M%S") + ".jpg", "wb") as f:
        f.write(request.files['imageFile'].read())

    data = {"message": "Message Received", "code": "SUCCESS"}
    return make_response(jsonify(data), 200)

def str_2_date(s):
    return datetime.strptime(s, "%Y-%m-%d %H:%M:%S")

if __name__ == "__main__":
    app.run(host=HOSTNAME, port=PORT)