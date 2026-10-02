# RFID + Weight Sensor Smart Billing Trolley

An ESP32-based smart billing trolley that uses RFID for product identification and weight verification to automate the billing process.

## 🚀 Project Overview

This project is designed to reduce manual billing and improve shopping convenience.

When a product is placed in the trolley:

1. The RFID tag is scanned.
2. The product name and price are identified.
3. The product weight is added to the expected total weight.
4. The bill is updated automatically.
5. A physical button can be used to remove the last scanned product.
6. The customer can view the bill through a mobile webpage.
7. During checkout, the actual/simulated weight is compared with the expected weight.
8. Online payment can be initiated through the webpage.

## 🔧 Components Used

- ESP32 Development Board
- RC522 RFID Reader
- RFID Tags/Cards
- Load Cell / Weight Sensor
- HX711 Load Cell Amplifier
- 16x2 I2C LCD
- Push Button
- Buzzer
- Jumper Wires
- Breadboard
- Wi-Fi Network

## 💻 Technologies Used

- Arduino C++
- ESP32
- RFID
- Wi-Fi
- Web Server
- LCD I2C
- Weight Sensing

## 📌 Features

- RFID-based product identification
- Automatic bill calculation
- Product quantity tracking
- Weight verification
- Remove last product using physical button
- Mobile-friendly billing webpage
- Online payment option
- LCD display
- Buzzer indication

## 📁 Project Files

| File | Description |
|---|---|
| `RFID_Smart_Billing_Trolley.ino` | Main ESP32 Arduino program |
| `1.jpeg` | Project image |
| `2.jpeg` | Project image |
| `3.jpeg` | Project image |
| `4.jpeg` | Project image |
| `5.jpeg` | Project image |

## 🔌 Pin Configuration

| Component | ESP32 Pin |
|---|---|
| RFID SS | GPIO 5 |
| RFID RST | GPIO 27 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |
| Buzzer | GPIO 15 |
| Remove Button | GPIO 13 |

## 🛒 Product Information

The system supports multiple products, with each product associated with:

- RFID UID
- Product name
- Price
- Expected weight

## 🔄 Working Flow

```text
Product
   ↓
RFID Scan
   ↓
Identify Product
   ↓
Add Product to Bill
   ↓
Update Expected Weight
   ↓
Display Bill
   ↓
Checkout
   ↓
Weight Verification
   ↓
Payment
