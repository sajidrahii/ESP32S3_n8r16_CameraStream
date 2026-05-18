#include <WiFi.h>
#include "ESP32CameraStream.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Create camera stream object
ESP32CameraStream camera;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\nESP32 Camera Stream");
    Serial.println("===================");
    
    // Initialize camera
    if (!camera.begin()) {
        Serial.println("Camera initialization failed!");
        return;
    }
    Serial.println("Camera initialized successfully");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    
    // Display IP information
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Start the web server
    camera.startServer(WiFi.localIP().toString());
    
    // Display URLs
    Serial.println("\nCamera Stream URLs:");
    Serial.print("Video Stream: ");
    Serial.println(camera.getStreamURL());
    Serial.print("Snapshot: ");
    Serial.println(camera.getSnapshotURL());
    
    // Optional: Adjust camera settings
    // camera.setBrightness(0);
    // camera.setSaturation(0);
    // camera.setContrast(0);
    // camera.setVerticalFlip(true);
}

void loop() {
    // Nothing needed here - the server handles everything
    delay(10000);
}