#define CLK 6
#define DT 7
#define SW 8

TaskHandle_t EncoderTaskHandle;

volatile int lastCLK;
int count = 0;
void EncoderTask(void *pvParameters) {
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);

  lastCLK = digitalRead(CLK);

  for (;;) {
    int currentCLK = digitalRead(CLK);
    
    // Rotation detected
    if (currentCLK != lastCLK) {
      if (digitalRead(DT) != currentCLK) {
        Serial.println("Rotated Right +");
        count += 1;
        Serial.println(count);
      } else {
        Serial.println("Rotated Left -");
        count -= 1;
        Serial.println(count);
      }
      lastCLK = currentCLK;
    }

    // Button pressed
    if (digitalRead(SW) == LOW) {
      Serial.println("Button Pressed");
      vTaskDelay(pdMS_TO_TICKS(200)); 
    }

    vTaskDelay(pdMS_TO_TICKS(1)); 
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    EncoderTask,
    "EncoderTask",
    2048,
    NULL,
    1,              
    &EncoderTaskHandle,
    1 // Core 1, ubah ke 0 jika ingin menggunakan Core 0         
  );
}

void loop() {
  // FreeRTOS handles everything
}
