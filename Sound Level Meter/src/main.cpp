#include <OLEDDisplay.h>
#include <SSD1306Wire.h>
#include <SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <ThingSpeak.h>

float soundLevel = 0.0f;
const int analogPin = 34;   // the pin connected to GPIO34
const int numReadings = 8; // Number of readings to include in the moving average
int readings[numReadings];  // Array to store the readings
int indexB = 0;             // Index of the current reading
int total = 0;              // Running total of the readings
const int gain40 = 100;
const float gain50 = 316.22777;
const int gain60 = 1000;
const float referencePressure = 0.00002; // 20 µPa reference pressure
const float sensitivity = 0.00631; // or 6.325 mV/dB sensor sensitivity
long lastMsg = 0; // for looping
const char *ssid = "Smadi1"; // for WiFi
const char *password = "smadi9496";

char thingSpeakAddress[] = "api.thingspeak.com";
const char *writeAPIKey = "GFXDIZX1JYJIMQFF";
const int thingsPort = 80;
unsigned long channelID = 2372038;
unsigned int dataFieldOne = 1;                            // Field to write SPL
unsigned int dataFieldTwo = 2;                            // Field to write output voltage
WiFiClient client;

#define Geom GEOMETRY_128_32 // Display
#define OLED_address 0x3C

SSD1306Wire display(OLED_address, 21, 22, Geom, I2C_ONE, 100000);

void setup_wifi()
{
  delay(10);
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
  ThingSpeak.begin(client);
}

int write2TSData( long TSChannel, unsigned int TSField1, float field1Data, unsigned int TSField2, float field2Data ){
     ThingSpeak.setField( TSField1, field1Data ); 
     ThingSpeak.setField( TSField2, field2Data );
   
     int writeSuccess = ThingSpeak.writeFields( TSChannel, writeAPIKey );
     return writeSuccess;
}

void setup()
{
  Serial.begin(115200);

  display.init();
  display.setContrast(255);
  display.flipScreenVertically();

  setup_wifi();

  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }
}

void loop()
{
    const int sensorValue = analogRead(analogPin); // szenzorról beolvasott adat
    const int rmsAnalog = sensorValue*sensorValue; // adat négyzetre emelése
    total -= readings[indexB];                     // legrégebbi elem törlése
    total += rmsAnalog;                            // legújabb elem hozzáadása
    readings[indexB] = rmsAnalog;                  // legújabb elem tárolása tömbben
    indexB = (indexB + 1) % numReadings;           // következő elemre mutatás, 0-ra áll ha eléri a végét
    const float averageAnalog = sqrt(total/numReadings); // nágyzetes közép számítása

    const float outputVoltage = (averageAnalog *(3.3/4096));
    
    const float soundPressure = ((outputVoltage)/(sensitivity*gain40));  // change the gain if it is connected to another pin

    if (soundPressure == 0)
    {
      soundLevel = 34.83832; // 60dB gain = 15.53832  50dB gain = 25.18832   40dB gain = 34.83832
    } else

    soundLevel = (19.88 * log10(soundPressure / referencePressure));  // calibration factor average is 0.9594*20=19.188 (4.061%)

    const String soundString(soundLevel, 2);

    const String voltageString(outputVoltage, 2);

    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, soundString + " dB");
    display.display();

    write2TSData(channelID, dataFieldOne, soundLevel, dataFieldTwo, outputVoltage);
    
    Serial.println("SPL: " + soundString);
    Serial.println("Voltage: " + voltageString);

    delay(125);
  }
  
// #include <PubSubClient.h>

// const char *mqttServer = "industrial.api.ubidots.com"; // ubidots server address
// const int mqttPort = 1883;                             // MQTT server port
// const char *mqttTopic = "ESP32"; // MQTT topic where you want to publish data
// const char *VARIABLE_LABEL1 = "decibelmeter";
// const char *VARIABLE_LABEL2 = "voltageoutput_spl";
// #define TOKEN "BBUS-gj1dc09Oy2d8XSChCQjkYK9x9lRxGq" // UBIDOTS token
// #define DEVICE_LABEL "ESP32" 
// WiFiClient espClient;           // Create an instance of the WiFi client
// PubSubClient client(espClient); // Create an MQTT client instance

//void reconnect()
//{
//  while (!client.connected())
//  {
//    Serial.print("Attempting MQTT connection...");
//    if (client.connect("ESP32", TOKEN, ""))
//    {
//      Serial.println("connected");
//    }
//    else
//    {
//      Serial.print("failed, rc=");
//      Serial.print(client.state());
//      Serial.println(" try again in 5 seconds");
//      delay(5000);
//    }
//  }
//}

//  if (!client.connected())
//  {
//    reconnect();
//  }

//    String payload;
//    payload.concat("{\"");
//    payload.concat(VARIABLE_LABEL1);
//    payload.concat("\":");
//    payload.concat(soundString1);
//    payload.concat(",\"");  
//    payload.concat(VARIABLE_LABEL2);  
//    payload.concat("\":");
//    payload.concat(voltageString);
//    payload.concat("}");

//    String topic;
//    topic.concat("/v1.6/devices/");
//    topic.concat(DEVICE_LABEL);
//    client.publish(topic.c_str(), payload.c_str());