//BOARD A
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <MQTTPubSubClient.h>
//WIFI and MQTT
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR_HIVEMQ_CLUSTER_URL";
///BUTTON
#define BUTTON_PIN 14
///
//HC SR-04 PIN
#define TRIG_PIN 12
#define ECHO_PIN 13
#define THRESHOLD 10 //10 cm
#define TimeCapture 10000 //10sec

unsigned long lastTrigger = 0;

WebSocketsClient client;
MQTTPubSubClient mqtt;

void setup() {
  Serial.begin(115200);
  //BUTTON
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Sử dụng trở kéo lên nội bộ
  //
  //HC SR-04 PIN MODE
  pinMode(TRIG_PIN, OUTPUT); // Chân Trig phát tín hiệu
  pinMode(ECHO_PIN, INPUT);  // Chân Echo nhận tín hiệu
  //WIFI and MQTT Connecting
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500); 
  }
  Serial.println("\nBoard A: Wifi Connected!");

  client.beginSSL(mqtt_server, 8884, "/mqtt");
  mqtt.begin(client);
  Serial.print("\nConnecting to MQTT broker...");
  while (!mqtt.connect("ESP32_A", "YOUR_MQTT_USER", "YOUR_MQTT_PASS")) { 
    Serial.print(".");
    delay(500);  
  }
  Serial.println("\nBoard A: MQTT Connected!");

  //Led initial state
  mqtt.publish("led","{\"led1\": true,\"led2\":false}");//Green on, Red off
}

void loop() {
  client.loop();
  mqtt.update();
  //BUTTON
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);
  //
  //Trig
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  //Echo Response (micro sec)
  long duration = pulseIn(ECHO_PIN, HIGH);
  //Tính khoảng cách (cm)
  float distance = duration * 0.034 / 2;

  // Press BUTTON
  if (lastState == HIGH && currentState == LOW) {
    Serial.println("Button is activated! Capturing...");
    
    //Sending to Board B
    if(mqtt.publish("trigger", "capture")) {
      Serial.println("Sending successfully!");
    }
    
    delay(500); //Debounce
  }
  lastState = currentState;

  if (distance > 0 && distance < THRESHOLD && (millis() - lastTrigger > TimeCapture)) {//10 sec
    Serial.printf("Distance: %.2f cm. Sending photo ...\n", distance);
    mqtt.publish("trigger", "capture");
    //
    mqtt.publish("led","{\"led1\": false,\"led2\":true}");//Green off, Red on
    //
    lastTrigger = millis();
  }

  static bool waiting = false;
  
  if (millis() - lastTrigger > TimeCapture) {
      if (waiting == true) {
          mqtt.publish("led", "{\"led1\": true, \"led2\": false}"); //Green on, Red off
          waiting = false; 
      }
  } else {
      waiting = true;
  }

  delay(100);
}
