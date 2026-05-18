#include "ESP32S3CameraStream.h"

// Pin definitions for ESP32-S3-EYE
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM    4
#define SIOC_GPIO_NUM    5
#define Y2_GPIO_NUM      11
#define Y3_GPIO_NUM      9
#define Y4_GPIO_NUM      8
#define Y5_GPIO_NUM      10
#define Y6_GPIO_NUM      12
#define Y7_GPIO_NUM      18
#define Y8_GPIO_NUM      17
#define Y9_GPIO_NUM      16
#define VSYNC_GPIO_NUM   6
#define HREF_GPIO_NUM    7
#define PCLK_GPIO_NUM    13

ESP32CameraStream::ESP32CameraStream() {
    server = NULL;
    cameraInitialized = false;
    sensor = NULL;
}

bool ESP32CameraStream::begin() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;
    config.frame_size = FRAMESIZE_SVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    
    if(psramFound()){
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
        Serial.println("PSRAM FOUND - Optimized settings");
    } else {
        config.frame_size = FRAMESIZE_HVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("NO PSRAM - Using lower resolution");
    }

    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        cameraInitialized = false;
        return false;
    }
    
    sensor = esp_camera_sensor_get();
    cameraInitialized = true;
    
    // Apply default settings
    setVerticalFlip(true);
    setBrightness(1);
    setSaturation(-1);
    
    return true;
}

void ESP32CameraStream::setVerticalFlip(bool flip) {
    if (sensor) sensor->set_vflip(sensor, flip);
}

void ESP32CameraStream::setHorizontalMirror(bool mirror) {
    if (sensor) sensor->set_hmirror(sensor, mirror);
}

void ESP32CameraStream::setBrightness(int level) {
    if (sensor && level >= -2 && level <= 2) {
        sensor->set_brightness(sensor, level);
    }
}

void ESP32CameraStream::setSaturation(int level) {
    if (sensor && level >= -2 && level <= 2) {
        sensor->set_saturation(sensor, level);
    }
}

void ESP32CameraStream::setContrast(int level) {
    if (sensor && level >= -2 && level <= 2) {
        sensor->set_contrast(sensor, level);
    }
}

String ESP32CameraStream::getLocalIP() {
    return localIP;
}

String ESP32CameraStream::getStreamURL() {
    return "http://" + localIP + "/stream";
}

String ESP32CameraStream::getSnapshotURL() {
    return "http://" + localIP + "/jpg";
}

// Static handler implementations
esp_err_t ESP32CameraStream::jpg_handler(httpd_req_t *req) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    return ESP_OK;
}

esp_err_t ESP32CameraStream::stream_handler(httpd_req_t *req) {
    static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

    while (true) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        char part_buf[64];
        size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);

        if (httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)) != ESP_OK ||
            httpd_resp_send_chunk(req, part_buf, hlen) != ESP_OK ||
            httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len) != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        esp_camera_fb_return(fb);
    }

    return ESP_OK;
}

void ESP32CameraStream::startServer(String ip) {
    if (!cameraInitialized) {
        Serial.println("Camera not initialized. Call begin() first.");
        return;
    }
    
    localIP = ip;
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_handler_stream = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = stream_handler,
            .user_ctx  = NULL
        };
        
        httpd_uri_t uri_handler_jpg = {
            .uri       = "/jpg",
            .method    = HTTP_GET,
            .handler   = jpg_handler,
            .user_ctx  = NULL
        };
        
        httpd_register_uri_handler(server, &uri_handler_stream);
        httpd_register_uri_handler(server, &uri_handler_jpg);
        Serial.println("Camera server started");
    } else {
        Serial.println("Failed to start server");
    }
}