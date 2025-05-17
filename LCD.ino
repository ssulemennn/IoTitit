#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Ganti alamat 0x27 jika LCD-mu memakai alamat lain (misal 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Alamat I2C, kolom 16, baris 2

void setup() {
  // Inisialisasi komunikasi I2C dengan pin SDA = 21, SCL = 22
  Wire.begin(21, 22);

  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();  // Nyalakan lampu latar

  // Tampilkan teks
  lcd.setCursor(0, 0);     // Kolom 0, Baris 0
  lcd.print("Halo Dunia!");

  lcd.setCursor(0, 1);     // Kolom 0, Baris 1
  lcd.print("ESP32 + LCD I2C");
}

void loop() {
  // Tidak ada aksi di loop
}
