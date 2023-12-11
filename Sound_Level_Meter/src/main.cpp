#include <OLEDDisplay.h>
#include <SSD1306Wire.h>
#include <SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

const int analogPin = A6;   // Replace with the pin your sensor is connected to
const int numReadings = 10; // Number of readings to include in the moving average
int readings[numReadings];  // Array to store the readings
int indexB = 0;             // Index of the current reading
int total = 0;              // Running total of the readings
float voltage = 0;          // The calculated dB value

#define Geom GEOMETRY_128_32
#define OLED_address 0x3C
#define TOKEN "BBUS-gj1dc09Oy2d8XSChCQjkYK9x9lRxGq"
#define DEVICE_LABEL "ESP32"

SSD1306Wire display(OLED_address, 21, 22, Geom, I2C_ONE, 100000);

// Define your WiFi credentials
const char *ssid = "Smadi1";
const char *password = "smadi9496";
const char *mqttServer = "industrial.api.ubidots.com"; // ubidots server address
const int mqttPort = 1883;                             // MQTT server port
// const char* mqttUser = "smadiamer"; // MQTT username (if required)
// const char* mqttPassword = "q9!5xAWuVRCe4SG"; // MQTT password (if required)
const char *mqttTopic = "ESP32"; // MQTT topic where you want to publish data
const char *VARIABLE_LABEL1 = "decibelmeter";

long lastMsg = 0;

WiFiClient espClient;           // Create an instance of the WiFi client
PubSubClient client(espClient); // Create an MQTT client instance

// Connect to Wi-Fi
void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// void callback(char *topic, byte *message, unsigned int length)
// {
//   Serial.print("Message arrived on topic: ");
//   Serial.print(topic);
//   Serial.print(". Message: ");
//   String messageTemp;

//   for (int i = 0; i < length; i++)
//   {
//     Serial.print((char)message[i]);
//     messageTemp += (char)message[i];
//   }
//   Serial.println(messageTemp);
// }

void setup()
{
  Serial.begin(115200);

  display.init();
  display.setContrast(255);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  // client.setCallback(callback);
  // If your MQTT server requires authentication, set the username and password here
  // client.setCredentials(mqttUser, mqttPassword);

  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32", TOKEN, ""))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;

    const int sensorValue = analogRead(analogPin); // Read the new sensor value
    total -= readings[indexB];               // Subtract the oldest reading from the total
    total += sensorValue;                    // Add the new reading to the total
    readings[indexB] = sensorValue;          // Store the new reading in the array
    indexB = (indexB + 1) % numReadings;     // Move to the next index, wrapping around if necessary
    //  int analogValue = map(sensorValue, 0, 4095, 0, 1023);

    // Calculate the RMS value
    const float rms = sqrt((float)total / numReadings);

    // Calculate sound level in dB based on reference sound pressure (20 µPa)
    const float referencePressure = 0.16168; // 20 µPa

    const float soundLevel_dB = 20 * log10(rms / referencePressure);

    // Display dB value on OLED
    const String soundString(soundLevel_dB, 2);

    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    //  display.drawString(0, 10, "Voltage: " + String(voltageMov, 3) + " V");
    display.drawString(0, 0, soundString + " dB");
    display.display();

    // char soundString[8];
    // dtostrf(soundLevel_dB, 1, 2, soundString);
    // Serial.print("Sound Level dB: ");
    // Serial.println(soundString);

    String payload;
    payload.concat("{\"");
    payload.concat(VARIABLE_LABEL1);
    payload.concat("\":");
    payload.concat(soundString);
    payload.concat("}");

    String topic;
    topic.concat("/v1.6/devices/");
    topic.concat(DEVICE_LABEL);
    client.publish(topic.c_str(), payload.c_str());
    Serial.println(soundString);

    // float voltageMov = (float(total) / numReadings) * (3.3f / 4096.0f); // Calculate the moving average
    // char voltageString[8];
    // dtostrf(voltageMov, 1, 2, voltageString);
    // Serial.print("Voltage: ");
    // Serial.println(voltageString);
    // client.publish("ESP32/voltage", voltageString);
  }
}
