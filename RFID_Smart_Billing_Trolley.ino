#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define RFID_SS       5
#define RFID_RST      27

#define LCD_SDA       21
#define LCD_SCL       22

#define BUZZER_PIN    15
#define REMOVE_PIN    13

// =====================================================
// WIFI
// =====================================================

// Change these when using another Wi-Fi/hotspot
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
// UPI
// =====================================================

const char* upiID = "YOUR_UPI_ID";

// =====================================================
// OBJECTS
// =====================================================

MFRC522 rfid(RFID_SS, RFID_RST);

LiquidCrystal_I2C lcd(0x27, 16, 2);

WebServer server(80);

// =====================================================
// PRODUCT DATABASE
// =====================================================

String productUID[] = {
  "C24E2B07",
  "8BC0E519",
  "D7CF2959",
  "CAF08A30",
  "D76DF959",
  "3A5DFE30",
  "77EA2959",
  "DAE21931",
  "E7AB0E5A",
  "A3411207"
};

String productName[] = {
  "Milk",
  "Bread",
  "Rice",
  "Biscuit",
  "Oil",
  "Coffee",
  "Sugar",
  "Soap",
  "Shampoo",
  "Toothpaste"
};

int productPrice[] = {
  50,
  40,
  120,
  30,
  150,
  80,
  55,
  35,
  120,
  90
};

int productWeight[] = {
  500,
  400,
  1000,
  100,
  1000,
  200,
  1000,
  100,
  180,
  100
};

// =====================================================
// BILLING VARIABLES
// =====================================================

int quantity[10] = {0};

int totalBill = 0;

int expectedWeight = 0;

int simulatedWeight = 0;

int lastProductIndex = -1;

// =====================================================
// STATUS VARIABLES
// =====================================================

bool checkoutDone = false;

bool weightChecked = false;

bool weightVerified = false;

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  Wire.begin(LCD_SDA, LCD_SCL);

  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("RFID SMART");

  lcd.setCursor(0, 1);
  lcd.print("TROLLEY");

  delay(2000);

  // ---------------------------------------------------
  // RFID
  // ---------------------------------------------------

  SPI.begin();

  rfid.PCD_Init();

  Serial.println();
  Serial.println("==============================");
  Serial.println("RFID SMART TROLLEY");
  Serial.println("==============================");

  Serial.println("RFID Reader Ready!");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("RFID READER");

  lcd.setCursor(0, 1);
  lcd.print("READY!");

  delay(2000);

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  // ---------------------------------------------------
  // REMOVE BUTTON
  // ---------------------------------------------------

  pinMode(REMOVE_PIN, INPUT_PULLUP);

  // ---------------------------------------------------
  // WIFI - AUTOMATIC IP
  // ---------------------------------------------------

  Serial.println();
  Serial.println("Connecting to WiFi...");

  // No WiFi.config()
  // ESP32 gets IP automatically from DHCP

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi Connected!");

  Serial.print("Mobile Bill IP: ");
  Serial.println(WiFi.localIP());

  // ---------------------------------------------------
  // WEB SERVER
  // ---------------------------------------------------

  server.on("/", handleHome);

  server.on("/online", handleOnline);

  server.on("/checkout", handleWebCheckout);

  server.on("/offline", handleOffline);

  server.begin();

  Serial.println("Web server started!");

  Serial.print("Open on mobile: http://");
  Serial.println(WiFi.localIP());

  Serial.println();

  // ---------------------------------------------------
  // START SCREEN
  // ---------------------------------------------------

  showScanScreen();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Handle phone requests
  server.handleClient();

  // ---------------------------------------------------
  // SHOPPING MODE
  // ---------------------------------------------------

  if (!checkoutDone) {

    // RFID detection
    if (rfid.PICC_IsNewCardPresent()) {

      if (rfid.PICC_ReadCardSerial()) {

        processRFID();

        rfid.PICC_HaltA();

        rfid.PCD_StopCrypto1();

        delay(1000);
      }
    }
    // -------------------------------------------------
    // REMOVE BUTTON
    // -------------------------------------------------

    if (digitalRead(REMOVE_PIN) == LOW) {

      delay(50);

      if (digitalRead(REMOVE_PIN) == LOW) {

        removeLastProduct();

        // Wait until button is released
        while (digitalRead(REMOVE_PIN) == LOW) {
          delay(10);
        }
      }
    }

}

  // ---------------------------------------------------
  // WEIGHT VERIFICATION
  // ---------------------------------------------------

  if (checkoutDone && !weightChecked) {

    if (Serial.available()) {

      processWeightInput();
    }
  }
}

// =====================================================
// PROCESS RFID
// =====================================================

void processRFID() {

  String uid = "";

  // Build UID
  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {

      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();

  Serial.println();

  Serial.println("==============================");

  Serial.print("RFID UID: ");
  Serial.println(uid);

  // Find product
  int index = findProduct(uid);

  // ---------------------------------------------------
  // UNKNOWN RFID
  // ---------------------------------------------------

  if (index == -1) {

    Serial.println("Unknown Product");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Unknown Product");

    lcd.setCursor(0, 1);
    lcd.print(uid);

    beepError();

    delay(2000);

    showScanScreen();

    return;
  }

  // ---------------------------------------------------
  // ADD PRODUCT
  // ---------------------------------------------------

  quantity[index]++;

  totalBill += productPrice[index];

  expectedWeight += productWeight[index];

  // Remember the most recently scanned product
  lastProductIndex = index;

  // ---------------------------------------------------
  // SERIAL OUTPUT
  // ---------------------------------------------------

  Serial.print("Product: ");
  Serial.println(productName[index]);

  Serial.print("Price: Rs.");
  Serial.println(productPrice[index]);

  Serial.print("Expected Weight: ");
  Serial.print(productWeight[index]);

  if (index == 8) {

    Serial.println(" ml");
  }
  else {

    Serial.println(" g");
  }

  Serial.print("Quantity: ");
  Serial.println(quantity[index]);

  Serial.print("Total Bill: Rs.");
  Serial.println(totalBill);

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);

  String line1 = productName[index];

  line1 += " Rs";

  line1 += String(productPrice[index]);

  lcd.print(line1);

  lcd.setCursor(0, 1);

  lcd.print("Wt:");

  if (index == 8) {

    lcd.print("180ml");
  }
  else if (productWeight[index] >= 1000) {

    lcd.print("1kg");
  }
  else {

    lcd.print(String(productWeight[index]));

    lcd.print("g");
  }

  beepSuccess();

  Serial.println("==============================");

  delay(2000);

  showScanScreen();
}

// =====================================================
// FIND PRODUCT
// =====================================================

int findProduct(String uid) {

  for (int i = 0; i < 10; i++) {

    if (uid == productUID[i]) {

      return i;
    }
  }

  return -1;
}

// =====================================================
// REMOVE LAST PRODUCT
// =====================================================

void removeLastProduct() {

  // ---------------------------------------------------
  // NOTHING TO REMOVE
  // ---------------------------------------------------

  if (lastProductIndex == -1 ||
      quantity[lastProductIndex] <= 0) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("NOTHING TO REMOVE");
    Serial.println("==============================");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("NOTHING TO");

    lcd.setCursor(0, 1);
    lcd.print("REMOVE");

    beepError();

    delay(2000);

    showScanScreen();

    return;
  }

  // ---------------------------------------------------
  // REMOVE PRODUCT
  // ---------------------------------------------------

  int index = lastProductIndex;

  quantity[index]--;

  totalBill -= productPrice[index];

  expectedWeight -= productWeight[index];

  Serial.println();
  Serial.println("==============================");
  Serial.println("PRODUCT REMOVED");
  Serial.println("==============================");

  Serial.print("Product: ");
  Serial.println(productName[index]);

  Serial.print("Remaining Quantity: ");
  Serial.println(quantity[index]);

  Serial.print("Total Bill: Rs.");
  Serial.println(totalBill);

  Serial.print("Expected Weight: ");
  Serial.print(expectedWeight);
  Serial.println(" g");

  // ---------------------------------------------------
  // Find another product still in the cart
  // ---------------------------------------------------

  lastProductIndex = -1;

  for (int i = 0; i < 10; i++) {

    if (quantity[i] > 0) {
      lastProductIndex = i;
    }
  }

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("REMOVED:");

  lcd.setCursor(0, 1);
  lcd.print(productName[index]);

  beepSuccess();

  delay(2000);

  // ---------------------------------------------------
  // CART EMPTY
  // ---------------------------------------------------

  if (totalBill == 0) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("CART EMPTY");

    lcd.setCursor(0, 1);
    lcd.print("SCAN PRODUCTS");

    delay(2000);
  }

  showScanScreen();
}

// =====================================================
// CHECKOUT
// =====================================================

void checkout() {

  // ---------------------------------------------------
  // EMPTY CART CHECK
  // ---------------------------------------------------

  if (totalBill == 0) {

    Serial.println();

    Serial.println("CART IS EMPTY!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("CART EMPTY");

    lcd.setCursor(0, 1);
    lcd.print("SCAN PRODUCTS");

    beepError();

    delay(2000);

    showScanScreen();

    return;
  }

  // ---------------------------------------------------
  // CHECKOUT START
  // ---------------------------------------------------

  checkoutDone = true;

  Serial.println();

  Serial.println("================================");

  Serial.println("CHECKOUT STARTED");

  Serial.println("================================");

  Serial.print("Total Bill: Rs.");
  Serial.println(totalBill);

  Serial.print("Expected Weight: ");
  Serial.print(expectedWeight);
  Serial.println(" g");

  Serial.println();

  Serial.println("ENTER ACTUAL WEIGHT");

  Serial.println("IN SERIAL MONITOR");

  Serial.println("Example: 1500");

  Serial.println();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("CHECKOUT");

  lcd.setCursor(0, 1);
  lcd.print("ENTER WEIGHT");

  beepCheckout();
}

// =====================================================
// PROCESS WEIGHT
// =====================================================

void processWeightInput() {

  int weight = Serial.parseInt();

  if (weight <= 0) {

    return;
  }

  simulatedWeight = weight;

  Serial.println();

  Serial.println("================================");

  Serial.println("WEIGHT VERIFICATION");

  Serial.println("================================");

  Serial.print("Expected Weight: ");
  Serial.print(expectedWeight);
  Serial.println(" g");

  Serial.print("Measured Weight: ");
  Serial.print(simulatedWeight);
  Serial.println(" g");

  int difference = simulatedWeight - expectedWeight;

  Serial.print("Difference: ");
  Serial.print(difference);
  Serial.println(" g");

  // ---------------------------------------------------
  // WEIGHT PASS
  // ---------------------------------------------------

  if (abs(difference) <= 20) {

    weightVerified = true;

    Serial.println();

    Serial.println("WEIGHT VERIFIED");

    Serial.println("FAULT CHECK: PASS");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WEIGHT PASS");

    lcd.setCursor(0, 1);
    lcd.print("Rs.");
    lcd.print(totalBill);

    beepSuccess();
  }

  // ---------------------------------------------------
  // EXTRA WEIGHT
  // ---------------------------------------------------

  else if (difference > 20) {

    weightVerified = false;

    Serial.println();

    Serial.println("WEIGHT MISMATCH");

    Serial.println("POSSIBLE UNSCANNED ITEM");

    Serial.println("FAULT CHECK: FAIL");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WEIGHT FAIL");

    lcd.setCursor(0, 1);
    lcd.print("UNSCANNED ITEM");

    beepError();
  }

  // ---------------------------------------------------
  // LESS WEIGHT
  // ---------------------------------------------------

  else {

    weightVerified = false;

    Serial.println();

    Serial.println("WEIGHT MISMATCH");

    Serial.println("POSSIBLE REMOVED ITEM");

    Serial.println("FAULT CHECK: FAIL");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WEIGHT FAIL");

    lcd.setCursor(0, 1);
    lcd.print("ITEM REMOVED");

    beepError();
  }

  Serial.println("================================");

  weightChecked = true;

  Serial.println();

  Serial.print("Mobile Bill: http://");

  Serial.println(WiFi.localIP());

  Serial.println();
}

// =====================================================
// LCD SCAN SCREEN
// =====================================================

void showScanScreen() {

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("SCAN YOUR RFID");

  lcd.setCursor(0, 1);

  lcd.print("TAG...");
}

// =====================================================
// SUCCESS BEEP
// =====================================================

void beepSuccess() {

  digitalWrite(BUZZER_PIN, HIGH);

  delay(150);

  digitalWrite(BUZZER_PIN, LOW);
}

// =====================================================
// ERROR BEEP
// =====================================================

void beepError() {

  for (int i = 0; i < 3; i++) {

    digitalWrite(BUZZER_PIN, HIGH);

    delay(100);

    digitalWrite(BUZZER_PIN, LOW);

    delay(100);
  }
}

// =====================================================
// CHECKOUT BEEP
// =====================================================

void beepCheckout() {

  digitalWrite(BUZZER_PIN, HIGH);

  delay(300);

  digitalWrite(BUZZER_PIN, LOW);
}

// =====================================================
// CREATE UPI LINK
// =====================================================

String makeUPILink() {

  String upiLink = "upi://pay?pa=";

  upiLink += upiID;

  upiLink += "&pn=Smart%20Trolley";

  upiLink += "&am=";

  upiLink += String(totalBill);

  upiLink += "&cu=INR";

  return upiLink;
}

// =====================================================
// URL ENCODING
// =====================================================

String urlEncode(String input) {

  String encoded = "";

  char hex[] = "0123456789ABCDEF";

  for (int i = 0; i < input.length(); i++) {

    char c = input.charAt(i);

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' ||
        c == '_' ||
        c == '.' ||
        c == '~') {

      encoded += c;
    }
    else {

      encoded += '%';

      encoded += hex[(c >> 4) & 0x0F];

      encoded += hex[c & 0x0F];
    }
  }

  return encoded;
}

// =====================================================
// MOBILE BILL HOME PAGE
// =====================================================

void handleHome() {

  String html = "";

  html += "<!DOCTYPE html>";

  html += "<html><head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  html += "<meta http-equiv='refresh' content='5'>";

  html += "<title>Smart Trolley</title>";

  // ---------------------------------------------------
  // CSS
  // ---------------------------------------------------

  html += "<style>";

  html += "body{font-family:Arial,sans-serif;background:#f2f2f2;text-align:center;padding:15px;}";

  html += ".box{background:white;padding:20px;border-radius:15px;max-width:600px;margin:auto;box-shadow:0 2px 10px #aaa;}";

  html += "table{width:100%;border-collapse:collapse;margin-top:15px;}";

  html += "th,td{padding:10px;border-bottom:1px solid #ddd;text-align:center;}";

  html += "th{background:#eeeeee;}";

  html += ".totalweight{font-size:20px;font-weight:bold;}";

  html += ".total{font-size:26px;font-weight:bold;}";

  html += ".button{display:block;padding:16px;margin:10px;border-radius:10px;text-decoration:none;font-size:18px;font-weight:bold;color:white;}";

  html += ".online{background:#28a745;}";

  html += ".offline{background:#555555;}";

  html += ".pass{font-size:22px;font-weight:bold;color:#28a745;}";

  html += ".fail{font-size:22px;font-weight:bold;color:#cc0000;}";

  html += "</style>";

  html += "</head><body>";

  html += "<div class='box'>";

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  html += "<h1>RFID SMART TROLLEY</h1>";

  if (!checkoutDone) {

    html += "<h3>Shopping in Progress</h3>";
  }
  else {

    html += "<h3>CHECKOUT</h3>";
  }

  // ---------------------------------------------------
  // BILL TABLE
  // ---------------------------------------------------

  html += "<table>";

  html += "<tr>";

  html += "<th>Product</th>";

  html += "<th>Qty</th>";

  html += "<th>Weight</th>";

  html += "<th>Price</th>";

  html += "</tr>";

  for (int i = 0; i < 10; i++) {

    if (quantity[i] > 0) {

      html += "<tr>";

      // Product
      html += "<td>";
      html += productName[i];
      html += "</td>";

      // Quantity
      html += "<td>";
      html += String(quantity[i]);
      html += "</td>";

      // Weight
      html += "<td>";

      int itemWeight =
        productWeight[i] * quantity[i];

      if (i == 8) {

        html += String(itemWeight);
        html += " ml";
      }
      else if (itemWeight >= 1000) {

        if (itemWeight % 1000 == 0) {

          html += String(itemWeight / 1000);
          html += " kg";
        }
        else {

          html += String(itemWeight);
          html += " g";
        }
      }
      else {

        html += String(itemWeight);
        html += " g";
      }

      html += "</td>";

      // Price
      html += "<td>";

      html += "Rs.";

      html += String(productPrice[i] * quantity[i]);

      html += "</td>";

      html += "</tr>";
    }
  }

  html += "</table>";

  // ---------------------------------------------------
  // TOTAL WEIGHT
  // ---------------------------------------------------

  html += "<p class='totalweight'>";

  html += "TOTAL WEIGHT: ";

  if (expectedWeight >= 1000) {

    if (expectedWeight % 1000 == 0) {

      html += String(expectedWeight / 1000);

      html += " kg";
    }
    else {

      html += String(expectedWeight);

      html += " g";
    }
  }
  else {

    html += String(expectedWeight);

    html += " g";
  }

  html += "</p>";

  // ---------------------------------------------------
  // TOTAL BILL
  // ---------------------------------------------------

  html += "<p class='total'>";

  html += "TOTAL: Rs.";

  html += String(totalBill);

  html += "</p>";

  // ---------------------------------------------------
  // WEIGHT VERIFICATION
  // ---------------------------------------------------

  if (checkoutDone && weightChecked) {

    html += "<hr>";

    html += "<h3>Weight Verification</h3>";

    html += "<p>Expected Weight: ";

    html += String(expectedWeight);

    html += " g</p>";

    html += "<p>Measured Weight: ";

    html += String(simulatedWeight);

    html += " g</p>";

    // -------------------------------------------------
    // PASS
    // -------------------------------------------------

    if (weightVerified) {

      html += "<p class='pass'>";

      html += "✓ WEIGHT VERIFIED";

      html += "</p>";

      html += "<p>";

      html += "FAULT CHECK: PASS";

      html += "</p>";

      // Online payment
      html += "<a class='button online' href='/online'>";

      html += "PAY ONLINE";

      html += "</a>";

      // Offline payment
      html += "<a class='button offline' href='/offline'>";

      html += "PAY OFFLINE";

      html += "</a>";
    }

    // -------------------------------------------------
    // FAIL
    // -------------------------------------------------

    else {

      html += "<p class='fail'>";

      html += "✗ WEIGHT MISMATCH";

      html += "</p>";

      html += "<p>";

      html += "FAULT CHECK: FAIL";

      html += "</p>";

      html += "<p><b>";

      html += "PAYMENT DISABLED";

      html += "</b></p>";
    }
  }

  // ---------------------------------------------------
  // WAITING FOR WEIGHT
  // ---------------------------------------------------

  else if (checkoutDone) {

    html += "<hr>";

    html += "<h3>Waiting for Weight Verification</h3>";

    html += "<p>";

    html += "Enter actual weight in Serial Monitor.";

    html += "</p>";
  }

  // ---------------------------------------------------
  // SHOPPING
  // ---------------------------------------------------

  else {

    html += "<hr>";

    html += "<p>";

    html += "Press REMOVE button on trolley to remove the last item.";

    html += "</p>";

    // Mobile checkout button
    if (totalBill > 0) {

      html += "<a class='button online' href='/checkout'>";
      html += "CHECKOUT";
      html += "</a>";
    }
  }

  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// =====================================================
// MOBILE CHECKOUT
// =====================================================

void handleWebCheckout() {

  // ---------------------------------------------------
  // Empty cart check
  // ---------------------------------------------------

  if (totalBill == 0) {

    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");

    return;
  }

  // ---------------------------------------------------
  // Start checkout
  // ---------------------------------------------------

  checkout();

  // Return to mobile bill page
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

// =====================================================
// ONLINE PAYMENT PAGE
// =====================================================

void handleOnline() {

  // Payment security check
  if (!checkoutDone ||
      !weightChecked ||
      !weightVerified) {

    server.send(
      403,
      "text/plain",
      "Payment not allowed"
    );

    return;
  }

  // ---------------------------------------------------
  // UPI LINK
  // ---------------------------------------------------

  String upiLink = makeUPILink();

  String encodedUPI = urlEncode(upiLink);

  // ---------------------------------------------------
  // QR CODE
  // ---------------------------------------------------

  String qrURL =
    "https://api.qrserver.com/v1/create-qr-code/?size=250x250&data="
    + encodedUPI;

  String html = "";

  html += "<!DOCTYPE html><html><head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  html += "<title>Online Payment</title>";

  html += "<style>";

  html += "body{font-family:Arial,sans-serif;text-align:center;background:#f2f2f2;padding:20px;}";

  html += ".box{background:white;padding:25px;border-radius:15px;max-width:400px;margin:auto;box-shadow:0 2px 10px #aaa;}";

  html += "img{width:250px;height:250px;}";

  html += ".amount{font-size:28px;font-weight:bold;}";

  html += "</style>";

  html += "</head><body>";

  html += "<div class='box'>";

  html += "<h1>ONLINE PAYMENT</h1>";

  // Amount
  html += "<p class='amount'>Amount: Rs.";

  html += String(totalBill);

  html += "</p>";

  // Weight
  html += "<p>Verified Weight: ";

  html += String(simulatedWeight);

  html += " g</p>";

  // UPI ID
  html += "<p>UPI ID:</p>";

  html += "<b>";

  html += upiID;

  html += "</b>";

  html += "<br><br>";

  // QR
  html += "<img src='";

  html += qrURL;

  html += "'>";

  html += "<h3>Scan QR to Pay</h3>";

  html += "<p>Use any UPI app</p>";

  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// =====================================================
// OFFLINE PAYMENT PAGE
// =====================================================

void handleOffline() {

  // Payment security check
  if (!checkoutDone ||
      !weightChecked ||
      !weightVerified) {

    server.send(
      403,
      "text/plain",
      "Payment not allowed"
    );

    return;
  }

  String html = "";

  html += "<!DOCTYPE html><html><head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  html += "<title>Offline Payment</title>";

  html += "<style>";

  html += "body{font-family:Arial,sans-serif;text-align:center;background:#f2f2f2;padding:20px;}";

  html += ".box{background:white;padding:30px;border-radius:15px;max-width:400px;margin:auto;box-shadow:0 2px 10px #aaa;}";

  html += ".amount{font-size:30px;font-weight:bold;}";

  html += ".weight{font-size:20px;}";

  html += "</style>";

  html += "</head><body>";

  html += "<div class='box'>";

  html += "<h1>OFFLINE PAYMENT</h1>";

  // Amount
  html += "<p class='amount'>Rs.";

  html += String(totalBill);

  html += "</p>";

  // Weight
  html += "<p class='weight'>Verified Weight: ";

  html += String(simulatedWeight);

  html += " g</p>";

  html += "<h3>Payment at Counter</h3>";

  html += "<p>Please pay the amount in cash.</p>";

  html += "<h2>Thank You!</h2>";

  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}