/**
  Kepler Roket Takımı Lab. RFID Card Reader System
  Name: lab_card_reader
  Purpose: Students can show their student cards to the RFID reader
  and can grant access to the lab.

  @author Ahmet Ozrahat
  @version 1.0 13/01/23
*/

#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <RTClib.h>

#define CHIP_SELECT_SD 8 // CS pin of SD Card module.

#define RESET_RC522 9 // Reset pin of the RC522 module.
#define CHIP_SELECT_RC522 10 // CS pin of the RC522 module.

#define BUZZER_PIN 4 // Buzzer pin.
#define LED_R 6 // RGB Led red pin.
#define LED_G 5 // RGB Led green pin.
#define LED_B 7 // RGB Led blue pin.

MFRC522 rfid(CHIP_SELECT_RC522, RESET_RC522); // Initialization of the RC522 module.
RTC_DS3231 rtc; // Initialization of the RTC module.
File file; // Initialization of the file.

bool is_debug_mode = true;

typedef struct {
  byte id[4];
  char* name;
  bool isInside;
} card;

card cardDict[] {
    {{0, 82, 95, 10}, "Ahmet", false},
    {{69, 221, 31, 62}, "Acar", false},
    {{252, 131, 13, 34}, "YETKİLİ", false},
    {{231, 12, 98, 164}, "Böke", false},
    {{169, 196, 91, 184}, "Kerim", false},
    {{181, 9, 92, 54}, "Burçak", false},
};

void setup() 
{
  SPI.begin();
  rfid.PCD_Init(); 
  rtc.begin();

  check_rtc_adjustment();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  set_led_color(255, 255, 255);

  Serial.begin(115200);
  Serial.println("SD kart hazirlaniyor..."); 
  if(!SD.begin(CHIP_SELECT_SD))
  {
    Serial.println("SD kart takili degil!!!");
    Serial.println("Lutfen SD kartinizi takiniz!!!");
    while(!SD.begin(CHIP_SELECT_SD));
    Serial.println("SD kart kullanima hazir!!!"); 
    delay(1000);
  }
  else
  {
    Serial.println("SD kart kullanima hazir!!!");
    delay(1000);
  }
}

void loop()
{
  if (rfid.PICC_IsNewCardPresent()) { 
    if(rfid.PICC_ReadCardSerial()) {
      print_card_id(rfid.uid.uidByte);
      int result = check_card(rfid.uid.uidByte);
      if (result != -1) {
        if (cardDict[result].isInside) blink_yellow(); else blink_green();
        Serial.println(cardDict[result].isInside ? "Hoşçakalın: " : "Hoşgeldiniz: ");
        Serial.println(cardDict[result].name);
        if (!is_debug_mode)
          log_entrance(get_log_entrance_string(result));
        cardDict[result].isInside = !cardDict[result].isInside;        
      }else {
        blink_red();
        Serial.println("Kart tanınmadı.");
      }
    }
    rfid.PICC_HaltA(); 
  } 
}

/**
  Checks the RTC time adjustments and adjusts if the power is lost.
*/
void check_rtc_adjustment() {
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

/**
  Logs the given entrance data to the SD card.

  @param data Entrance data.
*/
void log_entrance(String data) {
  file = SD.open("log.csv", FILE_WRITE);
  file.println(data);
  file.close();
  Serial.println("Veri SD karta yazıldı.");
}

/**
  Returns a csv row for logging the student entrance.

  @param id The student RFID card id.
  @return CSV row in the format of `id,name,date`
*/
String get_log_entrance_string(int id) {
  String data = "";
  DateTime now = rtc.now();
  char buffer[19];
  sprintf(buffer, "%02d/%02d/%d %02d:%02d:%02d\0", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  for (int i = 0; i < 4; i++) {
    data += cardDict[id].id[i];
  }
  data += ";";
  data += cardDict[id].name;
  data += ";";
  data += buffer;
  data += ";";
  data += cardDict[id].isInside ? "Çıkış" : "Giriş";
  Serial.println(data);
  return data;
}

/**
  Prints any given RFID card id to Serial Monitor.

  @param card_id RFID card id as 4 bytes.
*/
void print_card_id(byte card_id[4]) {
  Serial.println("Okunan Kart ID: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(card_id[i]);
    Serial.print(" ");
  }
  Serial.println();
}

/**
  Checks whether a recognized card is read or not.

  @param card_id 4 bytes long card id shown to RFID reader.
  @return Whether the card is recognized or not.
*/
int check_card(byte card_id[4]) {
  int length = sizeof(cardDict) / sizeof(cardDict[0]);
  for (int i = 0; i < 4; i++) {
    bool flag = true;
    for (int j = 0; j < 4; j++) {
      if (card_id[j] != cardDict[i].id[j]) {
        flag = false;
        break;
      }
    }
    if (flag) return i;
  }
  return -1;
}

/**
  Sets the leds color.

  @param red The red value in range of 0-255.
  @param green The green value in range of 0-255.
  @param blue The blue value in range of 0-255.
*/
void set_led_color(int red, int green, int blue){ 
  analogWrite(LED_R, red); 
  analogWrite(LED_G, green); 
  analogWrite(LED_B, blue);
}

/**
  Blinks the RGB light as green and power the buzzer
  once in order to show an recognized card has been read.
*/
void blink_green() {
  digitalWrite(BUZZER_PIN, HIGH);
  set_led_color(0, 255, 0);
  delay(200);
  set_led_color(255, 255, 255);
  digitalWrite(BUZZER_PIN, LOW);
}

/**
  Blinks the RGB light as yellow and power the buzzer
  once in order to show an recognized card has been read
  and the user is leaving.
*/
void blink_yellow() {
  digitalWrite(BUZZER_PIN, HIGH);
  set_led_color(180, 0, 0);
  delay(200);
  set_led_color(255, 255, 255);
  digitalWrite(BUZZER_PIN, LOW);
}

/**
  Blinks the RGB light as red and power the buzzer
  2 times with short intervals in order to show
  an unrecognized card has been read.
*/
void blink_red() {
  digitalWrite(BUZZER_PIN, HIGH);
  set_led_color(255, 0, 0);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  set_led_color(255, 255, 255);
  digitalWrite(BUZZER_PIN, LOW);
}