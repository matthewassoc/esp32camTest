// #include <Arduino.h>
// #include <WiFi.h>
// #include <EEPROM.h> // read and write from flash memory
// #include <SPI.h>
// #include "FS.h"     // SD Card ESP32
// #include "SD_MMC.h" // SD Card ESP32
// #include "esp_camera.h"
// #define CAMERA_MODEL_AI_THINKER // Has PSRAM

// #define PWDN_GPIO_NUM 32
// #define RESET_GPIO_NUM -1
// #define XCLK_GPIO_NUM 0
// #define SIOD_GPIO_NUM 26
// #define SIOC_GPIO_NUM 27
// #define Y9_GPIO_NUM 35
// #define Y8_GPIO_NUM 34
// #define Y7_GPIO_NUM 39
// #define Y6_GPIO_NUM 36
// #define Y5_GPIO_NUM 21
// #define Y4_GPIO_NUM 19
// #define Y3_GPIO_NUM 18
// #define Y2_GPIO_NUM 5
// #define VSYNC_GPIO_NUM 25
// #define HREF_GPIO_NUM 23
// #define PCLK_GPIO_NUM 22

// #define EEPROM_SIZE 2
// #define FB_COUNT 2

// int pictureNumber = 0;
// int frameCounter = 0;

// // Replace with your network credentials
// const char *ssid = "Wassoc";
// const char *password = "2062858273";

// // Define camera settings
// camera_config_t config;

// QueueHandle_t fbQueue;

// void startCameraServer();

// void takePhotoTask(void *pvParameters)
// {
//   unsigned long durationMs = 30000;
//   Serial.println("Taking images for" + String(durationMs));
//   unsigned long start = millis();
//   while (millis() - start < durationMs)
//   {
//     camera_fb_t *fb = esp_camera_fb_get();
//     if (!fb)
//     {
//       Serial.println("Failed to capture image");
//       return;
//     }
//     xQueueSend(fbQueue, &fb, portMAX_DELAY);
//   }
//   vTaskDelete(NULL);
// }

// void takePhotoAndDoNothing(void *pvParameters)
// {
//   unsigned long durationMs = 30000;
//   Serial.println("Taking images for" + String(durationMs));
//   unsigned long start = millis();

//   while (millis() - start < durationMs)
//   {
//     camera_fb_t *fb = esp_camera_fb_get();
//     if (!fb)
//     {
//       Serial.println("Failed to capture image");
//       return;
//     }
//     esp_camera_fb_return(fb);
//     frameCounter++;
//   }
//   vTaskDelete(NULL);
// }

// void savePhotoTask(void *pvParameters)
// {
//   camera_fb_t *fbToSave = NULL;
//   Serial.println("Starting save photo task");
//   while (true)
//   {
//     if (xQueueReceive(fbQueue, &fbToSave, portMAX_DELAY) == pdPASS)
//     {
//       EEPROM.begin(EEPROM_SIZE);
//       pictureNumber = EEPROM.read(0) + 1;

//       // Path where new picture will be saved in SD Card
//       String path = "/picture" + String(pictureNumber) + ".jpg";

//       fs::FS &fs = SD_MMC;
//       Serial.printf("Picture file name: %s\n", path.c_str());

//       File file = fs.open(path.c_str(), FILE_WRITE);
//       if (!file)
//       {
//         Serial.println("Failed to open file in writing mode");
//       }
//       else
//       {
//         file.write(fbToSave->buf, fbToSave->len); // payload (image), payload length
//         Serial.printf("Saved file to path: %s\n", path.c_str());
//         EEPROM.write(0, pictureNumber);
//         EEPROM.commit();
//       }
//       file.close();
//       frameCounter++;
//       esp_camera_fb_return(fbToSave);
//     }
//   }
// }

// void saveMegaPhotoTask(void *pvParameters)
// {
//   camera_fb_t *fbToSave = NULL;
//   Serial.println("Starting save photo task");
//   // Path where new picture will be saved in SD Card
//   String path = "/megaphoto.jpg";

//   fs::FS &fs = SD_MMC;
//   Serial.printf("Picture file name: %s\n", path.c_str());

//   File file = fs.open(path.c_str(), FILE_WRITE);
//   if (!file)
//   {
//     Serial.println("Failed to open file in writing mode");
//   }
//   while (true)
//   {
//     if (xQueueReceive(fbQueue, &fbToSave, portTICK_PERIOD_MS * 1000) == pdPASS)
//     {
//       EEPROM.begin(EEPROM_SIZE);

//       file.write(fbToSave->buf, fbToSave->len); // payload (image), payload length
//       Serial.printf("Saved file to path: %s\n", path.c_str());
//       frameCounter++;
//       esp_camera_fb_return(fbToSave);
//     }
//     else
//     {
//       Serial.printf("timeout hit, closing...");
//       file.close();
//       break;
//     }
//   }
//   vTaskDelete(NULL);
// }

// void setup()
// {
//   // Start Serial Monitor
//   Serial.begin(115200);

//   fbQueue = xQueueCreate(FB_COUNT, sizeof(camera_fb_t *));

//   // Connect to Wi-Fi
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");

//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(1000);
//     Serial.print(".");
//   }

//   // Successful connection
//   Serial.println("Connected to WiFi");
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());

//   // Initialize camera
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer = LEDC_TIMER_0;
//   config.pin_d0 = Y2_GPIO_NUM;
//   config.pin_d1 = Y3_GPIO_NUM;
//   config.pin_d2 = Y4_GPIO_NUM;
//   config.pin_d3 = Y5_GPIO_NUM;
//   config.pin_d4 = Y6_GPIO_NUM;
//   config.pin_d5 = Y7_GPIO_NUM;
//   config.pin_d6 = Y8_GPIO_NUM;
//   config.pin_d7 = Y9_GPIO_NUM;
//   config.pin_xclk = XCLK_GPIO_NUM;
//   config.pin_pclk = PCLK_GPIO_NUM;
//   config.pin_vsync = VSYNC_GPIO_NUM;
//   config.pin_href = HREF_GPIO_NUM;
//   config.pin_sscb_sda = SIOD_GPIO_NUM;
//   config.pin_sscb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn = PWDN_GPIO_NUM;
//   config.pin_reset = RESET_GPIO_NUM;
//   config.xclk_freq_hz = 20000000;
//   config.pixel_format = PIXFORMAT_JPEG;
//   // config.pixel_format = PIXFORMAT_RGB565;
//   // config.pixel_format = PIXFORMAT_YUV422;
//   // config.pixel_format = PIXFORMAT_GRAYSCALE;
//   // config.frame_size = FRAMESIZE_HD; // You can choose a different frame size if necessary
//   config.frame_size = FRAMESIZE_SVGA; // You can choose a different frame size if necessary
//   config.jpeg_quality = 40;           // Lower values = higher quality (default is 12)
//   config.fb_count = FB_COUNT;         // Number of frame buffers (1 or 2)

//   // Init the camera
//   if (esp_camera_init(&config) != ESP_OK)
//   {
//     Serial.println("Camera initialization failed");
//     return;
//   }

//   if (!SD_MMC.begin())
//   {
//     Serial.println("SD Card Mount Failed");
//     return;
//   }

//   uint8_t cardType = SD_MMC.cardType();
//   if (cardType == CARD_NONE)
//   {
//     Serial.println("No SD Card attached");
//     return;
//   }

//   Serial.println("Camera initialized");

//   // Create the consumer task pinned to core 1
//   // xTaskCreatePinnedToCore(savePhotoTask, "Consumer", 10000, NULL, 1, NULL, 1);
//   // xTaskCreatePinnedToCore(saveMegaPhotoTask, "Consumer", 10000, NULL, 1, NULL, 1);

//   Serial.println("taking video for 30 seconds");
//   unsigned long timeoutMs = 30000;
//   // Create the producer task pinned to core 0
//   // xTaskCreatePinnedToCore(takePhotoTask, "Producer", 10000, (void *)&timeoutMs, 1, NULL, 0);
//   xTaskCreatePinnedToCore(takePhotoAndDoNothing, "Producer", 100000, NULL, 1, NULL, 0);
//   // Start the camera server
//   // startCameraServer();
//   delay(31000);
//   Serial.println("Took photos in 30 seconds: " + String(frameCounter));
// }

// void loop()
// {
//   delay(5000);
//   // Nothing to do here, camera server is handling the loop
// }

// // Function to stream video
// void StreamVideo(WiFiClient &client)
// {
//   client.println("HTTP/1.1 200 OK");
//   client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");

//   while (true)
//   {
//     // Capture a frame from the camera
//     camera_fb_t *fb = esp_camera_fb_get();
//     if (!fb)
//     {
//       Serial.println("Failed to capture image");
//       return;
//     }

//     // Send the image data
//     client.println();
//     client.print("--frame\r\n");
//     client.print("Content-Type: image/jpeg\r\n");
//     client.print("Content-Length: " + String(fb->len) + "\r\n\r\n");
//     client.write(fb->buf, fb->len); // send image data
//     client.println();

//     // Return the buffer back to the driver
//     esp_camera_fb_return(fb);

//     // Delay between frames
//     delay(30);
//   }
// }

// void takePicture()
// {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb)
//   {
//     Serial.println("Failed to capture image");
//     return;
//   }

//   EEPROM.begin(EEPROM_SIZE);
//   pictureNumber = EEPROM.read(0) + 1;

//   // Path where new picture will be saved in SD Card
//   String path = "/picture" + String(pictureNumber) + ".jpg";

//   fs::FS &fs = SD_MMC;
//   Serial.printf("Picture file name: %s\n", path.c_str());

//   File file = fs.open(path.c_str(), FILE_WRITE);
//   if (!file)
//   {
//     Serial.println("Failed to open file in writing mode");
//   }
//   else
//   {
//     file.write(fb->buf, fb->len); // payload (image), payload length
//     Serial.printf("Saved file to path: %s\n", path.c_str());
//     EEPROM.write(0, pictureNumber);
//     EEPROM.commit();
//   }
//   file.close();
//   esp_camera_fb_return(fb);

//   // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
//   // pinMode(4, OUTPUT);
//   // digitalWrite(4, LOW);
// }

// // Function to start the camera server
// void startCameraServer()
// {
//   // Create the web server to stream video
//   WiFiServer server(80);
//   server.begin();

//   Serial.println("Web Server Started");

//   while (true)
//   {
//     WiFiClient client = server.available();

//     if (!client)
//     {
//       delay(100);
//     }
//     else
//     {
//       String req = client.readStringUntil('\r');
//       client.flush();
//       Serial.println("got req" + req);

//       if (req.indexOf("/stream") != -1)
//       {
//         Serial.println("starting stream");
//         // Stream video data
//         StreamVideo(client);
//       }
//       else if (req.indexOf("/takePhoto") != -1)
//       {
//         Serial.println("returning takePhoto html");
//         client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
//         client.print("<html><body><h1>ESP32-CAM</h1><button id='photo'>Take photo</button><button id='video'>Take video</button><script>document.getElementById('photo').addEventListener('click', () => {fetch('/photo')});document.getElementById('video').addEventListener('click', () => {fetch('/video')});</script></body></html>");
//       }
//       else if (req.indexOf("/photo") != -1)
//       {
//         Serial.println("saving photo");
//         takePicture();
//       }
//       else if (req.indexOf("/video") != -1)
//       {
//         Serial.println("taking video for 30 seconds");
//         unsigned long timeoutMs = 30000;
//         // Create the producer task pinned to core 0
//         xTaskCreatePinnedToCore(takePhotoTask, "Producer", 10000, (void *)&timeoutMs, 1, NULL, 0);
//         Serial.println("done recording video");
//       }
//       else
//       {
//         Serial.println("returning default page");
//         // Serve a basic page or default response
//         client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
//         client.print("<html><body><h1>ESP32-CAM</h1><a href='/stream'>Start Stream</a></body></html>");
//       }
//       delay(1);
//       client.stop();
//     }
//   }
// }
