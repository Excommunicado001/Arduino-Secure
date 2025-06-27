#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>
#include <Keypad.h>

const int pirPin = 10;
const int flamePin = 11;
const int waterPin = A0;
const int buzzerPin = 12;
const int servoPin = 13;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

RTC_DS1307 rtc;
Servo doorServo;
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const String correctCode = "1234";
String inputCode = "";
bool isUnlocked = false;

unsigned long doorOpenTime = 0;
const unsigned long autoCloseDelay = 10000;
unsigned long lastAlarmTime = 0;
const unsigned long alarmDelay = 2000;

void setup() {
  Serial.begin(9600);
  pinMode(pirPin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(waterPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  doorServo.attach(servoPin);
  doorServo.write(0);

  if (!rtc.begin()) {
    while (1);
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("Sicherheitssystem gestartet.");
  Serial.println("Geben Sie den 4-stelligen PIN-Code ein (# zum Bestätigen, * zum Abbrechen):");
}

void loop() {
  handleKeypadInput();
  monitorSensors();
  autoCloseDoor();
}

void handleKeypadInput() {
  char key = keypad.getKey();
  if (key) {
    if (isdigit(key)) {
      inputCode += key;
      Serial.print("*");
    } else if (key == '#') {
      if (inputCode.length() == 4) {
        DateTime now = rtc.now();
        if (inputCode == correctCode) {
          Serial.print("\nCode korrekt. Tür öffnet sich um ");
          printTime(now);
          doorServo.write(90);
          isUnlocked = true;
          doorOpenTime = millis();
        } else {
          Serial.print("\nFalscher Code. Versuch um ");
          printTime(now);
          doorServo.write(0);
          isUnlocked = false;
        }
        inputCode = "";
        Serial.println("\nNeuen Code eingeben:");
      } else {
        Serial.println("\nFehler: Code muss 4 Stellen haben!");
        inputCode = "";
        Serial.println("Neuen Code eingeben:");
      }
    } else if (key == '*') {
      inputCode = "";
      Serial.println("\nEingabe abgebrochen.");
      Serial.println("Neuen Code eingeben:");
    }
  }
}

void monitorSensors() {
  if (!isUnlocked && millis() - lastAlarmTime > alarmDelay) {
    bool motionDetected = digitalRead(pirPin);
    bool flameDetected = digitalRead(flamePin) == LOW;
    int waterLevel = analogRead(waterPin);
    DateTime now = rtc.now();

    if (motionDetected || flameDetected || waterLevel > 500) {
      Serial.print("\nALARM um ");
      printTime(now);
      Serial.println("!");
      if (motionDetected) {
        Serial.print("Bewegung erkannt um ");
        printTime(now);
        Serial.println("!");
      }
      if (flameDetected) {
        Serial.print("Flamme erkannt um ");
        printTime(now);
        Serial.println("!");
      }
      if (waterLevel > 500) {
        Serial.print("Hoher Wasserstand um ");
        printTime(now);
        Serial.println("!");
      }
      triggerBuzzer(3);
      lastAlarmTime = millis();
    }
  }
}

void autoCloseDoor() {
  if (isUnlocked && millis() - doorOpenTime >= autoCloseDelay) {
    DateTime now = rtc.now();
    doorServo.write(0);
    isUnlocked = false;
    Serial.print("Tür automatisch geschlossen um ");
    printTime(now);
    Serial.println("!");
  }
}

void triggerBuzzer(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(400);
    digitalWrite(buzzerPin, LOW);
    delay(200);
  }
}

void printTime(DateTime dt) {
  char timeString[20];
  snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
  Serial.print(timeString);
}
