#define buttonPin  13   
#define ledPin     12

void buttonTask(void *parameter) {
  for (;;) {
    int tombol = digitalRead(buttonPin);

    if (tombol == LOW) {              
      digitalWrite(ledPin, HIGH);     
    } else {
      digitalWrite(ledPin, LOW);      
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);  
  pinMode(ledPin, OUTPUT);

  xTaskCreatePinnedToCore(
    buttonTask,         
    "Button Task",      
    2048,               
    NULL,               
    1,                  
    NULL,               
    0                   
  );

  Serial.println("Button task running on Core 1");
}

void loop() {
}