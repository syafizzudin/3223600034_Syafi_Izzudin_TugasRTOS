/*
 * Program FreeRTOS Multi-Task untuk ESP32-S3
 * * Mengontrol beberapa periferal secara independen di task terpisah
 * dengan prioritas yang berbeda-beda.
 *
 * * VERSI FINAL (Perbaikan Buzzer + Perbaikan Typo):
 * - Menggunakan tone()/noTone() untuk buzzer
 * - Memperbaiki typo 'portTICK_PERIOD_MS' di loop()
 *
 * PERHATIAN: PIN_BUZZER (GPIO 17) adalah pin asumsi.
 * Harap ubah wiring di Wokwi: Pindahkan kabel buzzer dari 3.3V ke GPIO 17.
*/

// 1. Termasuk Library
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// 2. Definisi Pin (sesuai diagram.json Anda)
// OLED SSD1306 (I2C)
#define PIN_OLED_SDA 8
#define PIN_OLED_SCL 9

// Potentiometer
#define PIN_POT 1

// Servo
#define PIN_SERVO 18

// LEDs
#define PIN_LED_1 14 // Yellow
#define PIN_LED_2 13 // Red
#define PIN_LED_3 21 // White

// Push Buttons
#define PIN_BTN_1 15
#define PIN_BTN_2 16

// Rotary Encoder
#define PIN_ENCODER_CLK 10
#define PIN_ENCODER_DT  11
#define PIN_ENCODER_SW  12

// Stepper Motor (A4988)
#define PIN_STEPPER_STEP   38
#define PIN_STEPPER_DIR    39
#define PIN_STEPPER_ENABLE 20

#define STEP_PIN 18
#define DIR_PIN 19

TaskHandle_t StepperTaskHandle;

const int stepsPerRev = 200;  
volatile int stepDelayUS = 1000; 

// Buzzer (Asumsi pin setelah wiring diperbaiki)
#define PIN_BUZZER 17

// 3. Inisialisasi Objek Library
// OLED
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Servo
Servo myservo;

// Stepper
// Tipe driver: 1 = Driver (STEP/DIR)
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEPPER_STEP, PIN_STEPPER_DIR);

// Variabel Global (untuk demo sederhana)
long encoderPos = 0;

// 4. Deklarasi Fungsi Task
void taskOLED(void *pvParameters);
void taskLEDs(void *pvParameters);
void taskButtons(void *pvParameters);
void taskPotentiometer(void *pvParameters);
void taskEncoder(void *pvParameters);
void taskServo(void *pvParameters);
void taskStepper(void *pvParameters);
void taskBuzzer(void *pvParameters);

// ---------------------------------------------------
// SETUP
// ---------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("--- Multi-Task Demo ESP32-S3 (Final) ---");

  // Inisialisasi Pin
  pinMode(PIN_POT, INPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_LED_3, OUTPUT);
  pinMode(PIN_BTN_1, INPUT_PULLUP);
  pinMode(PIN_BTN_2, INPUT_PULLUP);
  pinMode(PIN_ENCODER_CLK, INPUT);
  pinMode(PIN_ENCODER_DT, INPUT);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_STEPPER_ENABLE, OUTPUT);
  digitalWrite(PIN_STEPPER_ENABLE, LOW); // Aktifkan driver
  pinMode(PIN_BUZZER, OUTPUT); // Set pin buzzer sebagai OUTPUT

  // Inisialisasi I2C untuk OLED
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Gagal inisialisasi SSD1306"));
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("OLED OK");
  display.display();

  // Inisialisasi Servo
  myservo.attach(PIN_SERVO);

  // Inisialisasi Stepper
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setEnablePin(PIN_STEPPER_ENABLE);
  stepper.enableOutputs();

  // --- Buat Tasks ---
  // Sesuai permintaan: prioritas diubah-ubah
  // tskIDLE_PRIORITY = 0 (Paling rendah)
  // configMAX_PRIORITIES = 24 (Default)

  // Core 0: Untuk I/O umum dan display
  xTaskCreatePinnedToCore(taskOLED,        "OLED",     4096, NULL, 1, NULL, 0); // Prio 2
  xTaskCreatePinnedToCore(taskLEDs,        "LEDs",     1024, NULL, 1, NULL, 0); // Prio 1
  xTaskCreatePinnedToCore(taskButtons,     "Buttons",  2048, NULL, 1, NULL, 0); // Prio 2
  xTaskCreatePinnedToCore(taskPotentiometer,"Pot",     2048, NULL, 1, NULL, 1); // Prio 2
  xTaskCreatePinnedToCore(taskBuzzer,      "Buzzer",   2048, NULL, 1, NULL, 0); // Prio 1

  // Core 1: Untuk timing-sensitive (Servo, Stepper, Encoder)
  xTaskCreatePinnedToCore(taskEncoder,     "Encoder",  2048, NULL, 1, NULL, 1); // Prio 3 (Penting)
  xTaskCreatePinnedToCore(taskServo,       "Servo",    2048, NULL, 1, NULL, 0); // Prio 2
  xTaskCreatePinnedToCore(taskStepper,     "Stepper",  4096, NULL, 1, NULL, 1); // Prio 3 (Penting)
}

// ---------------------------------------------------
// LOOP (dibiarkan kosong, RTOS yang bekerja)
// ---------------------------------------------------
void loop() {
  // Kosong. Scheduler akan menjalankan semua task.
  // vTaskDelay ini (dengan 1 'T') penting agar task IDLE tidak 100% CPU
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// ---------------------------------------------------
// Implementasi Task
// ---------------------------------------------------

// Task 1: Mengupdate OLED
void taskOLED(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("--- TASK AKTIF ---");
    display.setTextSize(1);
    display.printf("Encoder: %ld\n", encoderPos);
    display.printf("Pot: %d\n", analogRead(PIN_POT));
    display.printf("Btn1: %d Btn2: %d\n", 
                   digitalRead(PIN_BTN_1), digitalRead(PIN_BTN_2));
    display.println("Core 0, Prio 2");
    display.display();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Update 10x per detik
  }
}

// Task 2: Blinking LED
void taskLEDs(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    digitalWrite(PIN_LED_1, HIGH); // Nyalakan LED 1
    vTaskDelay(300 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED_1, LOW);
    
    digitalWrite(PIN_LED_2, HIGH); // Nyalakan LED 2
    vTaskDelay(300 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED_2, LOW);
    
    digitalWrite(PIN_LED_3, HIGH); // Nyalakan LED 3
    vTaskDelay(300 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED_3, LOW);
  }
}

// Task 3: Membaca Button (print ke Serial)
void taskButtons(void *pvParameters) {
  (void) pvParameters;
  bool btn1State = true, btn2State = true;
  for (;;) {
    if (digitalRead(PIN_BTN_1) == LOW && btn1State == true) {
      Serial.println("Button 1 Ditekan!");
      btn1State = false;
    }
    if (digitalRead(PIN_BTN_1) == HIGH && btn1State == false) {
      btn1State = true;
    }

    if (digitalRead(PIN_BTN_2) == LOW && btn2State == true) {
      Serial.println("Button 2 Ditekan!");
      btn2State = false;
    }
    if (digitalRead(PIN_BTN_2) == HIGH && btn2State == false) {
      btn2State = true;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
  }
}

// Task 4: Membaca Potentiometer (print ke Serial)
void taskPotentiometer(void *pvParameters) {
  (void) pvParameters;
  int lastPotValue = 0;
  for (;;) {
    int potValue = analogRead(PIN_POT);
    // Hanya print jika ada perubahan signifikan
    if (abs(potValue - lastPotValue) > 10) {
      Serial.printf("Potentiometer Value: %d\n", potValue);
      lastPotValue = potValue;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Task 5: Membaca Rotary Encoder
void taskEncoder(void *pvParameters) {
  (void) pvParameters;
  int lastClkState = digitalRead(PIN_ENCODER_CLK);
  bool swState = true;

  for (;;) {
    // Baca Encoder
    int clkState = digitalRead(PIN_ENCODER_CLK);
    if (clkState != lastClkState) { // Ada gerakan
      if (digitalRead(PIN_ENCODER_DT) != clkState) {
        encoderPos++;
        Serial.printf("Encoder +: %ld\n", encoderPos);
      } else {
        encoderPos--;
        Serial.printf("Encoder -: %ld\n", encoderPos);
      }
    }
    lastClkState = clkState;

    // Baca Tombol Encoder
    if (digitalRead(PIN_ENCODER_SW) == LOW && swState == true) {
      Serial.println("Encoder Switch Ditekan!");
      swState = false;
    }
    if (digitalRead(PIN_ENCODER_SW) == HIGH && swState == false) {
      swState = true;
    }
    
    vTaskDelay(1 / portTICK_PERIOD_MS); // Cek sangat cepat
  }
}

// Task 6: Menggerakkan Servo (Sweep)
void taskServo(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    myservo.write(0);
    vTaskDelay(pdMS_TO_TICKS(500));  

    myservo.write(90);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Task 7: Menggerakkan Stepper
void taskStepper(void *pvParameters) {
  (void) pvParameters;
  stepper.moveTo(2000); // Gerak ke posisi 2000
  
  for (;;) {
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
}

// Task 8: Memainkan Nada di Buzzer (Beep... Beep...)
// * VERSI PERBAIKAN MENGGUNAKAN tone() *
void taskBuzzer(void *pvParameters) {
  (void) pvParameters;
  for(;;) {
    // Pastikan wiring sudah benar ke PIN_BUZZER (GPIO 17)
    Serial.println("Buzzer: Beep!");
    
    // Mainkan nada 1000Hz di pin buzzer
    tone(PIN_BUZZER, 1000); 
    vTaskDelay(100 / portTICK_PERIOD_MS); // Biarkan menyala selama 100ms
    
    // Matikan nada
    noTone(PIN_BUZZER); 
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Diam selama 2 detik
  }
}