FROM python:3.11-slim

WORKDIR /app

COPY mqtt_to_pg.py .

RUN pip install paho-mqtt psycopg2-binary

CMD ["python", "mqtt_to_pg.py"]