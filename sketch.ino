#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const int trigPin = 12;  // Pin Trigger sensor ultrasonik
const int echoPin = 14;  // Pin Echo sensor ultrasonik
const int ledPin = 13;   // Pin LED
const int servoPin = 15; // Pin motor servo
const int buzzerPin = 4; // Pin buzzer

Servo myservo;

bool personDetected = false;
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 1000; // Duration for the buzzer to sound in milliseconds

// WiFi and MQTT configuration
const char* ssid = "Wokwi-GUEST";  // Replace with your WiFi SSID
const char* password = "";  // Replace with your WiFi password
const char* mqtt_server = "public.mqtthq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "undip/tekkom/tugas/akhir/data";  // Replace with your MQTT topic
const char* lwt_message = "{\"status\":\"ESP32 disconnected\"}";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Set the Last Will and Testament (LWT) message
    if (client.connect("ESP32Client", mqtt_topic, 0, true, lwt_message)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  myservo.attach(servoPin);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long duration, distance;

  // Mengirim sinyal trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Menerima sinyal echo
  duration = pulseIn(echoPin, HIGH);

  // Menghitung jarak berdasarkan waktu echo
  distance = (duration * 0.0343) / 2;

  Serial.print("Jarak: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Create JSON document
  StaticJsonDocument<200> jsonDoc;
  char distanceStr[16];
  snprintf(distanceStr, sizeof(distanceStr), "%ld cm", distance);
  jsonDoc["distance"] = distanceStr;
  const char* statusMessage;

  // Atur kondisi untuk mengaktifkan servo, LED, dan buzzer
  if (distance <= 200) {
    myservo.write(90);   // Posisi tengah untuk motor servo
    digitalWrite(ledPin, HIGH);  // Nyalakan LED

    if (!personDetected) {
      personDetected = true;
      buzzerStartTime = millis();
      tone(buzzerPin, 1000); // Start the buzzer
      Serial.println("Ada pengunjung Mendekat");
      statusMessage = "ada pengunjung Mendekat";
    } else {
      statusMessage = "ada pengunjung Mendekat";
    }

    // Turn off the buzzer after the duration has passed
    if (millis() - buzzerStartTime >= buzzerDuration) {
      noTone(buzzerPin);
    }
  } else {
    myservo.write(0);    // Posisi awal untuk motor servo
    digitalWrite(ledPin, LOW);   // Matikan LED
    noTone(buzzerPin);
    if (personDetected) {
      personDetected = false;
      Serial.println("Tidak ada pengunjung mendekat");
      statusMessage = "tidak ada pengunjung masuk";
    } else {
      statusMessage = "tidak ada pengunjung masuk";
    }
  }

  jsonDoc["status"] = statusMessage;

  // Serialize JSON document to string
  char jsonBuffer[256];
  serializeJson(jsonDoc, jsonBuffer);

  // Publish JSON to MQTT
  client.publish(mqtt_topic, jsonBuffer);

  delay(1000); // Tunggu sebentar sebelum membaca ulang sensor
}
