#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <ESP32Servo.h>

// === PIN SETUP (disesuaikan dengan wiring diagram) ===
#define RST_PIN    4
#define SS_PIN     5
#define TRIG_PIN   2  // (silakan sesuaikan jika sensor ultrasonik tetap digunakan)
#define ECHO_PIN   15 // (silakan sesuaikan jika sensor ultrasonik tetap digunakan)
#define SERVO_PIN  13
#define SDA_PIN    21
#define SCL_PIN    22

// === EEPROM SETUP ===
#define EEPROM_SIZE 64
#define UID_LENGTH 4
#define MAX_UIDS (EEPROM_SIZE / UID_LENGTH)

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2); // alamat bisa 0x3F jika 0x27 tidak berfungsi

String mode = "";
unsigned long lastDistanceUpdate = 0;
long distance = 0;

void setup() {
  Serial.begin(115200);

  // LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Absensi");

  // RFID
  SPI.begin(); // default: SCK=18, MISO=19, MOSI=23
  rfid.PCD_Init();

  // Ultrasonik
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Servo
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  delay(2000);
  lcd.clear();
}

long readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

bool isRegistered(byte *uid) {
  for (int i = 0; i < MAX_UIDS; i++) {
    int base = i * UID_LENGTH;
    bool match = true;
    for (int j = 0; j < UID_LENGTH; j++) {
      if (EEPROM.read(base + j) != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

bool saveUIDToEEPROM(byte *uid) {
  for (int i = 0; i < MAX_UIDS; i++) {
    int base = i * UID_LENGTH;
    bool empty = true;
    for (int j = 0; j < UID_LENGTH; j++) {
      if (EEPROM.read(base + j) != 0xFF) {
        empty = false;
        break;
      }
    }
    if (empty) {
      for (int j = 0; j < UID_LENGTH; j++) {
        EEPROM.write(base + j, uid[j]);
      }
      EEPROM.commit();
      return true;
    }
  }
  return false;
}

String uidToString(byte *uid, byte length) {
  String result = "";
  for (byte i = 0; i < length; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  return result;
}

void loop() {
  // Update mode setiap 2 detik berdasarkan jarak
  if (millis() - lastDistanceUpdate > 2000) {
    distance = readUltrasonicDistance();
    lastDistanceUpdate = millis();

    if (distance > 30) {
      mode = "REGISTRASI";
    } else {
      mode = "LOGIN";
    }

    Serial.print("Jarak: ");
    Serial.print(distance);
    Serial.print(" cm | Mode: ");
    Serial.println(mode);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode: " + mode);
    lcd.setCursor(0, 1);
    lcd.print("Jarak: ");
    lcd.print(distance);
    lcd.print(" cm");
  }

  // Proses RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uidStr = uidToString(rfid.uid.uidByte, rfid.uid.size);
    Serial.print("UID: ");
    Serial.println(uidStr);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("UID:");
    lcd.setCursor(0, 1);
    lcd.print(uidStr);

    if (mode == "REGISTRASI") {
      if (isRegistered(rfid.uid.uidByte)) {
        Serial.println(" | Sudah Terdaftar");
        lcd.clear();
        lcd.print("Sudah Terdaftar");
      } else {
        if (saveUIDToEEPROM(rfid.uid.uidByte)) {
          Serial.println(" | Berhasil DIDAFTARKAN");
          lcd.clear();
          lcd.print("UID Terdaftar");
        } else {
          Serial.println(" | GAGAL - EEPROM PENUH");
          lcd.clear();
          lcd.print("EEPROM Penuh");
        }
      }
    } else if (mode == "LOGIN") {
      if (isRegistered(rfid.uid.uidByte)) {
        Serial.println(" | Akses DITERIMA");
        lcd.clear();
        lcd.print("Akses Diterima");
        myServo.write(90);
        delay(1000);
        myServo.write(0);
      } else {
        Serial.println(" | Akses DITOLAK");
        lcd.clear();
        lcd.print("Akses Ditolak");
        myServo.write(180);
        delay(1000);
        myServo.write(0);
      }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(1500);
  }
}
