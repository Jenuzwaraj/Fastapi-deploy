#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

// ---- WiFi Credentials ----
const char* ssid = "hotspot";
const char* password = "12345678";

// ---- API Endpoint ----
const char* serverName = "http://10.115.208.26:8000/sensor-data";
// ---- DHT11 Setup ----
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---- Moisture Sensor ----
#define MOISTURE_PIN A0

// ---- Time Metadata ----
String created_at = "2025-10-25T10:00:00Z";
String updated_at = "2025-10-25T10:05:00Z";

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int moistureValue = analogRead(MOISTURE_PIN);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("❌ DHT11 read failed!");
      delay(2000);
      return;
    }

    // Convert moisture to %
    int moisturePercent = map(moistureValue, 1023, 0, 0, 100); 
    moisturePercent = constrain(moisturePercent, 0, 100);

    // --- JSON Payload ---
    String jsonData = "{";
    jsonData += "\"device_id\": 1,";
    jsonData += "\"readings\": [";

    // Soil Moisture Data
    jsonData += "{";
    jsonData += "\"sensor_id\": 1,";
    jsonData += "\"sensor_name\": \"soil moisture\",";
    jsonData += "\"reading_type\": \"moisture\",";
    jsonData += "\"reading\": " + String(moisturePercent) + ",";
    jsonData += "\"units\": \"%\",";
    jsonData += "\"created_at\": \"" + created_at + "\",";
    jsonData += "\"updated_at\": \"" + updated_at + "\"";
    jsonData += "},";

    // Temperature + Humidity
    jsonData += "{";
    jsonData += "\"sensor_id\": 2,";
    jsonData += "\"sensor_name\": \"dht11 sensor\",";
    jsonData += "\"reading_type\": \"temperature\",";
    jsonData += "\"reading\": " + String(temperature, 1) + ",";
    jsonData += "\"units\": \"C\",";
    jsonData += "\"created_at\": \"" + created_at + "\",";
    jsonData += "\"updated_at\": \"" + updated_at + "\"";
    jsonData += "},";

    jsonData += "{";
    jsonData += "\"sensor_id\": 3,";
    jsonData += "\"sensor_name\": \"dht11 sensor\",";
    jsonData += "\"reading_type\": \"humidity\",";
    jsonData += "\"reading\": " + String(humidity, 1) + ",";
    jsonData += "\"units\": \"%\",";
    jsonData += "\"created_at\": \"" + created_at + "\",";
    jsonData += "\"updated_at\": \"" + updated_at + "\"";
    jsonData += "}";

    jsonData += "]}";

    // --- Send POST Request ---
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Response: " + response);
    } else {
      Serial.print("POST failed, error: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();

    // Debug values locally
    Serial.println("----- Sensor Readings -----");
    Serial.printf("Moisture: %d%% | Temperature: %.1f°C | Humidity: %.1f%%\n",
                  moisturePercent, temperature, humidity);
    Serial.println("---------------------------");
  } else {
    Serial.println("❌ WiFi disconnected!");
  }

  delay(10000); // Send every 10 seconds
}
