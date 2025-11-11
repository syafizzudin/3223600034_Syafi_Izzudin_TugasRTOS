<<<<<<< HEAD
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 14
#define OLED_SCL 13

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Task function for OLED animation
void oledTask(void *parameter) {
  const char* text = "CORE 0";

  for (;;) {
    display.clearDisplay();
    display.setCursor(0, 20);

    // Typewriter effect
    for (int i = 0; i < strlen(text); i++) {
      display.print(text[i]);
      display.display();
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    display.clearDisplay();
    display.display();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 initialization failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  
  xTaskCreatePinnedToCore(
    oledTask,         
    "OLED Task",      
    4096,             
    NULL,             
    1,                
    NULL,             
    0                 
  );

  Serial.println("OLED task running on Core 1");
}

void loop() {
}
=======
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 14
#define OLED_SCL 13

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Task function for OLED animation
void oledTask(void *parameter) {
  const char* text = "CORE 0";

  for (;;) {
    display.clearDisplay();
    display.setCursor(0, 20);

    // Typewriter effect
    for (int i = 0; i < strlen(text); i++) {
      display.print(text[i]);
      display.display();
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    display.clearDisplay();
    display.display();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 initialization failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  
  xTaskCreatePinnedToCore(
    oledTask,         
    "OLED Task",      
    4096,             
    NULL,             
    1,                
    NULL,             
    0                 
  );

  Serial.println("OLED task running on Core 1");
}

void loop() {
}
>>>>>>> e721cc7e22c7a83e0501231324ede13f8c8ce00a
