#include <ESP32Servo.h>

Servo myServo;
TaskHandle_t ServoTaskHandle;

void ServoTask(void *pvParameters) {
  for (;;) {
    myServo.write(0);
    vTaskDelay(pdMS_TO_TICKS(500));  

    myServo.write(90);
    vTaskDelay(pdMS_TO_TICKS(500));

  }
}

void setup() {
  myServo.attach(4, 500, 2500);  

  xTaskCreatePinnedToCore(
    ServoTask,
    "ServoTask",
    2048,
    NULL,
    1,
    &ServoTaskHandle,
    1 // Core 1, ubah ke 0 jika ingin menggunakan Core 0
  );
}

void loop() {
  // Empty - FreeRTOS handles tasks
}
