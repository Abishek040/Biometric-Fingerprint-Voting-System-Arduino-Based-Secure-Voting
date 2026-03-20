#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

// --- Device Configurations ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial fpSerial(2, 3); // TX -> 2, RX -> 3
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// --- Pin Definitions ---
#define LED_GREEN 6
#define LED_YELLOW 7
#define LED_RED 11
#define BUZZER 12

#define BTN_ENROL A0
#define BTN_DEL A1
#define BTN_UP A2
#define BTN_DOWN A3
#define BTN_CAN1 5
#define BTN_CAN2 4
#define BTN_CAN3 8
#define BTN_RESULT 9
#define BTN_MATCH 10

// --- Global Variables ---
int voteCan1 = 0, voteCan2 = 0, voteCan3 = 0;
bool hasVoted[128]; 

void setup() {
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(BTN_ENROL, INPUT_PULLUP);
  pinMode(BTN_DEL, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_CAN1, INPUT_PULLUP);
  pinMode(BTN_CAN2, INPUT_PULLUP);
  pinMode(BTN_CAN3, INPUT_PULLUP);
  pinMode(BTN_RESULT, INPUT_PULLUP);
  pinMode(BTN_MATCH, INPUT_PULLUP);

  for (int i = 0; i < 128; i++) hasVoted[i] = false;

  finger.begin(57600);
  if (finger.verifyPassword()) {
    lcd.print("Sensor OK");
  } else {
    lcd.print("Sensor Error");
    while (1);
  }
  delay(1500);
  showWelcomeMessage();
}

void loop() {
  // 1. Enroll Mode
  if (digitalRead(BTN_ENROL) == LOW) {
    enrollFinger();
    showWelcomeMessage();
  }
  
  // 2. Delete Mode
  if (digitalRead(BTN_DEL) == LOW) {
    deleteFinger();
    showWelcomeMessage();
  }
  
  // 3. Voting Mode
  if (digitalRead(BTN_MATCH) == LOW) {
    startVotingProcess();
    showWelcomeMessage();
  }
  
  // 4. Result Mode
  if (digitalRead(BTN_RESULT) == LOW) {
    showResults();
    while (digitalRead(BTN_RESULT) == LOW); // Show until released
    showWelcomeMessage();
  }
}

// --- NEW ID SELECTION LOGIC ---

int getIDFromUser(uint8_t confirmButtonPin) {
  int currentID = 1;
  
  // IMPORTANT: Wait for the user to release the button they just pressed
  while(digitalRead(confirmButtonPin) == LOW); 
  delay(100); 

  lcd.clear();
  lcd.print("Select ID: 1");
  lcd.setCursor(0, 1);
  lcd.print("UP/DN | OK:ENR");

  while (true) {
    // UP Button
    if (digitalRead(BTN_UP) == LOW) {
      currentID++;
      if (currentID > 127) currentID = 1;
      updateLCD(currentID);
      delay(200); 
    }

    // DOWN Button
    if (digitalRead(BTN_DOWN) == LOW) {
      currentID--;
      if (currentID < 1) currentID = 127;
      updateLCD(currentID);
      delay(200);
    }

    // CONFIRM Button (Press Enroll again)
    if (digitalRead(confirmButtonPin) == LOW) {
      beep(100);
      while(digitalRead(confirmButtonPin) == LOW); // Wait for release
      return currentID;
    }
  }
}

void updateLCD(int id) {
  lcd.setCursor(11, 0);
  lcd.print(" "); // Clear old number
  lcd.setCursor(11, 0);
  lcd.print(id);
}

// --- SYSTEM FUNCTIONS ---

void showWelcomeMessage() {
  lcd.clear();
  lcd.print("Fingerprint");
  lcd.setCursor(0, 1);
  lcd.print("Voting System");
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
}

void beep(int duration) {
  tone(BUZZER, 1000, duration);
  delay(duration);
}

void enrollFinger() {
  beep(100);
  int id = getIDFromUser(BTN_ENROL); 
  
  lcd.clear();
  lcd.print("Enrolling #");
  lcd.print(id);
  digitalWrite(LED_YELLOW, HIGH);
  
  uint8_t p = getFingerprintEnroll(id);
  
  digitalWrite(LED_YELLOW, LOW);
  lcd.clear();
  if (p == FINGERPRINT_OK) {
    lcd.print("Enrolled!");
    beep(500);
  } else {
    lcd.print("Failed!");
    digitalWrite(LED_RED, HIGH);
    delay(1000);
    digitalWrite(LED_RED, LOW);
  }
  delay(2000);
}

void deleteFinger() {
  beep(100);
  int id = getIDFromUser(BTN_DEL); 
  lcd.clear();
  lcd.print("Deleting #");
  lcd.print(id);
  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    lcd.setCursor(0,1); lcd.print("Deleted!");
    beep(500);
  } else {
    lcd.setCursor(0,1); lcd.print("Failed!");
  }
  delay(2000);
}

void startVotingProcess() {
  beep(100);
  lcd.clear();
  lcd.print("Place finger...");
  digitalWrite(LED_YELLOW, HIGH); 

  int fingerID = getVerifiedFingerID();
  digitalWrite(LED_YELLOW, LOW);

  if (fingerID < 0) {
    lcd.clear(); lcd.print("Unknown User");
    digitalWrite(LED_RED, HIGH); beep(500); delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  if (hasVoted[fingerID]) {
    lcd.clear(); lcd.print("Already Voted!");
    digitalWrite(LED_RED, HIGH); beep(300); delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  // Valid Voter
  digitalWrite(LED_GREEN, HIGH); beep(200);
  lcd.clear(); lcd.print("Voter ID: "); lcd.print(fingerID);
  lcd.setCursor(0, 1); lcd.print("Vote C1, C2, C3");

  while (true) {
    if (digitalRead(BTN_CAN1) == LOW) { voteCan1++; break; }
    if (digitalRead(BTN_CAN2) == LOW) { voteCan2++; break; }
    if (digitalRead(BTN_CAN3) == LOW) { voteCan3++; break; }
  }

  hasVoted[fingerID] = true;
  digitalWrite(LED_GREEN, LOW);
  lcd.clear(); lcd.print("Recorded!"); beep(100); delay(1000);
}

void showResults() {
  lcd.clear();
  lcd.print("C1:"); lcd.print(voteCan1);
  lcd.print(" C2:"); lcd.print(voteCan2);
  lcd.setCursor(0, 1);
  lcd.print("C3:"); lcd.print(voteCan3);
}

int getVerifiedFingerID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

uint8_t getFingerprintEnroll(int id) {
  int p = -1;
  while (p != FINGERPRINT_OK) { p = finger.getImage(); }
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;

  lcd.clear(); lcd.print("Remove finger");
  delay(1000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) { p = finger.getImage(); }

  p = -1;
  lcd.clear(); lcd.print("Place again");
  while (p != FINGERPRINT_OK) { p = finger.getImage(); }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;

  if (finger.createModel() != FINGERPRINT_OK) return -1;
  return finger.storeModel(id);
}
