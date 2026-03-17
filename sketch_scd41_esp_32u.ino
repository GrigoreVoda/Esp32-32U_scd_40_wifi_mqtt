#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";


#define I2C_SDA 21
#define I2C_SCL 22
#define LED_RED     2
#define LED_YELLOW  4
#define LED_GREEN   5


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SensirionI2cScd4x scd4x; 
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMqttRetry = 0;
unsigned long lastPublish = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Initialize Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED Failed");
    for(;;); // Don't proceed if display fails
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();

  // Start WiFi
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);

  // Initialize SCD41
  scd4x.begin(Wire, 0x62);
  scd4x.stopPeriodicMeasurement();
  delay(500);
  scd4x.startPeriodicMeasurement();
  
  Serial.println("System Initialized");
}

void maintainConnections() {
  // WiFi Check
  if (WiFi.status() != WL_CONNECTED) {
    return; // Don't try MQTT if WiFi is down
  }

  // Non-blocking MQTT Reconnect (try every 5 seconds)
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastMqttRetry > 5000) {
      lastMqttRetry = now;
      Serial.print("Attempting MQTT connection...");
      if (client.connect("ESP32_32U_CO2_Sensor")) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.println(client.state());
      }
    }
  }
}

void loop() {
  maintainConnections();
  client.loop();

  uint16_t co2 = 0;
  float temperature = 0.0;
  float humidity = 0.0;
  bool dataReady = false;

  scd4x.getDataReadyStatus(dataReady);

  if (dataReady) {
    uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);
    
    if (error == 0 && co2 != 0) {
      // 1. LED Logic
      digitalWrite(LED_RED, co2 > 1500);
      digitalWrite(LED_YELLOW, (co2 > 1000 && co2 <= 1500));
      digitalWrite(LED_GREEN, co2 <= 1000);

      // 2. MQTT Publish (Every 10 seconds)
      if (client.connected() && (millis() - lastPublish > 10000)) {
        lastPublish = millis();
        client.publish("office/co2", String(co2).c_str());
        client.publish("office/temp", String(temperature, 1).c_str());
        client.publish("office/hum", String(humidity, 0).c_str());
      }

      // 3. Display Update
      display.clearDisplay();
      
      // WiFi Status Icon (Top Right)
      display.setCursor(100, 0);
      display.print(WiFi.status() == WL_CONNECTED ? "W+" : "W-");

      // CO2 Large Text
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("CO2 Level:");
      
      display.setTextSize(3);
      display.setCursor(0, 15);
      display.print(co2);
      
      display.setTextSize(1);
      display.setCursor(90, 30);
      display.print("ppm");

      // Temp/Hum Bottom
      display.setCursor(0, 50);
      display.printf("T:%.1fC  H:%.0f%%", temperature, humidity);
      
      display.display();
    }
  }
  
  delay(100); // Small delay for stability
}
