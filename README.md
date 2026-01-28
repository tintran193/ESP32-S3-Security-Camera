# ESP32-S3-Security-Camera
A distributed security system utilizing two ESP32 nodes to perform remote monitoring. The system integrates ultrasonic sensing, secure cloud communication, and image processing.

Demo: https://drive.google.com/file/d/1aOzsJGSmYAez5FedNxBgtBOYVNrgWs0k/view?usp=sharing

## System Architecture
The project consists of two specialized nodes communicating via MQTT (HiveMQ Cloud):
### Node A (Sensor Hub - ESP32):
Hardware: HC-SR04 Ultrasonic Sensor, Push Button.

Function: Detects intruders within a 10cm threshold (or your threshold) or accepts manual triggers to send a "capture" command.
### Node B (Vision Processing - ESP32-S3):
Hardware: OV2640 Camera, Flash LED.

Function: Receives MQTT commands, captures high-resolution images, encodes data to Base64, and uploads via secure payloads.
## Key Technical Features

Secure Connectivity: 
Implements MQTT over SSL/TLS (Port 8884) for encrypted data transmission to HiveMQ Cloud.

DVP Interface: Configured high-speed Digital Video Port pins for stable image data acquisition from the OV2640 sensor.

Efficient Data Handling: Uses Base64 encoding to wrap binary image data into JSON payloads for easy integration with cloud dashboards or mobile apps.

State Management: Implemented a "Ready/Busy" logic to prevent memory overflow during continuous triggering.
## Installation
Libraries required: `esp_camera`, `WiFi`, `WebSocketsClient`, `MQTTPubSubClient`, `ArduinoJson.Configuration`.

Update ssid, password, and HiveMQ mqtt_server credentials in both files.
## How it Works
When an object is detected $< 10cm$ (or your threshold), Node A publishes a message to the trigger topic. Node B (subscribed to trigger) receives the message and activates the Flash LED. Node B captures the photo, converts it to a Base64 string, and sends it back to the cloud.
