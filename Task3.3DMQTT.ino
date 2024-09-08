#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "OnePlus";
const char* password = "pt123456789";

// MQTT broker details
const char* mqtt_server = "t82fb668.ala.dedicated.aws.emqxcloud.com";
const int mqtt_port = 1883;
const char* mqtt_user = "priyanshu";    // MQTT Username
const char* mqtt_pass = "priyanshu";    // MQTT Password
const char* mqtt_topic_wave = "SIT210/wave";
const char* mqtt_topic_pat = "SIT210/pat";

// Ultrasonic sensor pins
const int trigPin = 2;
const int echoPin = 3;

// LED pin
const int ledPin = 4;

// Debounce variables
bool messageSent = false;
unsigned long lastDebounceTime = 0;
const long debounceDelay = 1000; // 1 second debounce delay

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    Serial.println("MQTT Disconnected, reconnecting...");
    connectMQTT();
  }
  client.loop();

  // Send trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse
  long duration = pulseIn(echoPin, HIGH);
  long distance = (duration / 2) / 29.1; // it calculates distance of object based on the duration of echo pulse

  Serial.print("Distance: ");
  Serial.println(distance);

  // Only publish if the distance is less than 10 cm and not sent yet
  if (distance < 10 && distance > 0 && !messageSent) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      Serial.println("Publishing message: Priyanshu");
      client.publish(mqtt_topic_wave, "Priyanshu");

      // Flash LED 3 times for wave detection
      for (int i = 0; i < 3; i++) {
        digitalWrite(ledPin, HIGH);
        delay(500);
        digitalWrite(ledPin, LOW);
        delay(500);
      }

      messageSent = true;  // Mark message as sent
      lastDebounceTime = currentTime;  // Update the debounce time
    }
  } else if (distance >= 10 || distance == 0) {
    messageSent = false;  // it resets the message state if the distance is >= 10 cm or invalid
  }

  delay(1000);  // adds small delay to avoid spamming the Serial Monitor
}

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.disconnect();  // it ensures the WiFi module is reset before connecting
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 30) {  // Attempt for about 30 times and send message after that
      Serial.println("Failed to connect to WiFi. Check credentials or signal strength.");
      return;
    }
  }
  Serial.println("Connected to WiFi!");
}

void connectMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!client.connected()) {
    if (client.connect("ArduinoClient", mqtt_user, mqtt_pass)) {
      Serial.println("Connected to MQTT!");
      client.subscribe(mqtt_topic_pat);  // Subscribe to the "pat" topic to make led blink differently
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received: ");
  String message;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  // Check if the received message is for the "pat" topic
  if (String(topic) == mqtt_topic_pat) {
    // Flash LED 5 times for "pat"
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
    }
  }
}
