/*
  RFID Door Lock System with Arduino (MFRC522 + I2C LCD + Servo)
  - First run: If no master UID in EEPROM, the first scanned card becomes MASTER.
  - After enrollment: Only MASTER unlocks; others are denied.
  - LCD, LEDs, buzzer give clear feedback.

  Wiring (UNO/Nano):
    MFRC522: SDA(SS)->D10, SCK->D13, MOSI->D11, MISO->D12, RST->D9, VCC->3.3V, GND->GND
    LCD I2C: VCC 5V, GND GND, SDA A4, SCL A5  (address 0x27 or 0x3F)
    LEDs:    Blue->D2, Green->D3, Red->D4 (each with 220Ω to GND)
    Buzzer:  + -> D5, - -> GND
    Servo:   Signal->D6, VCC->5V, GND->GND

  Libraries: MFRC522 (Miguel Balboa), LiquidCrystal_I2C (Frank de Brabander),
             Servo, SPI, Wire, EEPROM
*/

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

// ---------------- Pins ----------------
#define RST_PIN   9
#define SS_PIN    10
#define LED_BLUE  2
#define LED_GREEN 3
#define LED_RED   4
#define BUZZER    5
#define SERVO_PIN 6

// --------------- LCD ------------------
#ifndef LCD_ADDR
#define LCD_ADDR  0x27     // change to 0x3F if your LCD uses that address
#endif
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// -------------- Servo -----------------
#define LOCK_POS     10    // adjust to your lock's "locked" angle
#define UNLOCK_POS  100    // adjust to your lock's "unlocked" angle
#define UNLOCK_TIME 3000   // ms door stays unlocked

// -------------- RFID ------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ------------- EEPROM -----------------
// Layout:
//  [0]  = magic flag 0xA5 (set when master stored)
//  [1]  = master UID length (1..10)
//  [2..11] = master UID bytes (max 10)
#define EE_FLAG_ADDR   0
#define EE_LEN_ADDR    1
#define EE_UID_ADDR    2
#define EE_MAGIC     0xA5

// ----------- Globals ------------------
Servo servo;
byte masterUID[10] = {0};
byte masterLen = 0;
bool hasMaster = false;

// ----------- Helpers ------------------
void beep(unsigned f, unsigned t) {
  tone(BUZZER, f);
  delay(t);
  noTone(BUZZER);
}

void idleStatus() {
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER);
  servo.write(LOCK_POS);
}

void lcdIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  Access Control ");
  lcd.setCursor(0, 1); lcd.print(" Scan Your Card  ");
}

void toHexString(const byte* uid, byte len, String& out) {
  out = "";
  for (byte i = 0; i < len; i++) {
    out += (uid[i] < 0x10) ? " 0" : " ";
    out += String(uid[i], HEX);
  }
  out.toUpperCase();
  if (out.length() && out[0] == ' ') out.remove(0, 1);
}

void saveMasterToEEPROM(const byte* uid, byte len) {
  EEPROM.update(EE_FLAG_ADDR, EE_MAGIC);
  EEPROM.update(EE_LEN_ADDR, len);
  for (byte i = 0; i < 10; i++) {
    EEPROM.update(EE_UID_ADDR + i, (i < len) ? uid[i] : 0x00);
  }
}

bool loadMasterFromEEPROM() {
  if (EEPROM.read(EE_FLAG_ADDR) != EE_MAGIC) return false;
  byte len = EEPROM.read(EE_LEN_ADDR);
  if (len == 0 || len > 10) return false;
  masterLen = len;
  for (byte i = 0; i < 10; i++) {
    masterUID[i] = EEPROM.read(EE_UID_ADDR + i);
  }
  return true;
}

bool sameUID(const byte* a, byte la, const byte* b, byte lb) {
  if (la != lb) return false;
  for (byte i = 0; i < la; i++) if (a[i] != b[i]) return false;
  return true;
}

// Returns true and fills 'uid' & 'len' when a card is read
bool readCard(byte* uid, byte& len) {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial())   return false;

  len = (mfrc522.uid.size > 10) ? 10 : mfrc522.uid.size;
  for (byte i = 0; i < len; i++) uid[i] = mfrc522.uid.uidByte[i];

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return true;
}

void grantAccess() {
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);

  lcd.clear();
  lcd.setCursor(2, 0); lcd.print("Permission");
  lcd.setCursor(0, 1); lcd.print(" Access Granted ");
  beep(2000, 200); delay(150); beep(2000, 200);

  servo.write(UNLOCK_POS);
  delay(UNLOCK_TIME);
  servo.write(LOCK_POS);
}

void denyAccess() {
  lcd.clear();
  lcd.setCursor(2, 0); lcd.print("Permission");
  lcd.setCursor(0, 1); lcd.print(" Access Denied  ");

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_RED, HIGH);  beep(1800, 120);
    digitalWrite(LED_RED, LOW);   delay(130);
  }
  noTone(BUZZER);
}

// --------------- Setup ----------------
void setup() {
  pinMode(LED_BLUE,  OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);

  servo.attach(SERVO_PIN);
  servo.write(LOCK_POS);

  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  // Some libraries need begin; calling both is harmless:
  lcd.begin(16, 2);
  lcd.backlight();

  hasMaster = loadMasterFromEEPROM();

  if (!hasMaster) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("No Master Found!");
    lcd.setCursor(0, 1); lcd.print("Tap to ENROLL...");
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);

    Serial.println(F(">> Enrollment Mode: Tap a card to set as MASTER UID"));
    byte uid[10]; byte len = 0;
    while (!readCard(uid, len)) { delay(10); }

    saveMasterToEEPROM(uid, len);
    memcpy(masterUID, uid, len);
    masterLen = len;
    hasMaster = true;

    String s; toHexString(masterUID, masterLen, s);
    Serial.print(F("Enrolled MASTER UID: ")); Serial.println(s);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Master Enrolled!");
    lcd.setCursor(0, 1); lcd.print(" Ready to Scan  ");
    beep(2200, 180); delay(140); beep(2200, 180);
    delay(1200);
  }

  lcdIdleScreen();
  idleStatus();
}

// ---------------- Loop ----------------
void loop() {
  idleStatus();

  byte uid[10]; byte len = 0;
  if (readCard(uid, len)) {
    String s; toHexString(uid, len, s);
    Serial.print(F("Scanned UID: ")); Serial.println(s);

    if (hasMaster && sameUID(uid, len, masterUID, masterLen)) {
      grantAccess();
    } else {
      denyAccess();
    }

    delay(700);
    lcdIdleScreen();
  }

  delay(15);
}
