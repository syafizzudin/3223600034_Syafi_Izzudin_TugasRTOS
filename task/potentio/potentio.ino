#define potPin 3

void potTask(void *parameter) {
  for (;;) {
    int nilai = analogRead(potPin);
    float volt = (nilai / 4095) * 3.3;

    Serial.print("ADC: ");
    Serial.print(nilai);
    Serial.print(" | Volt: ");
    Serial.println(volt, 2);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(potPin, INPUT);

  xTaskCreatePinnedToCore(
    potTask,
    "Pot Task",
    2048,
    NULL,
    1,
    NULL,
    0
  );

  Serial.println("Potentiometer task running on Core 1");
}

void loop() {}