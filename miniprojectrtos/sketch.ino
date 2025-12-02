#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// OLED SSD1306 (I2C)
#define PIN_OLED_SDA 8
#define PIN_OLED_SCL 9

// Potentiometer
#define PIN_POT 1

// Servo
#define PIN_SERVO1 18
#define PIN_SERVO2 2

// LEDs
#define PIN_LED_1 14 // Yellow (botol detected)
#define PIN_LED_2 13 // Red  (non-botol detected)
#define PIN_LED_3 21 // White

// Push Buttons
#define PIN_BTN_1 15 // botol detect
#define PIN_BTN_2 16 // non-botol detect
#define PIN_BTN_3 10 // start

// Stepper Motor (A4988) - not used in this example but kept
#define PIN_STEPPER_STEP   38
#define PIN_STEPPER_DIR    39
#define PIN_STEPPER_ENABLE 20

// Buzzer
#define PIN_BUZZER 17

TaskHandle_t hTaskOLED       = NULL;
TaskHandle_t hTaskStartBtn   = NULL;
TaskHandle_t hTaskServoPintu = NULL;
TaskHandle_t hTaskServobuang = NULL;
TaskHandle_t hTaskStepper    = NULL;
TaskHandle_t hTaskBuzzer     = NULL;
TaskHandle_t hTaskDetectBtn  = NULL;

const int stepsPerRev = 200;
volatile int stepDelayUS = 1000;

// OLED
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Servos
Servo servobuang;
Servo servopintu;

// Stepper (driver)
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEPPER_STEP, PIN_STEPPER_DIR);

// Mutex to protect state / counter updates
static SemaphoreHandle_t mutexstate = NULL;

int counterbotol = 0;
int state = 0; // 1 = start screen, 2 = open servo then -> 3, 3 = wait for detection buttons, 4 = botol detected, 5 = non-botol detected

// Task prototypes
void taskStateManager(void *pvParameters);
void taskOLED(void *pvParameters);
void taskStartBtn(void *pvParameters);
void taskServopintu(void *pvParameters);
void taskServobuang(void *pvParameters);
void taskDetectButtons(void *pvParameters);
void taskStepper(void *pvParameters);
void taskBuzzer(void *pvParameters);
void taskLEDindikator(void *pvParameters);

void setup() {
  Serial.begin(115200);
  Serial.println("--- Miniproject Demo ESP32-S3 (with detection v2) ---");

  // Pins
  pinMode(PIN_POT, INPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_LED_3, OUTPUT);
  digitalWrite(PIN_LED_1, LOW);
  digitalWrite(PIN_LED_2, LOW);
  digitalWrite(PIN_LED_3, LOW);
  pinMode(PIN_BTN_1, INPUT_PULLUP);
  pinMode(PIN_BTN_2, INPUT_PULLUP);
  pinMode(PIN_BTN_3, INPUT_PULLUP);
  pinMode(PIN_STEPPER_ENABLE, OUTPUT);
  digitalWrite(PIN_STEPPER_ENABLE, LOW); // enable driver
  pinMode(PIN_BUZZER, OUTPUT);

  // I2C OLED
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal inisialisasi SSD1306"));
    while (1) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("OLED OK");
  display.display();

  // Servos
  servopintu.attach(PIN_SERVO1);
  servobuang.attach(PIN_SERVO2);

  // Stepper
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setEnablePin(PIN_STEPPER_ENABLE);
  stepper.enableOutputs();

  // Mutex
  mutexstate = xSemaphoreCreateMutex();
  if (mutexstate == NULL) {
    Serial.println("Failed to create mutex!");
    while (1) delay(1000);
  }

  // initial state
  state = 1;

  // Create core tasks that are always present
  xTaskCreatePinnedToCore(taskLEDindikator, "LEDs", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskStateManager,  "StateMgr", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskOLED, "OLED", 4096, NULL, 3, &hTaskOLED, 1);
  xTaskCreatePinnedToCore(taskStartBtn, "StartBtn", 2048, NULL, 3, &hTaskStartBtn, 1);
  xTaskCreatePinnedToCore(taskServopintu, "ServoPintu", 2048, NULL, 2, &hTaskServoPintu, 1);
  xTaskCreatePinnedToCore(taskServobuang, "ServoBuang", 2048, NULL, 2, &hTaskServobuang, 1);
  xTaskCreatePinnedToCore(taskDetectButtons, "DetectBtn", 2048, NULL, 3, &hTaskDetectBtn, 1);
  // stepper and buzzer are optional; enable if needed:
  xTaskCreatePinnedToCore(taskStepper, "Stepper", 4096, NULL, 2, &hTaskStepper, 0);
  xTaskCreatePinnedToCore(taskBuzzer, "Buzzer", 2048, NULL, 1, &hTaskBuzzer, 0);
}

void loop() {
  delay(1000);
}

// State manager: kept for logging/extension
void taskStateManager(void *pvParameters) {
  (void) pvParameters;
  int last_state = -1;

  for(;;) {  
    int local_state;
    if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_state = state;
      xSemaphoreGive(mutexstate);
    } else {
      local_state = state;
    }

    if (local_state != last_state) {
      Serial.printf("State changed %d -> %d\n", last_state, local_state);
      last_state = local_state;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// OLED task: always running and updates per current state
void taskOLED(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    int local_state;
    if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_state = state;
      xSemaphoreGive(mutexstate);
    } else {
      local_state = state;
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ALAT DETEKSI BOTOL");
    display.setTextSize(1);

    if (local_state == 1) {
      display.println("tekan button start");
    } else if (local_state == 2) {
      display.println("Membuka pintu...");
    } else if (local_state == 3) {
      display.println("Masukkan botol plastik");
      display.println("Tunggu deteksi...");
      display.println("");
      display.println("BTN1=Botol  BTN2=Non");
    } else if (local_state == 4) {
      display.setTextSize(2);
      display.println("Sampah");
      display.println("terdeteksi");
      display.setTextSize(1);
      display.println("");
      display.println("-> BOTOL");
      display.println("");
      // show counter
      display.printf("Jumlah botol: %d", counterbotol);
    } else if (local_state == 5) {
      display.setTextSize(2);
      display.println("Sampah");
      display.println("terdeteksi");
      display.setTextSize(1);
      display.println("");
      display.println("-> NON-BOTOL");
    } else {
      display.printf("State: %d", local_state);
    }

    display.display();
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// Start button task: updates state when button pressed.
void taskStartBtn(void *pvParameters) {
  (void) pvParameters;
  bool btnState = true;
  for (;;) {
    if (digitalRead(PIN_BTN_3) == LOW && btnState == true) {
      Serial.println("Button START pressed!");
      if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(50)) == pdTRUE) {
        counterbotol = 0;
        state = 2; // trigger servo open -> it will set to 3 after opening
        xSemaphoreGive(mutexstate);
      }
      btnState = false;
    }
    if (digitalRead(PIN_BTN_3) == HIGH && btnState == false) {
      btnState = true;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Servo pintu task: move to 90 when entering state 2, then set state=3.
// Also respond to entering states 4/5 by closing to 0.
void taskServopintu(void *pvParameters) {
  (void) pvParameters;

  int last_state = -1;
  const int open_angle = 90;
  const int closed_angle = 0;

  // ensure known starting position
  servopintu.write(closed_angle);
  vTaskDelay(pdMS_TO_TICKS(200));

  for (;;) {
    int local_state;
    if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_state = state;
      xSemaphoreGive(mutexstate);
    } else {
      local_state = state;
    }

    if (local_state != last_state) {
      // detected transition
      if (local_state == 2) {
        // open to 90 (smooth)
        for (int p = closed_angle; p <= open_angle; ++p) {
          servopintu.write(p);
          vTaskDelay(pdMS_TO_TICKS(12));
        }
        servopintu.write(open_angle);
        // immediately change to state 3 so detection can happen
        if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(20)) == pdTRUE) {
          state = 3;
          xSemaphoreGive(mutexstate);
          Serial.println("Servo opened -> moving to state 3 (wait detection)");
        } else {
          state = 3;
        }
      } else if (local_state == 4 || local_state == 5) {
        // close to 0 when a detection state is entered
        for (int p = open_angle; p >= closed_angle; --p) {
          servopintu.write(p);
          vTaskDelay(pdMS_TO_TICKS(12));
        }
        servopintu.write(closed_angle);
        Serial.println("Servo pintu closed after detection");
      } else if (local_state == 1) {
        // ensure closed when returning to start
        servopintu.write(closed_angle);
      }
      last_state = local_state;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Servo buang: when entering state 4, move to 90 then back to 0 (one-shot)
void taskServobuang(void *pvParameters) {
  (void) pvParameters;
  int last_state = -1;
  const int junk_open = 90;
  const int junk_closed = 0;

  // ensure known starting position
  servobuang.write(junk_closed);
  vTaskDelay(pdMS_TO_TICKS(100));

  for (;;) {
    int local_state;
    if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_state = state;
      xSemaphoreGive(mutexstate);
    } else {
      local_state = state;
    }

    if (local_state != last_state) {
      if (local_state == 4) {
        // move to 90, wait a bit, move back
        for (int p = junk_closed; p <= junk_open; ++p) {
          servobuang.write(p);
          vTaskDelay(pdMS_TO_TICKS(12));
        }
        servobuang.write(junk_open);
        vTaskDelay(pdMS_TO_TICKS(500)); // slight hold
        for (int p = junk_open; p >= junk_closed; --p) {
          servobuang.write(p);
          vTaskDelay(pdMS_TO_TICKS(12));
        }
        servobuang.write(junk_closed);
        Serial.println("Servo buang cycled for BOTOL");
      }
      last_state = local_state;
    }

    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

// Detect buttons in state 3. BTN1 -> state 4 (botol), BTN2 -> state 5 (non-botol)
void taskDetectButtons(void *pvParameters) {
  (void) pvParameters;
  bool btn1Prev = true;
  bool btn2Prev = true;

  for (;;) {
    int local_state;
    if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_state = state;
      xSemaphoreGive(mutexstate);
    } else {
      local_state = state;
    }

    if (local_state == 3) {
      // BTN1 (botol)
      if (digitalRead(PIN_BTN_1) == LOW && btn1Prev == true) {
        Serial.println("DetectBtn: BTN1 -> BOTOL");
        if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(50)) == pdTRUE) {
          state = 4;
          counterbotol++;            // increment once per detection
          xSemaphoreGive(mutexstate);
        }
        digitalWrite(PIN_LED_1, HIGH);
        digitalWrite(PIN_LED_2, LOW);
        btn1Prev = false;
      }
      if (digitalRead(PIN_BTN_1) == HIGH && btn1Prev == false) {
        btn1Prev = true;
      }

      // BTN2 (non-botol)
      if (digitalRead(PIN_BTN_2) == LOW && btn2Prev == true) {
        Serial.println("DetectBtn: BTN2 -> NON-BOTOL");
        if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(50)) == pdTRUE) {
          state = 5;
          xSemaphoreGive(mutexstate);
        }
        digitalWrite(PIN_LED_2, HIGH);
        digitalWrite(PIN_LED_1, LOW);
        btn2Prev = false;
      }
      if (digitalRead(PIN_BTN_2) == HIGH && btn2Prev == false) {
        btn2Prev = true;
      }
    }

    // If we're in state 4 or 5, keep message for a short time then return to start (state 1)
    if (local_state == 4 || local_state == 5) {
      // show message for 3 seconds (OLED task handles the text)
      vTaskDelay(pdMS_TO_TICKS(3000));
      if (xSemaphoreTake(mutexstate, pdMS_TO_TICKS(50)) == pdTRUE) {
        // reset LEDs and return to start
        state = 2;
        xSemaphoreGive(mutexstate);
      } else {
        state = 2; // fallback
      }
      digitalWrite(PIN_LED_1, LOW);
      digitalWrite(PIN_LED_2, LOW);
      Serial.println("Returning to state 1 (start)");
    }

    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

void taskStepper(void *pvParameters) {
  (void) pvParameters;
  pinMode(PIN_STEPPER_STEP, OUTPUT);
  pinMode(PIN_STEPPER_DIR, OUTPUT);

  for (;;) {
    digitalWrite(PIN_STEPPER_DIR, HIGH);
    for (int i = 0; i < stepsPerRev; i++) {
      digitalWrite(PIN_STEPPER_STEP, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(PIN_STEPPER_STEP, LOW);
      ets_delay_us(stepDelayUS);
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    digitalWrite(PIN_STEPPER_DIR, LOW);
    for (int i = 0; i < stepsPerRev; i++) {
      digitalWrite(PIN_STEPPER_STEP, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(PIN_STEPPER_STEP, LOW);
      ets_delay_us(stepDelayUS);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void taskBuzzer(void *pvParameters) {
  (void) pvParameters;
  for(;;) {
    tone(PIN_BUZZER, 1000);
    vTaskDelay(pdMS_TO_TICKS(100));
    noTone(PIN_BUZZER);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void taskLEDindikator(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    // Optional heartbeat, disabled to avoid interfering with detection LEDs
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}