#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include "time.h"

const char* ssid     = "AMP_WiFi";
const char* password = "amoghmp98";

// NTP server and offset
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;   // GMT+5:30 for India
const int   daylightOffset_sec = 0;

// Update interval
unsigned long lastSync = 0;
const unsigned long syncInterval = 3UL * 60UL * 60UL * 1000UL; // 3 hours in ms
unsigned long lastWrite = 0;

File dataFile;
uint32_t counter = 0;

void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected");
}

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "NTP Time set: %d-%m-%Y %H:%M:%S");
}

void updateRTC() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("RTC updated from NTP");
  } else {
    Serial.println("Failed to update RTC");
  }
}

String getTimeStamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00-00-0000,00:00:00";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y,%H:%M:%S", &timeinfo);
  return String(buffer);
}

void writeHeader() {
  if (!SD.exists("/log.csv")) {
    File file = SD.open("/log.csv", FILE_WRITE);
    if (file) {
      file.println("Date,Time,Counter");
      file.close();
      Serial.println("Header written to log.csv");
    }
  }
}

void appendLog() {
  File file = SD.open("/log.csv", FILE_APPEND);
  if (file) {
    String line = getTimeStamp() + "," + String(counter);
    file.println(line);
    file.close();
    Serial.println(line);
  } else {
    Serial.println("Failed to open log.csv for appending");
  }
}

void setup() {
  Serial.begin(115200);

  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return;
  }

  initWiFi();
  initTime();       // Sync NTP once at boot
  WiFi.disconnect(true); // turn off WiFi after sync to save power
  lastSync = millis();

  writeHeader();
}

void loop() {
  unsigned long now = millis();

  // Append every 1 second
  if (now - lastWrite >= 1000) {
    counter++;
    appendLog();
    lastWrite = now;
  }

  // Resync RTC with NTP every 3 hours
  if (now - lastSync >= syncInterval) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    updateRTC();
    WiFi.disconnect(true);
    lastSync = now;
  }
}
