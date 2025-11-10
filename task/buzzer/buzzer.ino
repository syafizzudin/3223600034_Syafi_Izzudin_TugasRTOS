#define buzzerPin 14  

void buzzerTask(void *parameter) {
  for (;;) {
    tone(buzzerPin, 2000); 
    vTaskDelay(500 / portTICK_PERIOD_MS);
    noTone(buzzerPin);
    vTaskDelay(300 / portTICK_PERIOD_MS);

    tone(buzzerPin, 2500); 
    vTaskDelay(500 / portTICK_PERIOD_MS);
    noTone(buzzerPin);
    vTaskDelay(800 / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    buzzerTask,        
    "Buzzer Task",     
    2048,              
    NULL,              
    1,                 
    NULL,              
    1  // Core 1, ubah ke 0 jika ingin menggunakan Core 0               
  );

  Serial.println("Buzzer task running on Core 0");
}

void loop() {
}