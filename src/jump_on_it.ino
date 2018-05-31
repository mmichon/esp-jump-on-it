#include <Adafruit_GFX.h>     // pio lib install 13
#include <Adafruit_SSD1306.h> // git clone https://github.com/mcauser/Adafruit_SSD1306/tree/esp8266-64x48 lib
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h> // pio lib install 567
#include <Wire.h>

#include "config.h"

/* Sample request/response:
foo:~ mmichon$ curl -u "user:pass" "https://app.socialbicycles.com/api/bikes.json?latitude=37.0&longitude=-122.0&sort=distance_asc&page=1&per_page=3&access_key=your_access_key" | jq
{
  "current_page": 1,
  "per_page": 3,
  "total_entries": 214,
  "items": [
    {
      "id": 17948,
      "name": "0321",
      "network_id": 155,
      "sponsored": false,
      "ebike_battery_level": 63,
      "ebike_battery_distance": 23.94,
      "hub_id": null,
      "inside_area": true,
      "address": "877-899 Illinois Street, San Francisco, CA",
      "current_position": {
        "type": "Point",
        "coordinates": [
          -122.38764666666667,
          37.760936666666666
        ]
      },
      "distance": 285.96
    },
    {
      "id": 17949,
*/

#define LED_BUILTIN 2
#define OLED_RESET 0 // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

WiFiManager wifiManager;
WiFiClientSecure client;

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("Connecting to display...");

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 64x48)
    clear_display();

    Serial.println("Connecting to wifi...");

    // Wait up to 3 minutes to connect to AP
    wifiManager.setTimeout(180);

    // Configure wifi on first connect otherwise just connect to the last known
    if (!wifiManager.autoConnect("Jump On It")) {
        Serial.println("Failed to connect and hit timeout; resetting");
        delay(3000);

        ESP.reset();
        delay(5000);
    }

    Serial.println("Downloading prediction...");

    int first_prediction, second_prediction, first_percent, second_percent;
    if (download_prediction_json(first_prediction, second_prediction, first_percent, second_percent) == "ERROR") {
        display_error();
    } else {
        Serial.println("Displaying prediction...");
        display_predictions(first_prediction, second_prediction, first_percent, second_percent);
    }

    display.display();

    // Turn on the onboard LED if we have a nearby charged bike
    if ((int(first_prediction / 84) >= 5) && (first_percent >= 50)) {
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
    }

    // This requires GPIO16 (D0) to be shorted to RST (not when programming)
    Serial.println("Going to sleep.");
    ESP.deepSleep(POLL_SECONDS * 1000000, WAKE_RF_DEFAULT);
}

void loop() {
}

// Clears the display and sets basic settings
void clear_display() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
}

// Displays predictions on the display
void display_predictions(int first_prediction, int second_prediction, int first_percent, int second_percent) {
    clear_display();

    display.setTextSize(1);
    display.println("Jump bikes");
    display.println();

    display.setTextSize(2);
    display.print(String(int(first_prediction / 84))); // 1.4m/s walking rate according to wikipedia
    display.setTextSize(1);
    display.print(" " + String(first_percent) + "%");
    display.setTextSize(2);
    display.println();

    display.print(String(int(second_prediction / 84)));
    display.setTextSize(1);
    display.println(" " + String(second_percent) + "%");
}

// Display error in case of problems getting predictions
void display_error() {
    clear_display();

    Serial.println("Displaying error");

    display.setTextSize(2);
    display.println("Error");

    display.setTextSize(1);
    display.println("No pred!");
}

// Retrieves distances (in meters) and charge level of the nearest two bikes from SoBi API
String download_prediction_json(int &first_prediction, int &second_prediction, int &first_percent, int &second_percent) {
    const char *host = "app.socialbicycles.com";

    Serial.print("Connecting to API...");

    if (!client.connect(host, 443)) {
        Serial.println(F("Connection failed"));
        return String("ERROR");
    }

    char url[256];
    sprintf(url,
            "https://app.socialbicycles.com/api/bikes.json?latitude=%s&longitude=%s&access_key=%s&sort=distance_asc&page=1&per_page=3",
            lat, lon, access_key);

    Serial.print("Requesting URL: ");
    Serial.println(url);

    String request = String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Authorization: Basic " + String(auth) + "\r\n" +
                     "Connection: close\r\n\r\n";
    Serial.println(request);
    client.print(request);
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(F("Client timeout!"));
            client.stop();

            return String("ERROR");
        }
    }

    // Check response status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
        Serial.print(F("Unexpected response: "));
        Serial.println(status);
        return String("ERROR");
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return String("ERROR");
    }

    // Use arduinojson.org/assistant to compute the buffer capacity
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);

    // Parse JSON object
    JsonObject &root = jsonBuffer.parseObject(client);
    if (!root.success()) {
        Serial.println(F("Parsing failed!"));
        return String("ERROR");
    }

    // Extract values
    Serial.println("Response:");

    // See top of this file for example JSON object
    JsonObject &item_0 = root["items"][0];
    const char *prediction_0 = item_0["distance"].as<char *>();
    Serial.print(prediction_0);
    first_prediction = atoi(prediction_0);
    const char *percent_0 = item_0["ebike_battery_level"].as<char *>();
    Serial.println(String(" @") + percent_0 + "%");
    first_percent = atoi(percent_0);

    JsonObject &item_1 = root["items"][1];
    const char *prediction_1 = item_1["distance"].as<char *>();
    Serial.print(prediction_1);
    second_prediction = atoi(prediction_1);
    const char *percent_1 = item_1["ebike_battery_level"].as<char *>();
    Serial.println(String(" @") + percent_1 + "%");
    second_percent = atoi(percent_1);

    Serial.println("Closing connection.");

    // Disconnect
    client.stop();

    return "";
}
