from flask import Flask, render_template, make_response
from flask import redirect, request, jsonify, url_for

import sqlite3
from datetime import datetime
import json
import os

from HydroCam import count_marks

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

@app.route('/getdata', methods=['GET'])
def get_data():
    conn = sqlite3.connect("HydroCam.db")
    c    = conn.cursor()
    #  WHERE date > \"{}\" AND date < \"{}\"
    q    =  'SELECT * FROM readings;'.format(request.args['from_date'], request.args['to_date'])
    r    = c.execute(q).fetchall()
    data = [{'date': x[0], 'count': x[1], 'scale': x[2]} for x in r]
    return json.dumps(data)

@app.route('/postimg', methods=['POST'])
def post_image():
    print("POST RECEIVED")
    print(request.headers['Content-Length'])

    marker_count, scale = count_marks("photo.jpg")
    if (marker_count != -1 and scale != -1):
        print("Saving Count: {}    Scale: {}".format(marker_count, scale))
        conn = sqlite3.connect("HydroCam.db")
        c    = conn.cursor()
        q = """INSERT INTO readings (date, reading, scale)
            VALUES(datetime('now', 'localtime'), {}, {});""".format(round(marker_count, 2), int(scale))
        c.execute(q)
        conn.commit()
        conn.close()

    data = {"message": "Message Received", "code": "SUCCESS"}
    return make_response(jsonify(data), 200)

def str_2_date(s):
    return datetime.strptime(s, "%Y-%m-%d %H:%M:%S")

if __name__ == "__main__":
    print("Starting Server")
    conn    = sqlite3.connect("HydroCam.db")
    c       = conn.cursor()
    c.execute("""CREATE TABLE IF NOT EXISTS readings (
        date TEXT PRIMARY KEY NOT NULL,
        reading REAL NOT NULL,
        scale INTEGER NOT NULL
    );""")
    conn.close()

    app.run(host=HOSTNAME, port=PORT)