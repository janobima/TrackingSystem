// Libraries
#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

// Pins
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

// Buffer
char replybuffer[255];

// Instances
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Fona instance
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t type;


void setup() {
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
  if (imeiLen > 0) {
    Serial.print("Module IMEI: ");
    //Serial.println(imei);
  }
  
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
  float GSMlatitude, GSMlongitude, speed_kph, heading, speed_mph, altitude, GPSlongitude, GPSlatitude;

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
  Serial.println(F("Checking for Cell network..."));
  if (fona.getNetworkStatus() == 1) {
    // network & GPRS? Great! Print out the GSM location to compare
    boolean gsmloc_success = fona.getGSMLoc(&GSMlatitude, &GSMlongitude);
    if (gsmloc_success) {
      Serial.print("GSMLoc lat:");
      Serial.println(GSMlatitude,6);
      Serial.print("GSMLoc long:");
      Serial.println(GSMlongitude,6);

      // Prepare request
      uint16_t statuscode;
      int16_t length;
      int lockStatus = 0 ; // 0= unlocked 1= locked
      int tampStatus = 0;  // 0= no tampering 1= tampering
      String url = "anon809.000webhostapp.com/index.php?ID=";
     
      String imei2 = imei;
      url += imei2;
      url += "&Bat=";
      url += String(vbat); 
      url += "&Lon=";
      url += String(GSMlatitude,7);
      url += "&Lat=";
      url += String(GSMlongitude,7);
      url += "&Stat=";
      url += lockStatus; // String(lockStatus);
      url += "&Tamp=";
      url += tampStatus ; 
 
      char buf[90];
      url.toCharArray(buf, url.length()+1);
      Serial.print("Request: ");
      Serial.println(buf);

      // Send location to Dweet.io
      if (!fona.HTTP_GET_start(buf, &statuscode, (uint16_t *)&length+1)) {
        Serial.println("Failed!");
      }
      while (length > 0) {
        while (fona.available()) {
          char c = fona.read();
// Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
          /* Wait until data register empty. */
          loop_until_bit_is_set(UCSR0A, UDRE0);
          UDR0 = c;
#else
          Serial.write(c);
#endif
          length--;
        }
      }
      fona.HTTP_GET_end();
      // Send an update every minute
      delay(60000);
    } else {
      Serial.println("GSM location failed...");
      Serial.println(F("Disabling GPRS"));
      fona.enableGPRS(false);
      Serial.println(F("Enabling GPRS"));
      if (!fona.enableGPRS(true)) {
        Serial.println(F("Failed to turn GPRS on"));
      }
    }
  }
}

