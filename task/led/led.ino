#define ledMerah 13
#define ledTosca 12
#define ledHijau 11

void ledTask(void *parameter) {
  for (;;) {
    digitalWrite(ledMerah, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(ledMerah, LOW);

    digitalWrite(ledTosca, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(ledTosca, LOW);

    digitalWrite(ledHijau, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(ledHijau, LOW);
vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(ledMerah, OUTPUT);
  pinMode(ledTosca, OUTPUT);
  pinMode(ledHijau, OUTPUT);

  xTaskCreatePinnedToCore(
    ledTask,          
    "LED Task",       
    2048,            
    NULL,             
    1,                
    NULL,             
    0                 
  );
}

void loop() {
}
