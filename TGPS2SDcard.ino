// this program uses the TinyGPSPlus by Mikal Hart to derive the actual Lat Long and Alt data
// it uses hdop to determine when the GPS has locked on after a cold start.
// while the software will display course and speed we need some of the other NMEA sentences and 
// that slows things down so just stick with Lat Long and Altitude.

#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <SD.h>
// It requires the use of SoftwareSerial, and assumes that you have a
// 9600-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;                        // The TinyGPSPlus object
SoftwareSerial ss(RXPin, TXPin);        // The serial connection to the GPS device

//  SD card attached to SPI bus as follows:
//  SDO - pin 11
//  SDI - pin 12
//  CLK - pin 13
//  CS - pin  10 
const int chipSelect = 10;
File myFile;

char fileName[16];
char gpsData[90];
char dateArr[16];
char timeArr[16];
char hdopArr[6];
char latArr[16];
char lngArr[16];
char altArr[10];

int nLines = 0;

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.print("Initializing SD card...");                             
  while (!SD.begin(chipSelect)) {                       // loops every 4 sec until we have the SD card initialised
	  Serial.println("initialization failed. retry!");
	  smartDelay(4000);	                                  // delay 4 sec and try again
  }

  Serial.println("initialization done.");

    myFile = SD.open("gps.txt", FILE_WRITE);
    Serial.println("Opened GPS.txt");
    smartDelay(1000);

    myFile.close();
    Serial.println("File Closed");

  ss.println("$PMTK314,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29");    // set all NMEA sentences

  while (gps.hdop.hdop() == 0.0) {                                    // hdop is zero until locked on
    smartDelay(2000);
    Serial.print(".");
  }

  Serial.println("Valid HDOP received");                              // when hdop non zero we have lock
  ss.println("$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29");    // set to only GGA sentence for speed
  ss.println("$PMTK220,200*2C");                                      // set to 5 hertz update
  Serial.println("set GPS to 5Hz GGA only sentence now.");            // let us know
}

void loop() {
  smartDelay(100);                                                     // is there any data

  if (++nLines % 5 == 0) {                                            // at 5 hertz this is 1 second of data
    myFile.close();                                                   // file may be open
    
    sprintf(fileName, "GPS_%i.txt", nLines / 5);                      // create a new file name
    myFile = SD.open(fileName, FILE_WRITE);                           // open it for writing
  };
    
  // sprintf(dateArr, "%02d/%02d/%02d %02d:%02d:%02d.%02d ", gps.date.day(), gps.date.month(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
  sprintf(dateArr, "%02d/%02d/%02d  ", gps.date.day(), gps.date.month(), gps.date.year());
  sprintf(timeArr, "%02d:%02d:%02d.%02d ", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
  dtostrf(gps.location.lat(),11, 6, latArr);                          // convert float to a string -> _buffer
  dtostrf(gps.location.lng(),12, 6, lngArr);                          // convert float to a string -> _buffer
  dtostrf(gps.altitude.meters(),7, 2, altArr);              	        // convert float to a string -> _buffer
  dtostrf(gps.hdop.hdop(), 5, 1, hdopArr);                             // convert float to a string -> _buffer

  sprintf(gpsData, "%s %s %s %s %s %s\n", dateArr, timeArr, latArr, lngArr, altArr, hdopArr);  // create the string for writing to sd card
  Serial.print(gpsData);                                              // show me
  myFile.print(gpsData);                                              // and write to SD card.

  smartDelay(50);                                                     // check if more data

  if (millis() > 5000 && gps.charsProcessed() < 10)                   // have we lost GPS
    Serial.println(F("No GPS data received: check wiring"));
}


static void smartDelay(unsigned long ms) {      // This custom version of delay() ensures that the gps object is being "fed".
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

