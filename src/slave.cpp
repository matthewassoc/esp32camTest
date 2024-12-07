#include <ESP32SPISlave.h>
#include <WiFi.h>
#include "helper.h"
#include "esp_task_wdt.h"

void startCameraServer(void *pvParameters);
void StreamVideo(WiFiClient &client);
void slaveTask(void *params);

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 32;
static constexpr size_t QUEUE_SIZE = 500;

uint8_t tx_buf[1] = {0};
uint8_t rx_buf_1[BUFFER_SIZE] = {0};
uint8_t rx_buf_2[BUFFER_SIZE] = {0};

const char *ssid = "Wassoc";
const char *password = "2062858273";

unsigned int i = 0;
unsigned int imageLen = 0;
unsigned int incomingImageLen = 0;
unsigned int transactionsToQueue = 0;

uint8_t *receiveBuf1 = (uint8_t *)calloc(30000, sizeof(uint8_t));
uint8_t *receiveBuf2 = (uint8_t *)calloc(30000, sizeof(uint8_t));
uint8_t *imageBuf = receiveBuf1;
uint8_t *writeBuf = receiveBuf2;

void setup()
{
  esp_task_wdt_init(100, true); // 5 seconds timeout
  esp_task_wdt_deinit();        // This doesn't appear to work
  Serial.begin(115200);

  delay(2000);

  slave.setDataMode(SPI_MODE0);   // default: SPI_MODE0
  slave.setQueueSize(QUEUE_SIZE); // default: 1

  // begin() after setting
  slave.begin(VSPI); // default: HSPI (please refer README for pin assignments)

  xTaskCreatePinnedToCore(startCameraServer, "Producer", 16384, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(slaveTask, "SlaveBoi", 16384, NULL, 1, NULL, 0);

  Serial.println("start spi slave");
}

void loop()
{
  delay(1000);
}

void slaveTask(void *params)
{
  unsigned int transactionsToQueue = 0;
  unsigned int incomingImageLen = 0;
  uint8_t imageTransactionLengthBuf[BUFFER_SIZE] = {0};
  uint8_t emptyBuf[BUFFER_SIZE] = {0};
  while (true)
  {
    if (slave.hasTransactionsCompletedAndAllResultsHandled())
    {

      // printf("ready to receive image size message\n");
      initializeBuffer(imageTransactionLengthBuf, BUFFER_SIZE);
      while (imageTransactionLengthBuf[BUFFER_SIZE - 1] != 255)
      {
        slave.transfer(tx_buf, imageTransactionLengthBuf, BUFFER_SIZE);
        for (int k = 0; k < BUFFER_SIZE; k++)
        {
          printf("%02X ", imageTransactionLengthBuf[k]);
        }
        printf("\n");
      }
      slave.transfer(imageTransactionLengthBuf, emptyBuf, BUFFER_SIZE);
      transactionsToQueue = 0;
      incomingImageLen = 0;
      // for (int k = 0; k < BUFFER_SIZE; k++)
      // {
      //   printf("%02X ", imageTransactionLengthBuf[k]);
      // }

      incomingImageLen |= (imageTransactionLengthBuf[0] << (8 * 3)) & 0xff000000;
      incomingImageLen |= (imageTransactionLengthBuf[1] << (8 * 2)) & 0xff0000;
      incomingImageLen |= (imageTransactionLengthBuf[2] << (8 * 1)) & 0xff00;
      incomingImageLen |= imageTransactionLengthBuf[3] << (8 * 0);

      transactionsToQueue |= (imageTransactionLengthBuf[4] << (8 * 3)) & 0xff000000;
      transactionsToQueue |= (imageTransactionLengthBuf[5] << (8 * 2)) & 0xff0000;
      transactionsToQueue |= (imageTransactionLengthBuf[6] << (8 * 1)) & 0xff00;
      transactionsToQueue |= imageTransactionLengthBuf[7] << (8 * 0);

      printf("incoming image len: %d\n", incomingImageLen);
      if (transactionsToQueue > 0 && transactionsToQueue < 2000)
      {
        printf("initializing %d transactions\n", transactionsToQueue);
        if (imageBuf == receiveBuf1)
        {
          writeBuf = receiveBuf2;
        }
        else
        {
          writeBuf = receiveBuf1;
        }
        for (int i = 0; i < transactionsToQueue; i++)
        {
          slave.queue(tx_buf, writeBuf + (i * BUFFER_SIZE), BUFFER_SIZE);
        }

        // finally, we should trigger transaction in the background
        slave.trigger();
      }
      else
      {
        printf("Did not queue\n");
      }
    }

    // you can do some other stuff here
    // NOTE: you can't touch dma_tx/rx_buf because it's in-flight in the background
    // if all transactions are completed and all results are ready, handle results
    if (slave.hasTransactionsCompletedAndAllResultsReady(transactionsToQueue))
    {
      printf("transaction complete\n");
      imageBuf = writeBuf;
      // get received bytes for all transactions
      const std::vector<size_t> received_bytes = slave.numBytesReceivedAll();
      size_t totalBytes = 0;
      unsigned int totalReceived = 0;
      // printf("\nbytes per message:");
      int size = received_bytes.size();
      // for (size_t bytes : received_bytes)
      // {
      //   printf(" %d", bytes);
      //   totalBytes += bytes;
      //   totalReceived++;
      // }
      imageLen = incomingImageLen;
      bool imageIsBuf1 = receiveBuf1 == imageBuf;

      // unsigned int sum = 0;
      // for (int j = 0; j < imageLen; j++)
      // {
      //   sum += imageBuf[j];
      // }

      // printf("\ngot sum: %d", sum);
      // printf("\ngot total bytes: %d\n", totalBytes);
      // printf("\nimage is buf1: %s\n", imageIsBuf1 ? "True" : "False");
      // printf("got total messages: %d\n", totalReceived);
      // printf("messages in flight: %d\n", slave.numTransactionsInFlight());
      // printf("messages completed: %d\n", slave.numTransactionsCompleted());
      // printf("is ready for next message %d\n", slave.hasTransactionsCompletedAndAllResultsHandled() ? 1 : 0);
    }
  }
}

// Function to start the camera server
void startCameraServer(void *pvParameters)
{
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  // Successful connection
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  // Create the web server to stream video
  WiFiServer server(80);
  server.begin();

  Serial.println("Web Server Started");
  while (true)
  {
    WiFiClient client = server.available();

    if (!client)
    {
      delay(100);
    }
    else
    {
      String req = client.readStringUntil('\r');
      printf("got client %s", client.readString());
      client.flush();
      Serial.println("got req" + req);

      Serial.println("starting stream");
      // Stream video data
      StreamVideo(client);

      // if (req.indexOf("/stream") != -1)
      // {
      //   Serial.println("starting stream");
      //   // Stream video data
      //   StreamVideo(client);
      // }
      // else
      // {
      //   Serial.println("returning default page");
      //   // Serve a basic page or default response
      //   client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      //   client.print("<html><body><h1>ESP32-CAM</h1><a href='/stream'>Start Stream</a></body></html>");
      // }
      delay(1);
      client.stop();
    }
  }
}

// Function to stream video
void StreamVideo(WiFiClient &client)
{
  printf("Using buf %s\n", imageBuf == receiveBuf1 ? "1" : "2");
  printf("Len %d\n", imageLen);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");

  while (true)
  {
    printf("writing to client\n");
    // Capture a frame from the camera

    // Send the image data
    client.println();
    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n");
    client.print("Content-Length: " + String(imageLen) + "\r\n\r\n");
    client.write(imageBuf, imageLen); // send image data
    client.println();

    // Delay between frames
    delay(5000);
  }
}