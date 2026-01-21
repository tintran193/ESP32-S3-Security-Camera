//BOARD B
#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <MQTTPubSubClient.h>
#include <base64.h>
#include <time.h>
#include <ArduinoJson.h> 

// LED PIN
#define ledPin1 2 //Green
#define ledPin2 20 //Red

//ESP32-S3-CAM CAMERA PIN Configuration
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5
#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM     13

#define FLASH_LED_PIN     21 //Flash

//WIFI and MQTT
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR_HIVEMQ_CLUSTER_URL";

WebSocketsClient client;
MQTTPubSubClient mqtt;

// Get Time
String getTimeNow() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "Unknown Time";
  char buf[25];
  strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", &timeinfo);
  return String(buf);
}
//Capture and Send FUnc
void captureAndSend() {
  Serial.println("Activate the Camera...");
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(500);

  camera_fb_t * fb = esp_camera_fb_get(); //Capture
  if (!fb) {
    Serial.println("Error,cannot capture photo!");
    digitalWrite(FLASH_LED_PIN, LOW);
    return;
  }

  Serial.println("Changing to Base64...");
  
  String imagedata = base64::encode(fb->buf, fb->len);
  String timedata = getTimeNow();
  String payload = "{\"time\":\"" + timedata + "\", \"image\":\"" + imagedata + "\"}";
  
  //Send to Broker
  Serial.print("Sending (Size: "); 
  Serial.print(imagedata.length()); 
  Serial.println(" bytes)...");

  //Publish
  if (mqtt.publish("photo", payload)) {
    Serial.println("Photo successfully sent :>");
  } else {
    Serial.println("Failed to send photo :<");
  }

  esp_camera_fb_return(fb); // Free
  digitalWrite(FLASH_LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_LED_PIN, OUTPUT);
  while (!Serial) delay(10);
  //Led
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  //Camera Configuration 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG; // Định dạng ảnh nén
  config.frame_size = FRAMESIZE_QVGA;    // Độ phân giải 320x240 (Nhẹ để gửi MQTT)
  config.jpeg_quality = 12;              // 0-63 (số càng thấp ảnh càng nét nhưng nặng)
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed!");
    return;
  }
  //Time configuration 
  configTime(7 * 3600, 0, "pool.ntp.org");

  //WIFI and MQTT Connecting
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500); 
  }
  Serial.println("\nBoard B: Wifi Connected!");

  client.beginSSL(mqtt_server, 8884, "/mqtt");
  mqtt.begin(client);
  Serial.print("\nConnecting to MQTT broker...");
  while (!mqtt.connect("ESP32_Camera_B", "YOUR_MQTT_USER", "YOUR_MQTT_PASS")) { 
    Serial.print(".");
    delay(500);  
  }
  Serial.println("\nBoard B: MQTT Connected!");

  //Subscribe
  mqtt.subscribe("trigger", [](const String& payload, const size_t size) {
    Serial.println("Receive: " + payload);
    if (payload.indexOf("capture") >= 0) {
      captureAndSend();
    }
  });

  mqtt.subscribe("led",[](const String& payload, const size_t size) {
    Serial.println("Led status" + payload);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("JSON Error: "));
      Serial.println(error.f_str());
      return;
    }

    if (doc.containsKey("led1")) {
      bool ledState = doc["led1"];
      digitalWrite(ledPin1, ledState);
      mqtt.publish("led1_status", String(digitalRead(ledPin1)));
    }
    if (doc.containsKey("led2")) {
      bool ledState = doc["led2"];
      digitalWrite(ledPin2, ledState);
      mqtt.publish("led2_status", String(digitalRead(ledPin2)));
    }
  });
}

void loop() {
  client.loop();
  mqtt.update();
}