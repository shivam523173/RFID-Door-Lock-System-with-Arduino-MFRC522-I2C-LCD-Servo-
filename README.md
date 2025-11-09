# RFID Door Lock System with Arduino (MFRC522 + I2C LCD + Servo)

Single-sketch RFID door lock using **Arduino**, **MFRC522**, **I2C 16×2 LCD**, **servo**, **LEDs**, and **buzzer**. On first boot, the system enters **enrollment mode**—the first tapped card becomes the **master** and is saved in **EEPROM**. Later scans grant/deny based on that master UID.

## 🧩 Components
- Arduino UNO/Nano
- MFRC522 RFID module + cards/tags (power at **3.3 V**)
- 16×2 LCD with I2C backpack (PCF8574, addr `0x27` or `0x3F`)
- Servo SG90/MG90S
- Blue/Green/Red LEDs + 220 Ω resistors
- Buzzer, wires, breadboard, 5 V USB power

## 📚 Libraries
`MFRC522`, `LiquidCrystal_I2C`, `Servo`, `SPI`, `Wire`, `EEPROM`  
*(Install via IDE: Tools → Manage Libraries)*

## 🔌 Wiring (UNO/Nano)
**MFRC522 → Arduino**  
SDA(SS)→D10, SCK→D13, MOSI→D11, MISO→D12, RST→D9, VCC→3.3 V, GND→GND

**LCD (I2C) → Arduino**  
VCC→5 V, GND→GND, SDA→A4, SCL→A5

**LEDs**: Blue→D2, Green→D3, Red→D4 (each via 220 Ω to GND)  
**Buzzer**: +→D5, −→GND  
**Servo**: Signal→D6, VCC→5 V, GND→GND

## ▶️ Usage
1. Upload `RFID_Door_Lock_Single.ino`.
2. On first boot, tap a card to **enroll** as master (stored in EEPROM).
3. Next scans: **master = Access Granted** (unlock for a few seconds). Others are denied.
4. Adjust `LOCK_POS`, `UNLOCK_POS`, `UNLOCK_TIME`, and `LCD_ADDR` as needed.

## 🛡️ Notes
- Power MFRC522 strictly at **3.3 V**.
- If LCD shows blocks/blank, try address `0x27 ↔ 0x3F`.
- Provide a stable 5 V supply for LCD + servo (avoid USB brownouts).

## 📜 License
MIT – educational and hobby use.
