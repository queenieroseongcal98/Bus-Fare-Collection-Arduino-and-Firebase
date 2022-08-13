//ESP8266
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <ESP8266Firebase.h>
#include <ESP8266WiFi.h>

//RFID
#include <SPI.h>
#include <MFRC522.h>

//GPS
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

//LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//NTP Date&Time
#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

//GPS
//TinyGPSPlus gps;
SoftwareSerial espSerial(1, 3); //RX -> D1 TX -> D0

//ESP8266
#define _SSID "DRRM OFFICE"        // To be changed
#define _PASSWORD "Drrmoffice@2020"    // To be changed
#define PROJECT_ID "rfid-based-bfcs-default-rtdb"
#define FINGERPRINT "A1 B8 E0 02 42 47 38 1D 84 1B 3C B3 1F 6D B8 F4 13 77 57 EF"
Firebase firebase(PROJECT_ID, FINGERPRINT);

//RFID
MFRC522 rfid(D4, D3);

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);

//firebase consts
const int MINIMUM_FARE = 50;

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup() {
  Serial.begin(115200);
  timeClient.begin();
  timeClient.setTimeOffset(28800);

  rfidReader_SetUp();
  //gps_SetUp();
  esp8266_SetUp();
  lcd_SetUp();
}

void loop() {
  //<<-- start rfid card reading -->>
  String tag;
  lcdPrint("Tap your card","",3,1);
  if( ! rfid.PICC_IsNewCardPresent()) return;
  if(rfid.PICC_ReadCardSerial()){
    for(byte i = 0; i < 4; i++){
      tag += rfid.uid.uidByte[i];
    }
    //Serial.println("RFID : " + tag);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    //buzz for 1 sec
    digitalWrite(D0, HIGH);
    delay(100);
    digitalWrite(D0, LOW);

    //Get Origin and Destination
    lcd.clear();
    lcdPrint("Choose Origin: ","",0,0);
    bool buttonIsPushed_flag = false;
    int origin = 0;
    while(!buttonIsPushed_flag){
      String originDB = firebase.getString("Temp/Place");
      if(originDB == "1"){
        origin = 2; //Tagb
        buttonIsPushed_flag = true;
        lcdPrint("Tagbilaran","",2,1);
      }
      else if(originDB == "2"){
        origin = 3; //Baclayon
        buttonIsPushed_flag = true;
        lcdPrint("Baclayon","",2,1);
      }
      else if(originDB == "3"){
        origin = 4; //Albur
        buttonIsPushed_flag = true;
        lcdPrint("Albur","",2,1);
      }
      else if(originDB == "4"){
        origin = 5; //loay
        buttonIsPushed_flag = true;
        lcdPrint("Loay","",2,1);
      }
      else if(originDB == "5"){
        origin = 6; //loboc
        buttonIsPushed_flag = true;
        lcdPrint("Loboc","",2,1);
      }
    }
    firebase.deleteData("Temp");
    lcdPrint("Choose Dest: ","",0,2);
    buttonIsPushed_flag = false;
    int destination = 0;
    while(!buttonIsPushed_flag){
      String destDB = firebase.getString("Temp/Place");
      if(destDB == "1"){
        destination = 2; //Tagb
        buttonIsPushed_flag = true;
        lcdPrint("Tagbilaran","",2,3);
      }
      else if(destDB == "2"){
        destination = 3; //Baclayon
        buttonIsPushed_flag = true;
        lcdPrint("Baclayon","",2,3);
      }
      else if(destDB == "3"){
        destination = 4; //Albur
        buttonIsPushed_flag = true;
        lcdPrint("Albur","",2,3);
      }
      else if(destDB == "4"){
        destination = 5; //Loay
        buttonIsPushed_flag = true;
        lcdPrint("Loay","",2,3);
      }
      else if(destDB == "5"){
        destination = 6; //Loboc
        buttonIsPushed_flag = true;
        lcdPrint("Loboc","",2,3);
      }
    }
    
    firebase.deleteData("Temp");
    //Serial.clear();
    int fare = getFare(origin, destination);
    
    //get data from db
    getDataFromDB(tag, fare, origin, destination);
    tag = "";
  }

}

void lcd_SetUp(){
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
}
void rfidReader_SetUp(){
  SPI.begin();
  rfid.PCD_Init();

  //for buzzer
  pinMode(D0, OUTPUT);
  digitalWrite(D0, LOW);
}

/*void gps_SetUp(){
  //ss.begin(9600);
  getGPSInfo();
}*/

void esp8266_SetUp(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  // Connect to WiFi
  //Serial.println();
  //Serial.print("Connecting to: ");
  //Serial.println(_SSID);
  WiFi.begin(_SSID, _PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print("-");
  }

  //Serial.println("");
  //Serial.println("WiFi Connected");
}

//get fare
int getFare(int origin, int destination){
    /*{1,1,15},{1,2,15},{1,3,20},{1,4,30},{1,5,40},{1,6,50},
     {2,1,15},{2,2,15},{2,3,20},{2,4,30},{2,5,40},{2,6,50},
     {3,1,20},{3,2,20},{3,3,15},{3,4,20},{3,5,30},{3,6,40},
     {4,1,30},{4,2,30},{4,3,20},{4,4,15},{4,5,20},{5,6,30},
     {5,1,40},{5,2,40},{5,3,30},{5,4,20},{5,5,15},{5,6,20},
     {6,1,50},{6,2,50},{6,3,40},{6,4,30},{6,5,20},{6,6,15}*/
  int fareList[6][6] = {{15,15,20,30,40,50},
                        {15,15,20,30,40,50},
                        {20,20,15,20,30,40},
                        {30,30,20,15,20,30},
                        {40,40,30,20,15,20},
                        {50,50,40,30,20,15}};
  return fareList[origin-1][destination-1];
}

//get place
String getPlace(int point){
  String places[6] = {"Bus Trmnl","Tagb","Baclyn","Albur","Loay","Loboc"};
  return places[point-1];
}

//get GPS location
/*void getGPSInfo()
{
  while (ss.available() > 0){
    if (gps.encode(ss.read())){
        Serial.print(F("Location: ")); 
        if (gps.location.isValid())
        {
          Serial.print(gps.location.lat(), 6);
          Serial.print(F(","));
          Serial.print(gps.location.lng(), 6);
          String loc = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
          //lcdPrint("LatLng", loc);
        }
        else
        {
          Serial.print(F("INVALID"));
        }
        Serial.println();
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}*/

//Displays value of toPrint variable
void lcdPrint(String label, String toPrint, int x, int y){      
  //lcd.init();
  lcd.backlight();
  lcd.setCursor(x,y);
  lcd.print(label + " " + toPrint);
}

//Get initial data from firebase
void getDataFromDB(String uid, int fare, int origin, int destination){
  lcd.clear();
  lcdPrint(getPlace(origin)+"->",getPlace(destination),0,0);
  lcdPrint("Fare: " + String(fare),".00",1,1);
  lcdPrint("Loading card info ..","",0,2);
  String loadbal = firebase.getString("User/" + uid + "/LoadBal");
  String cardstat = firebase.getString("User/" + uid + "/CardStatus");
  //String boardstat = firebase.getString("User/" + uid + "/BoardingStatus");
  lcd.clear();
  //Serial.println("Loadbal: " + String(loadbal));
  if(loadbal.toInt() >= fare){
    if(cardstat == "1"){
      lcdPrint("Load Bal: ", String(loadbal.toInt()-fare) + ".00",0,0);
      lcdPrint("Card Status: ", "Active",0,2);
      
      //update load balance
      int newBal = loadbal.toInt() - fare;
      firebase.setString("User/" + uid + "/LoadBal", String(newBal));

      //add travel record
      //get time from net
      timeClient.update();
      time_t epochTime = timeClient.getEpochTime();
      int currentHour = timeClient.getHours();
      int currentMinute = timeClient.getMinutes();
      //Get a time structure
      struct tm *ptm = gmtime ((time_t *)&epochTime); 
      int monthDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon+1;
      int currentYear = ptm->tm_year+1900;
      String currentDate = String(currentYear) + "" + String(currentMonth) + "" + String(monthDay);
      
      String dateOfTravel = currentDate + "/";
      String dateTimeOfTravel = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay) + " " + String(currentHour) + ":" + String(currentMinute);
      //String origin = "Tagbilaran City";
      //String destination = "Loboc";
      //String fare = String(MINIMUM_FARE);
      firebase.setString("TravelHistory/" + dateOfTravel + uid + "/DateTime", dateTimeOfTravel);
      firebase.setString("TravelHistory/" + dateOfTravel + uid + "/Origin", getPlace(origin));
      firebase.setString("TravelHistory/" + dateOfTravel + uid + "/Destination", getPlace(destination));
      firebase.setString("TravelHistory/" + dateOfTravel + uid + "/Fare", String(fare));
      firebase.setString("TravelHistory/" + dateOfTravel + uid + "/UID", uid);
    }else{
      lcdPrint("Card Status: ", "",0,0);
      lcdPrint("Inactive", "",0,1);
      delay(3000);
    }
  }else{
    lcdPrint("Insufficient Balance", "",0,0);
    lcdPrint("Please", "reload.",0,2);
    delay(3000);
  }
  //buzz for 1 sec
  digitalWrite(D0, HIGH);
  delay(100);
  digitalWrite(D0, LOW);
  lcd.clear();
}
