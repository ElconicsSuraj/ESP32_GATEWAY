#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include "time.h"
#include <Preferences.h>

const char* ssid     = "AMP_WiFi";
const char* password = "amoghmp98";

// NTP server and offset
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;   // GMT+5:30 for India
const int   daylightOffset_sec = 0;

// Update interval
unsigned long lastSync = 0;
const unsigned long syncInterval = 3UL * 60UL * 60UL * 1000UL; // 3 hours
unsigned long lastWrite = 0;

Preferences prefs;
uint32_t counter = 0;

String currentLogFile = "";

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

String getDateStamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00-00-0000";
  }
  char buffer[15];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y", &timeinfo);
  return String(buffer);
}

String getTimeStamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00:00";
  }
  char buffer[15];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

String getLogFileName() {
  return "/log_" + getDateStamp() + ".csv";
}

void writeHeader(String filename) {
  if (!SD.exists(filename)) {
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      file.println("Date,Time,Counter");
      file.close();
      Serial.println("Header written to " + filename);
    }
  }
}

void appendLog() {
  String todayFile = getLogFileName();

  // If date changed → switch to new file
  if (todayFile != currentLogFile) {
    currentLogFile = todayFile;
    writeHeader(currentLogFile);
  }

  File file = SD.open(currentLogFile, FILE_APPEND);
  if (file) {
    String line = getDateStamp() + "," + getTimeStamp() + "," + String(counter);
    file.println(line);
    file.close();
    Serial.println(line);
  } else {
    Serial.println("Failed to open " + currentLogFile + " for appending");
  }
}

void setup() {
  Serial.begin(115200);

  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // --- Load counter from NVS ---
  prefs.begin("my-app", false); // namespace
  counter = prefs.getUInt("counter", 0); // default = 0
  Serial.printf("Restored counter = %u\n", counter);

  initWiFi();
  initTime();       // Sync NTP once at boot
  WiFi.disconnect(true); // turn off WiFi after sync
  lastSync = millis();

  // Setup today's log file
  currentLogFile = getLogFileName();
  writeHeader(currentLogFile);
}

void loop() {
  unsigned long now = millis();

  // Append every 1 second
  if (now - lastWrite >= 1000) {
    counter++;
    appendLog();

    // Save to flash every second (safe for ESP32 Preferences)
    prefs.putUInt("counter", counter);

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
