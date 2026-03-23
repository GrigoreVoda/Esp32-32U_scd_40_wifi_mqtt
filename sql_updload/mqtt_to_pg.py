import os
import json
import psycopg2
from paho.mqtt import client as mqtt_client

# 1. DEFINE CONFIG FIRST (at the top level, outside any functions)
DB_CONFIG = {
    "host": os.getenv("PG_HOST", "host.docker.internal"),
    "database": os.getenv("PG_DB", "mqtt"),
    "user": os.getenv("PG_USER", ""),
    "password": os.getenv("PG_PASSWORD", "")
}

TOPICS = [("office/co2", 0), ("office/temp", 0), ("office/hum", 0)]
topic_cache = {}

# 2. DEFINE YOUR FUNCTIONS
def get_sensor_id(cur, topic):
    if topic in topic_cache:
        return topic_cache[topic]
    
    cur.execute(
        "INSERT INTO sensors (topic_name) VALUES (%s) "
        "ON CONFLICT (topic_name) DO UPDATE SET topic_name = EXCLUDED.topic_name "
        "RETURNING id", (topic,)
    )
    s_id = cur.fetchone()[0]
    topic_cache[topic] = s_id
    return s_id

def on_message(client, userdata, msg):
    try:
        conn = userdata['conn']
        cur = conn.cursor()
        
        payload = msg.payload.decode()
        try:
            val = float(payload)
        except:
            val = float(json.loads(payload))

        s_id = get_sensor_id(cur, msg.topic)

        cur.execute(
            "INSERT INTO sensor_data (sensor_id, value) VALUES (%s, %s)",
            (s_id, val)
        )
        conn.commit()
        cur.close()
    except Exception as e:
        print(f"Error in on_message: {e}")

# 3. RUN THE MAIN LOGIC
try:
    # This line uses the DB_CONFIG defined at the top
    conn = psycopg2.connect(**DB_CONFIG)
    print("Successfully connected to Postgres")
    
    client = mqtt_client.Client(userdata={'conn': conn})
    client.on_connect = lambda c, u, f, rc: c.subscribe(TOPICS)
    client.on_message = on_message

    client.connect(os.getenv("MQTT_BROKER", "mqtt-broker"), 1883)
    client.loop_forever()

except Exception as e:
    print(f"Could not start script: {e}")
