import os
import json
import psycopg2
from paho.mqtt import client as mqtt_client

MQTT_BROKER = os.getenv("MQTT_BROKER", "mqtt")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))

PG_HOST = os.getenv("PG_HOST", "host.docker.internal")
PG_DB = os.getenv("PG_DB", "yourdb")
PG_USER = os.getenv("PG_USER", "youruser")
PG_PASSWORD = os.getenv("PG_PASSWORD", "yourpassword")

TOPICS = [("office/co2", 0), ("office/temp", 0), ("office/hum", 0)]

def connect_pg():
    return psycopg2.connect(
        host=PG_HOST,
        database=PG_DB,
        user=PG_USER,
        password=PG_PASSWORD
    )

conn = connect_pg()
cur = conn.cursor()

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT:", rc)
    client.subscribe(TOPICS)

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()

        # try parse as number or JSON
        try:
            value = float(payload)
        except:
            value = float(json.loads(payload))

        cur.execute(
            "INSERT INTO sensor_data (topic, value) VALUES (%s, %s)",
            (msg.topic, value)
        )
        conn.commit()

        print(f"{msg.topic}: {value}")

    except Exception as e:
        print("Error:", e)

client = mqtt_client.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_BROKER, MQTT_PORT)
client.loop_forever()