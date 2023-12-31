#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <Adafruit_Sensor.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SimpleTimer.h>

SimpleTimer timer;

float calibration_value = 21.34 - 0.7 + 14.7;
int phval = 0;
unsigned long int avgval;
int buffer_arr[10], temp;

float ph_act;

char ssid[] = "Sidhant";
char pass[] = "Sid@1234";
char broker[] = "broker.hivemq.com";  // Get this from HiveMQ Cloud
const char topic1[] = "WaterLevel";
const char topic2[] = "DHT";
const char topic3[] = "Ph";
const char *HOST="api.thingspeak.com";
String api_key = "XUT443RMSHZ08YYY";

WiFiClient wifiClient;
MqttClient client(wifiClient);

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int waterSensorPin = A0;  // Assuming you have an analog water level sensor

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  dht.begin();

  // Connecting to broker
  if (!client.connect(broker, 1883)) {
    Serial.print("MQTT CONNECTION FAILED");
    Serial.println(client.connectError());
  }
}

void loop() {
  client.poll();
  int waterLevel = analogRead(waterSensorPin);
  float temperature = dht.readTemperature();
  if (client.connected()) {
    if (client.beginMessage(topic1)) {


      Serial.println("Published Water Level: " + String(waterLevel));
      //  client.println(temperature);
      client.println(waterLevel);
      client.endMessage();
    } else {
      Serial.println("Publish water level failed");
    }

    if (client.beginMessage(topic2)) {


      Serial.println("Published Temperature: " + String(temperature));

      client.println(temperature);
      client.endMessage();

    } else {
      Serial.println("Publish DHT failed");
    }

    if (client.beginMessage(topic3)) {
      timer.run();  // Initiates SimpleTimer
      for (int i = 0; i < 10; i++) {
        buffer_arr[i] = analogRead(A1);
        delay(30);
      }
      for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
          if (buffer_arr[i] > buffer_arr[j]) {
            temp = buffer_arr[i];
            buffer_arr[i] = buffer_arr[j];
            buffer_arr[j] = temp;
          }
        }
      }
      avgval = 0;
      for (int i = 2; i < 8; i++)
        avgval += buffer_arr[i];
      float volt = (float)avgval * 5.0 / 1024 / 6;
      ph_act = -5.70 * volt + calibration_value;

      Serial.print("Published pH_Val: ");
      Serial.println(ph_act);

      client.print(ph_act);
      client.endMessage();
    } else {
      Serial.println("Publish failed");
    }
    delay(1000);  // Publish every 2 seconds (adjust as needed)
  }
  if (isnan(temperature) || isnan(waterLevel) || isnan(ph_act)) {
    Serial.println(F("Failed to read data from DHT22 Sensor!!"));
    return;
  }


  Serial.print(F(" Temperature: "));
  Serial.print(temperature);
  Serial.print(F(".C"));
  Serial.print(F(" Water Level: "));
  Serial.print(waterLevel);


  Send_Data(temperature,waterLevel,ph_act);
  delay(1000);
}

void Send_Data(float t, int WaterLevel,float ph) {
  Serial.println("\nPrepare to Send Data");
  WiFiClient client;
  const int httpPort = 80;

  if (!client.connect(HOST, httpPort)) {
    Serial.println("Connection Failed");
    return;
  } else {
    String data_to_send = "api_key=" + api_key;
    data_to_send += "&field1=";
    data_to_send += String(WaterLevel);
    data_to_send += "&field2=";
    data_to_send += String(t);
      data_to_send += "&field3=";
    data_to_send += String(ph);
    data_to_send += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);
    delay(1000);
  }
  client.stop();
}
