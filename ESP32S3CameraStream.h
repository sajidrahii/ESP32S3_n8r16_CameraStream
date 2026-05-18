#ifndef ESP32_CAMERA_STREAM_H
#define ESP32_CAMERA_STREAM_H

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

class ESP32CameraStream {
public:
    ESP32CameraStream();
    
    // Initialize camera with default ESP32-S3-EYE pins
    bool begin();
    
    // Start the web server (call after WiFi is connected)
    void startServer(String ip);
    
    // Get the local IP address
    String getLocalIP();
    
    // Get the stream URL
    String getStreamURL();
    
    // Get the snapshot URL
    String getSnapshotURL();
    
    // Optional: Configure camera settings
    void setBrightness(int level);    // -2 to 2
    void setSaturation(int level);    // -2 to 2
    void setContrast(int level);      // -2 to 2
    void setVerticalFlip(bool flip);
    void setHorizontalMirror(bool mirror);
    
private:
    httpd_handle_t server;
    sensor_t* sensor;
    bool cameraInitialized;
    String localIP;
    
    static esp_err_t jpg_handler(httpd_req_t *req);
    static esp_err_t stream_handler(httpd_req_t *req);
};

#endif