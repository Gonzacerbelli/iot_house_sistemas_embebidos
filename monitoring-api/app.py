from flask import Flask, request, Response
from datetime import datetime
import logging
import time
import json

from database import db_session, Events

app = Flask(__name__)


@app.route("/", methods=["GET"])
def hello():
    try:
        return Response('1', status=200, mimetype='application/json')

    except Exception as e:
        return Response(json.dumps({"error": str(e)}), status=500, mimetype='application/json')


@app.route("/carga", methods=["GET"])
def carga_de_datos():
    try:
        print(request)
        print(request.args.get("temperatura"))

        temperature = request.args.get("temperatura")
        heat_index = request.args.get("indiceDeCalor")
        humedity = request.args.get("humedad")
        water_tank = request.args.get("nivelTanque")

        timestamp = datetime.now()

        event = Events(temperature=temperature, heat_index=heat_index, humedity=humedity, water_tank=water_tank, timestamp=timestamp)

        db_session.add(event)
        db_session.commit()

        time.sleep(1)
        return Response('1', status=200, mimetype='application/json')

    except Exception as e:
        return Response(json.dumps({"error": str(e)}), status=500, mimetype='application/json')


if __name__ == '__main__':
    logging.basicConfig()
    app.run(debug=True)
