/* ESP32 HTTP IoT Server Example for Wokwi.com

  https://wokwi.com/projects/320964045035274834

  To test, you need the Wokwi IoT Gateway, as explained here:

  https://docs.wokwi.com/guides/esp32-wifi#the-private-gateway

  Then start the simulation, and open http://localhost:9080
  in another browser tab.

  Note that the IoT Gateway requires a Wokwi Club subscription.
  To purchase a Wokwi Club subscription, go to https://wokwi.com/club
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#define LED_GREEN GPIO_NUM_27
#define LED_RED   GPIO_NUM_26
#define BUTTON_PIN GPIO_NUM_18
#define POT_ADC_CHANNEL ADC1_CHANNEL_6 // GPIO34

#define MAX_COUNT_SEM 10 

// 3276 is about 80% of the max value
// It will be the critical threshold for "radiation levels"
#define SENSOR_THRESHOLD 3276

// Handles for semaphores and mutex - you'll initialize these in the main program
SemaphoreHandle_t sem_button;
SemaphoreHandle_t sem_sensor; // Recheck this for counting
SemaphoreHandle_t print_mutex;

volatile int SEMCNT = 0; //You may not use this value in your logic -- but you can print it if you wish


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

WebServer server(80);

const int LED1 = 26;
const int LED2 = 27;

bool led1State = false;
bool led2State = false;

void sendHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>Alarm</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
        </div>
      </body>
    </html>
  )";
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

void system_task(void *pvParameters) { // System Heartbeat Task
    bool led_on = false;
    TickType_t currentTime = pdTICKS_TO_MS( xTaskGetTickCount() );

    while (1) {
      currentTime = pdTICKS_TO_MS( xTaskGetTickCount() );
      gpio_set_level(LED_GREEN, led_on); // set LED state to bool led_on
      led_on = !led_on;  // toggle state for next time

      //printf("Status LED %s @ %lu \n", led_on ? "ON" : "OFF", currentTime);
      
      //printf("Status LED @ %lu \n", currentTime); //Periodic Timing
      
      vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms using MS to Ticks Function vs alternative which is MS / ticks per ms

    }
}

void sensor_task(void *pvParameters) {

    const TickType_t periodTicks = pdMS_TO_TICKS(17); // e.g. 17 ms period
    TickType_t lastWakeTime = xTaskGetTickCount(); // initialize last wake time

    static int last_val = -1;  // Ask ChatGPT to make it only print on change 

    while (1) {
        int val = adc1_get_raw(POT_ADC_CHANNEL);

        //Add serial print to log the raw sensor value (mutex protected)
        //Hint: use xSemaphoreTake( ... which semaphore ...) and printf
        if (val != last_val) {
          if (xSemaphoreTake(print_mutex, portMAX_DELAY) == pdTRUE) { // Takes Mutex
          
          //May comment this printf out because it filled the console
          printf("Radiation Level is %d [%.2f%%]!\n", val, ((float)val / 4095) * 100);

          xSemaphoreGive(print_mutex); // Returns Mutex
          }
          last_val = val;  // update last printed value
        }

        //printf("Value: %d\n", val);        

        //: prevent spamming by only signaling on rising edge; See prior application #3 for help!
        static bool prev_above = false;  // remembers if the last reading was above threshold
        bool curr_above = (val > SENSOR_THRESHOLD);

        if (curr_above && !prev_above) {
            // Rising edge detected: value just crossed the threshold upward{
            if(SEMCNT < MAX_COUNT_SEM+1) SEMCNT++; // DO NOT REMOVE THIS LINE
            
            //printf("Radiation Level is %d [%.2f%%]!\n", val, ((float)val / 4095) * 100);
            
            xSemaphoreGive(sem_sensor);  // Signal sensor event

        }
        // Update previous state for next iteration
        prev_above = curr_above;

        vTaskDelayUntil(&lastWakeTime, periodTicks);
        //vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void button_task(void *pvParameters) {

   static bool previous_state = false;

    while (1) {
        int state = gpio_get_level(BUTTON_PIN);
        bool current_state = (state == 0); // active-low button
        // TODO 4a: Add addtional logic to prevent bounce effect (ignore multiple events for 'single press')
        // You must do it in code - not by modifying the wokwi simulator button

        if (current_state && !previous_state) {
            xSemaphoreGive(sem_button);

          if (xSemaphoreTake(print_mutex, portMAX_DELAY) == pdTRUE) {
              printf("Button Press Detected! \n");
              xSemaphoreGive(print_mutex);
          }

            // optional debounce delay of arbitrary time
            vTaskDelay(pdMS_TO_TICKS(200)); //Comment this to break the system
        }

        //Comment this to break the system
        previous_state = current_state; // update previous state
            
        //Comment this to break the system
        vTaskDelay(pdMS_TO_TICKS(10)); // Do Not Modify This Delay!
    }
}

void event_handler_task(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(sem_sensor, 0)) {
            SEMCNT--;  // DO NOT MODIFY THIS LINE

            xSemaphoreTake(print_mutex, portMAX_DELAY);
            printf("Threshold Exceeds Safe Levels!\n");
            xSemaphoreGive(print_mutex);

            gpio_set_level(LED_RED, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_RED, 0);
        }

        if (xSemaphoreTake(sem_button, 0)) {
            xSemaphoreTake(print_mutex, portMAX_DELAY);
            printf("System Alert Triggered!\n");
            xSemaphoreGive(print_mutex);

            gpio_set_level(LED_RED, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_level(LED_RED, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Idle delay to yield CPU
    }
}



void setup(void) {

    // Configure output LEDs
    gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << LED_GREEN) | (1ULL << LED_RED),
      .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Configure input button
    gpio_config_t btn_conf = {
      .pin_bit_mask = (1ULL << BUTTON_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn_conf);

    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_ADC_CHANNEL, ADC_ATTEN_DB_11);

    sem_button = xSemaphoreCreateBinary();
    sem_sensor = xSemaphoreCreateCounting(MAX_COUNT_SEM,0);
    print_mutex = xSemaphoreCreateMutex();



  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", sendHtml);

  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle Alarm");
    Serial.println(led);

    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        digitalWrite(LED1, led1State);
        break;
      case 2:
        led2State = !led2State;
        digitalWrite(LED2, led2State);
        break;
    }

    sendHtml();
  });

  server.begin();
  Serial.println("HTTP server started");



  xTaskCreate(system_task, "System Status", 2048, NULL, 1, NULL);
  xTaskCreate(sensor_task, "System Sensor", 2048, NULL, 2, NULL);
  xTaskCreate(button_task, "System Button", 2048, NULL, 3, NULL);
  xTaskCreate(event_handler_task, "Event_Handler", 2048, NULL, 2, NULL);

}

void loop(void) {
  server.handleClient();
  delay(2);
}
