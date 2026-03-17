# MQTT Broker with Docker Compose (Fedora)



## Prerequisites

* Fedora Linux
* Docker
* Docker Compose

Install dependencies:

```bash
sudo dnf install docker docker-compose -y
sudo systemctl enable --now docker
```

(Optional: run Docker without sudo)

```bash
sudo usermod -aG docker $USER
```

Log out and back in for group changes to take effect.

---

## Project Structure

```
mqtt/
├── docker-compose.yml
├── config/
│   └── mosquitto.conf
├── data/
└── log/
```

Create it with:

```bash
mkdir -p mqtt/{config,data,log}
cd mqtt
```

---

## Configuration

Create `config/mosquitto.conf`:

```conf
persistence true
persistence_location /mosquitto/data/

log_dest file /mosquitto/log/mosquitto.log

listener 1883
allow_anonymous true
```

---

## Docker Compose

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  mqtt:
    image: eclipse-mosquitto:2
    container_name: mqtt-broker
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./config:/mosquitto/config
      - ./data:/mosquitto/data
      - ./log:/mosquitto/log
```

---

## Start the Broker

```bash
docker-compose up -d
```

Check logs:

```bash
docker logs -f mqtt-broker
```

---



## Fedora (SELinux) Notes

If you encounter permission issues:

```bash
sudo chcon -Rt svirt_sandbox_file_t ./config ./data ./log
```

Temporary workaround (not recommended for production):

```bash
sudo setenforce 0
```

---

## Security (Optional Improvements)

* Disable anonymous access
* Add username/password authentication
* Enable TLS (secure MQTT on port 8883)

---

## Useful Tools

* Node-RED (flow-based automation)
* MQTT Explorer (GUI client for MQTT)

---

## License

MIT License
