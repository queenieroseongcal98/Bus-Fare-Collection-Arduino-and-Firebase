//ESP8266
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <ESP8266Firebase.h>
#include <ESP8266WiFi.h>

#include <Keypad.h>

//ESP8266
#define _SSID "DRRM OFFICE"        // To be changed
#define _PASSWORD "Drrmoffice@2020"    // To be changed
#define PROJECT_ID "rfid-based-bfcs-default-rtdb"
#define FINGERPRINT "A1 B8 E0 02 42 47 38 1D 84 1B 3C B3 1F 6D B8 F4 13 77 57 EF"
Firebase firebase(PROJECT_ID, FINGERPRINT);

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'7','8','9','C'},
  {'4','5','6','B'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {D3, D2, D1, D0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {D7, D6, D5, D4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), colPins, rowPins, ROWS, COLS );

void setup(){
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(_SSID);
  WiFi.begin(_SSID, _PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }
}
  
void loop(){
  char key = keypad.getKey();
  
  if (key){
    Serial.println(key);
    firebase.setString("Temp/Place", String(key));
    delay(2000);
  }
}
