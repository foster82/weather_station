#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "esp_bt.h"      
#include "esp_wifi.h"    
#include "config.h"      // <-- Imports your private credentials!

#define DHTPIN 4
#define DHTTYPE DHT11

// MQTT Topics
const char* temp_bmp_topic = "sensor2/temp_bmp";
const char* temp_dht_topic = "sensor2/temp_dht";
const char* humidity_topic = "sensor2/humidity";
const char* pressure_topic = "sensor2/pressure";

// Deep Sleep Settings
#define uS_TO_S_FACTOR 1000000ULL  
#define TIME_TO_SLEEP  600         

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
    } else {
        Serial.println("\nFailed to connect to WiFi. Going back to sleep...");
        goToSleep();
    }
}

void connectMQTT() {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    int attempts = 0;
    
    while (!client.connected() && attempts < 5) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32WeatherStation")) {
            Serial.println(" Connected!");
        } else {
            Serial.print(" Failed. Retry in 2 seconds...");
            delay(2000);
            attempts++;
        }
    }
    
    if (!client.connected()) {
        Serial.println("\nFailed to connect to MQTT. Going back to sleep...");
        goToSleep();
    }
}

// Dedicated function to shut everything down cleanly before sleeping
void goToSleep() {
    Serial.print("💤 Going to deep sleep for ");
    Serial.print(TIME_TO_SLEEP);
    Serial.println(" seconds.");
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void setup() {
    btStop();
    
    Serial.begin(115200);
    delay(1000); 
    
    Serial.println("🚀 Waking up from deep sleep...");

    if (!bmp.begin(0x76)) {
        Serial.println("❌ Could not find BMP280 sensor!");
    }
    dht.begin();
    
    delay(2000); 

    connectWiFi();
    connectMQTT();

    float temp_bmp = bmp.readTemperature();
    float temp_dht = dht.readTemperature();
    float humidity = dht.readHumidity();
    float pressure = bmp.readPressure() / 100.0F;

    Serial.println("📡 Sending sensor data to MQTT...");

    if (client.connected()) {
        client.publish(temp_bmp_topic, String(temp_bmp).c_str());
        client.publish(temp_dht_topic, String(temp_dht).c_str());
        client.publish(humidity_topic, String(humidity).c_str());
        client.publish(pressure_topic, String(pressure).c_str());
        Serial.println("✅ Data published successfully!");
    }

    client.loop();
    delay(500); 

    goToSleep();
}

void loop() {
    // Empty
}
