#define STEP_PIN 18
#define DIR_PIN 19

TaskHandle_t StepperTaskHandle;

const int stepsPerRev = 200;  
volatile int stepDelayUS = 1000; 

void StepperTask(void *pvParameters) {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  for (;;) {
    // Spin CW
    digitalWrite(DIR_PIN, HIGH);
    for (int i = 0; i < stepsPerRev; i++) {
      digitalWrite(STEP_PIN, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(STEP_PIN, LOW);
      ets_delay_us(stepDelayUS);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));  


    digitalWrite(DIR_PIN, LOW);
    for (int i = 0; i < stepsPerRev; i++) {
      digitalWrite(STEP_PIN, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(STEP_PIN, LOW);
      ets_delay_us(stepDelayUS);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));  
  }
}

void setup() {
  
  xTaskCreatePinnedToCore(
    StepperTask,
    "StepperTask",
    2048,
    NULL,
    2,
    &StepperTaskHandle,
    1 // Core 1, ubah ke 0 jika ingin menggunakan Core 0
  );
}

void loop() {
  
}
