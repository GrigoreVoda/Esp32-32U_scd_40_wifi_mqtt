# ESP32 Raumluftqualitäts-Monitor

> Echtzeit-Messung von CO₂, Temperatur und Luftfeuchtigkeit über WiFi, MQTT und PostgreSQL — realisiert mit einem ESP32-32U und dem Sensirion SCD41.

![Platform](https://img.shields.io/badge/Platform-ESP32--32U-blue)
![Sensor](https://img.shields.io/badge/Sensor-Sensirion%20SCD41-green)
![Protokoll](https://img.shields.io/badge/Protokoll-MQTT-orange)
![Broker](https://img.shields.io/badge/Broker-Mosquitto%20%28Docker%29-lightblue)

---

## Projektübersicht

Dieses Projekt realisiert einen eigenständigen IoT-Sensorknoten, der kontinuierlich die Raumluftqualität erfasst und die Messwerte per WiFi über das MQTT-Protokoll überträgt. Die Daten werden lokal auf einem OLED-Display angezeigt und durch drei LEDs farblich bewertet.

Das Projekt ist Teil einer **gemeinsamen Klasseninfrastruktur**: Die Docker-Dienste (Mosquitto-Broker und PostgreSQL) laufen in einer VM im Remote-Labor der Schule. Jeder ESP32-Knoten im Klassenraum publisht seine Messwerte auf denselben Broker — alle Teilnehmer können die Daten in Echtzeit abonnieren und auswerten.
<img width="3178" height="2045" alt="1000027480" src="https://github.com/user-attachments/assets/0dc46b33-00f6-495e-89e7-e0c8039593d9" />


---

## Systemarchitektur

```
  Klassenraum
 ┌─────────────────────────────────────┐
 │  ESP32-Knoten (pro Student)         │
 │                                     │
 │  ┌──────────┐    ┌───────────────┐  │
 │  │ SCD41    │I²C │  ESP32 MCU    │  │
 │  │ CO₂/T/H  ├───►│  Arduino FW   │  │
 │  └──────────┘    │               │  │
 │                  │  ┌──────────┐ │  │
 │  ┌──────────┐    │  │ SSD1306  │ │  │
 │  │ RGB-LEDs │◄───┤  │ OLED     │ │  │
 │  └──────────┘    └──────┬──────┘ │  │
 └─────────────────────────┼────────┘
                           │ WiFi / MQTT (publish)
                           │
  Remote-Labor VM          ▼
 ┌─────────────────────────────────────┐
 │  Docker Compose                     │
 │                                     │
 │  ┌──────────────────────────────┐   │
 │  │  Eclipse Mosquitto :1883     │◄──┼── alle Knoten publishen hier
 │  └───────────────┬──────────────┘   │
 │                  │ subscribe        │
 │                  │◄─────────────────┼── alle Studenten können abonnieren
 │  ┌───────────────▼──────────────┐   │
 │  │  Python Subscriber           │   │
 │  │  (sql_updload/)              │   │
 │  └───────────────┬──────────────┘   │
 │                  │                  │
 │  ┌───────────────▼──────────────┐   │
 │  │  PostgreSQL Datenbank        │   │
 │  └──────────────────────────────┘   │
 └─────────────────────────────────────┘
```

---

## Funktionen

- **CO₂-Messung** via Sensirion SCD41 (NDIR-Photoakustik, ±40 ppm Genauigkeit)
- **Temperatur & Luftfeuchtigkeit** aus demselben Sensorgehäuse
- **OLED-Liveanzeige** — CO₂ in großer Schrift, Temperatur/Feuchtigkeit unten, WiFi-Statusanzeige
- **Dreistufige LED-Ampel** für sofortige visuelle Rückmeldung

  | LED      | Bedingung         | Bedeutung              |
  |----------|-------------------|------------------------|
  | 🟢 Grün  | CO₂ ≤ 1000 ppm    | Gute Luftqualität      |
  | 🟡 Gelb  | 1000 – 1500 ppm   | Lüften empfohlen       |
  | 🔴 Rot   | > 1500 ppm        | Schlechte Luftqualität |

- **Nicht-blockierender WiFi/MQTT-Reconnect** — der Knoten erholt sich automatisch von Netzwerkausfällen
- **MQTT-Veröffentlichung** alle 10 Sekunden auf drei Topics
- **Docker-basierter Broker** — einfach deploybar auf jedem Linux-Server
- **PostgreSQL-Persistenz** über Python-Subscriber in `sql_upload/`

---

## Hardware

| Komponente | Details |
|------------|---------|
| Mikrocontroller | ESP32-32U (mit integrierter Antenne) |
| CO₂-Sensor | Sensirion SCD41 (I²C, Adresse `0x62`) |
| Display | SSD1306 128×64 OLED (I²C, Adresse `0x3C`) |
| LEDs | Rot, Gelb, Grün (GPIO 2, 4, 5) |
| I²C-Bus | SDA → GPIO 21, SCL → GPIO 22 |

### Verdrahtung

```
ESP32-32U          SCD41 / OLED
──────────────────────────────────
GPIO 21 (SDA)  ──► SDA
GPIO 22 (SCL)  ──► SCL
3,3 V          ──► VCC
GND            ──► GND

GPIO  2        ──► LED Rot    (+ 220 Ω Widerstand)
GPIO  4        ──► LED Gelb   (+ 220 Ω Widerstand)
GPIO  5        ──► LED Grün   (+ 220 Ω Widerstand)
```

---

## Projektstruktur

```
.
├── sketch_scd41_esp_32u.ino   # Haupt-Firmware (Arduino)
├── SensirionI2cScd4x.cpp      # Sensirion SCD4x Treiber
├── SensirionI2cScd4x.h        # Sensirion SCD4x Treiber Header
├── sql_updload/               # Python MQTT → PostgreSQL Subscriber
└── README.md
```

---

## Firmware einrichten

### Voraussetzungen

- [Arduino IDE](https://www.arduino.cc/en/software) ≥ 2.x **oder** [arduino-cli](https://arduino.github.io/arduino-cli/)
- ESP32-Boardpaket von Espressif (Board-Manager-URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`)

### Benötigte Bibliotheken

Über den Arduino Library Manager installieren:

| Bibliothek | Zweck |
|------------|-------|
| `PubSubClient` | MQTT-Client |
| `Adafruit GFX Library` | OLED-Grafikprimitive |
| `Adafruit SSD1306` | OLED-Treiber |

Der Sensirion-SCD4x-Treiber ist **direkt im Repository enthalten** (`SensirionI2cScd4x.cpp/.h`) — keine separate Installation nötig.

### Konfiguration

`sketch_scd41_esp_32u.ino` öffnen und die eigenen Zugangsdaten eintragen:

```cpp
const char* ssid        = "DEIN_WLAN_NAME";
const char* password    = "DEIN_WLAN_PASSWORT";
const char* mqtt_server = "IP_DES_BROKERS";
```

### Flashen

1. ESP32 per USB verbinden
2. **Board:** `ESP32 Dev Module`, richtigen **Port** auswählen
3. **Hochladen** klicken

---

## MQTT-Broker (Remote-Labor)

Der Mosquitto-Broker sowie die PostgreSQL-Datenbank laufen als Docker-Container in einer **VM im Remote-Labor der Schule** und sind für alle Teilnehmer über das Schulnetzwerk erreichbar.

### Broker verbinden

Die IP-Adresse des Brokers in der Firmware eintragen:

```cpp
const char* mqtt_server = "IP_DER_LAB_VM";
```

### Als Student abonnieren

Jeder Teilnehmer kann die Daten aller Knoten im Klassenraum live empfangen:

```bash
# Alle Topics abonnieren
mosquitto_sub -h IP_DER_LAB_VM -t "office/#" -v

# Nur CO₂-Werte
mosquitto_sub -h IP_DER_LAB_VM -t "office/co2"
```

### Broker-Setup (Referenz)

Die Docker-Konfiguration des Brokers liegt im Repository-Root. Für die Einrichtung auf einem neuen Server:

```bash
mkdir -p mqtt/{config,data,log}
```

`mqtt/config/mosquitto.conf`:

```
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
listener 1883
allow_anonymous true
```

`docker-compose.yml`:

```yaml
version: '3.8'
services:
  mqtt:
    image: eclipse-mosquitto:2
    container_name: mqtt-broker
    restart: unless-stopped
    ports:
      - "1883:1883"
    volumes:
      - ./mqtt/config:/mosquitto/config
      - ./mqtt/data:/mosquitto/data
      - ./mqtt/log:/mosquitto/log
```

```bash
docker compose up -d
docker logs -f mqtt-broker
```

**Fedora / SELinux:** Bei Berechtigungsproblemen mit den gemounteten Volumes:

```bash
sudo chcon -Rt svirt_sandbox_file_t ./mqtt/config ./mqtt/data ./mqtt/log
```

---

## MQTT-Topics

| Topic | Nutzlast | Einheit | Beispiel |
|-------|----------|---------|---------|
| `office/co2` | Ganzzahl | ppm | `842` |
| `office/temp` | Dezimalzahl (1 Stelle) | °C | `21.4` |
| `office/hum` | Dezimalzahl (0 Stellen) | % rF | `54` |

Alle Topics gleichzeitig abonnieren:

```bash
mosquitto_sub -h localhost -t "office/#" -v
```

---

## Datenbankpersistenz (`sql_updload/`)

Das Modul `sql_updload` enthält einen Python-Subscriber, der als eigenständiger **Docker-Container** in der Lab-VM läuft. Er abonniert alle MQTT-Topics des gemeinsamen Brokers und schreibt jeden eingehenden Messwert (CO₂, Temperatur, Luftfeuchtigkeit) mit Zeitstempel in eine PostgreSQL-Tabelle — die Daten aller Knoten im Klassenraum landen so zentral in einer Datenbank.

### Architektur

```
[alle ESP32-Knoten] ──MQTT──► [Mosquitto-Container]
                                       │
                              [Python-Subscriber-Container]
                                       │
                              [PostgreSQL-Container]
```

### Starten

```bash
cd sql_updload
docker compose up -d
```

---

## Sicherheitshinweise

Die aktuelle Konfiguration erlaubt anonymen MQTT-Zugriff (Entwicklungsmodus). Für den produktiven Einsatz empfiehlt sich:

- Benutzername/Passwort-Authentifizierung in `mosquitto.conf` aktivieren
- TLS auf Port 8883 aktivieren
- `allow_anonymous false` setzen

---

