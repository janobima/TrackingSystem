#include <ArduinoJson.h>
#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

// Pins
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
const int solenoidPin =9; 
const int wirePin = 8;     // the number of the pushbutton pin

// Buffer
int lockStatus = 0 ;
int wireState =0;
int tampStatus = 0;  // 0= no tampering 1= tampering
float GSMlatitude = 0 , GSMlongitude = 0, speed_kph, heading, altitude, GPSlongitude, GPSlatitude;

// Instances
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Fona instance
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t type;


void setup() {
  //Init Solenoid
  pinMode(solenoidPin, OUTPUT);
   pinMode(wirePin, INPUT);
   
  // Initi serial
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("Initializing)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }

  type = fona.type();
  Serial.println(F("FONA is OK"));
  switch (type) {
  case FONA808_V2:
    Serial.println(F("FONA 808 (v2)"));
    break;
  default:
    Serial.println(F("???"));
    break;
  }

  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  
  // Setup GPRS settings
  fona.setGPRSNetworkSettings(F("wholesale"));

  // Wait
  delay(1000);
  // Turn GPRS off & on again
  if (!fona.enableGPRS(false))
    Serial.println(F("Failed to turn off"));
  delay(1000);
  if (!fona.enableGPRS(true))
    Serial.println(F("Failed to turn on"));
  delay(1000);
}


void loop() {
  // Measure data
  

  char imei[15] = {0}; 
  uint8_t imeiLen = fona.getIMEI(imei);

  uint16_t vbat;
  fona.getBattPercent(&vbat);
  
  // GPS BASED
  boolean gps_success = fona.getGPS(&GPSlatitude, &GPSlongitude, &speed_kph, &heading, &altitude);
  if (gps_success) {
    Serial.print("GPS lat:");
    Serial.println(GPSlatitude, 6);
    Serial.print("GPS long:");
    Serial.println(GPSlongitude, 6);
  } else {
    Serial.println("Waiting for FONA GPS 3D fix...");
  }

  // Connection Based 
 /* Serial.println(F("Checking for Cell network..."));
  if (fona.getNetworkStatus() == 1) {
    // network & GPRS? Great! Print out the GSM location to compare
    boolean gsmloc_success = fona.getGSMLoc(&GSMlatitude, &GSMlongitude);
    if (gsmloc_success) {
      Serial.print("GSMLoc lat:");
      Serial.println(GSMlatitude,6);
      Serial.print("GSMLoc long:");
      Serial.println(GSMlongitude,6);
*/
      //==== Variables 
      uint16_t statuscode;
      int16_t length;
      //int lockStatus = 0 ; // 0= unlocked 1= locked
    
      String url = "http://locking-system.herokuapp.com/index.php?ID=";
      char buf[100];
      int lengthCheck =0;
      String bufferString; 
      int breakFlag = 0;
      int count; 
      
      //==== Prepare request
      String imei2 = imei;
      url += imei2;
      url += "&Bat=";
      url += String(vbat); 
      url += "&Lon=";
      url += String(GPSlongitude,7);//String(GSMlongitude,7);
      url += "&Lat=";
      url += String(GPSlatitude,7);//String(GSMlatitude,7);
      url += "&Stat=";
      url += lockStatus; 
      url += "&Tamp=";
      url += tampStatus ; 
 
      url.toCharArray(buf, url.length()+1);
      Serial.print("Request: ");
      Serial.println(buf);

     // Send location to Dweet.io
     if (!fona.HTTP_GET_start(buf, &statuscode, (uint16_t *)&length+1)) {
        Serial.println("Failed!");
     }
  
      while (length > 0) {
        count++;
        while (fona.available()) {
          char c = fona.read();
          bufferString += c; 
          Serial.print(c);
          length--;  
       }
      // delay(6);
       Serial.println(count);   // Might need to include a delay here 
      if(length == lengthCheck ){
        lengthCheck++;
        if(lengthCheck > 2000 || count > 10000  ){
          fona.HTTP_GET_end();
          break;
        }
      } else{
        lengthCheck = length;
      }
   }

    Serial.println("STRING: \r\n");
    Serial.println(bufferString);
    Serial.println("LENGTH: \r\n");
    Serial.println(bufferString.length());
    delay(1000);

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(bufferString);
    lockStatus = root[String("Stat")];
    Serial.print("STATUS: ");
    Serial.println(lockStatus);

  // Detecting lock status ------------------------------
    if(bufferString != ""  ){
      if(lockStatus==1){
        Serial.println("LOCK");
        digitalWrite(solenoidPin,HIGH);
    }
    else{
      Serial.println("UNLOCK");
      digitalWrite(solenoidPin,LOW);
    }
    }

    // Detecting wire cut --------------------------------
    wireState = digitalRead(wirePin);
  if (wireState == HIGH) {
    Serial.println("GOOD");
    tampStatus = 0; 
  } else {
        Serial.println("Cut wire");
    tampStatus = 1; 
  }
    
    // Send an update every minute
    fona.HTTP_GET_end();
    delay(1000);
    }
 // }
//}
