#include <WiFiNINA.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <ArduinoJSON.h>
#include <Base64.h>
#include "rgb_lcd.h"
#include "arduino_secrets.cpp"
#include <HttpClient.h>


#define DHTPIN 2
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

rgb_lcd lcd; //LCD
const int pinAdc = 14; // sound

// NTP client
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 3600000);  // UTC time (0 offset)

int timezoneOffset = 1; // Default: UTC+1 (Copenhagen Standard Time)

// Function to check if it's daylight saving time (DST)
bool isDST() {
  // Placeholder for simplicity, assuming DST is active from March to October
  // You can replace this with more advanced methods if needed
  // Manually set the month for testing (replace with real date logic)
  int currentMonth = 1;  // January (Change this to simulate different months)
  
  return (currentMonth >= 3 && currentMonth <= 10);  // DST is between March and October
}

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
char server[] = "192.168.1.100"; // Server IP

//HttpClient http;
WiFiClient client;
String postData;
String postData1;
String postData2;
unsigned long lastTempSentTime = 0;
unsigned long lastHumSentTime = 0;
unsigned long lastNoiSentTime = 0;
unsigned long lastMillis = 0;



char authToken;

String postVariable1 = "Temp =";
String postVariable2 = "Hum =";
String postVariable3 = "Noi =";

void setup() {

  Serial.begin(9600);
  lcd.begin(16,2);
  uint32_t delayMS;
  delay(1000);

    // Connect to WiFi
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  timeClient.begin();
  
  // Check if it's daylight saving time and update timezone offset
  if (isDST()) {
    timezoneOffset = 2; // UTC+2 during daylight saving time
  }

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
 
  Serial.begin(9600);
 
// Initialize DHT sensor
  dht.begin();
  Serial.println(F("DHT11 Humidity&Tempature Sensor "));
  // Print temperature sensor details
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  // Display sensor details on the LCD
  lcd.clear();
  lcd.print("Initializing...");

  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

}

void loop() {

timeClient.update();
//time
  long localTime = timeClient.getEpochTime() + timezoneOffset * 3600;
  int hours = (localTime % 86400L) / 3600;
  int minutes = (localTime % 3600) / 60;
  int seconds = localTime % 60;
  Serial.print("LocalTime:");
  Serial.print(hours);
  Serial.print(":");
  if (minutes < 10) {
    Serial.print("0");
  }
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) {
    Serial.print("0");
  }
  Serial.println(seconds);
  delay(1000);

  // Read temperature and humidity from DHT sensor
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  float Savetemp = event.temperature;
  dht.humidity().getEvent(&event);
  float SaveHum = event.relative_humidity;
if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(Savetemp);
    Serial.println(F("°C"));
  }
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(SaveHum);
    Serial.println(F("%"));
  }
 int reading = analogRead(DHTPIN);
  float voltage = reading * 5.0;
  voltage /= 1024.0;
  float temperature = (voltage - 0.5) * 100 ;
  float temperatureF = (temperature * 9.0 / 5.0) + 32.0;

  // Read noise from analog sensor
  long noise = 0;
  for (int i = 0; i < 32; i++) {
    noise += analogRead(pinAdc);
  }
  noise >>= 5;
  float Savenoi = noise;
  Serial.print(F("Noise: "));
  Serial.println(Savenoi);
  delay(1000);
  lcd.clear();
unsigned long currentMillis = millis();

if (isnan(Savetemp) || isnan(SaveHum)) {
    Serial.println("Failed to read from DHT sensor");
    return;
}

lcd.clear();  // Clear the display before printing new data
  //LCD Show data
  // First line
lcd.setCursor(0,0); // Set cursor to the beginning of the first line
lcd.print(F("Tem:"));
lcd.print(Savetemp);
lcd.print(F("C")); 
lcd.print(F("No:"));
lcd.print(noise);   
// Second line
lcd.setCursor(0,1); // Set cursor to the beginning of the second line
lcd.print(F("Hum:"));
lcd.print(SaveHum);
lcd.print(F("%"));


// Build the JSON payload for POST request
String postData = " {"
                  "\"fkSensor\": 5, "
                  "\"fkDevice\": 7, "
                  "\"fkLogStatus\": 1, "
                  "\"value\": " + String(Savetemp) +
                  "}";

// Ensure the temperature value is valid
if (isnan(Savetemp)) {
    Serial.println("Invalid temperature value! Aborting POST request.");
    return;
}

// Send POST request
if (client.connect(server, 80)) {
    Serial.println("Connected to server");

    // Prepare and send the POST request headers
    client.println("POST /api/logs HTTP/1.1");
    client.println("Host: 192.168.1.100");
    client.println("Authorization: Basic QXJkdWlub1Vub3IyOlRlc3Rpbmc=");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();

    // Send the actual data in the body
    client.print(postData);

    // Read the server response
    String response = "";
    while (client.available()) {
        char c = client.read();
        response += c;
    }

    // Output the server response
    Serial.println(response);

    client.stop();  // Stop the client connection
} else {
    Serial.println("Connection failed!");
}

  // Wait before sending new data
  delay(5000);

  // Build the JSON payload for POST request
String postData1 = " {"
                  "\"fkSensor\": 4, "
                  "\"fkDevice\": 7, "
                  "\"fkLogStatus\": 1, "
                  "\"value\": " + String(SaveHum) +
                  "}";

// Ensure the temperature value is valid
if (isnan(SaveHum)) {
    Serial.println("Invalid temperature value! Aborting POST request.");
    return;
}

// Send POST request
if (client.connect(server, 80)) {
    Serial.println("Connected to server");

    // Prepare and send the POST request headers
    client.println("POST /api/logs HTTP/1.1");
    client.println("Host: 192.168.1.100");
    client.println("Authorization: Basic QXJkdWlub1Vub3IyOlRlc3Rpbmc=");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(postData1.length());
    client.println();

    // Send the actual data in the body
    client.print(postData1);

    // Read the server response
    String response = "";
    while (client.available()) {
        char c = client.read();
        response += c;
    }

    // Output the server response
    Serial.println(response);

    client.stop();  // Stop the client connection
} else {
    Serial.println("Connection failed!");
}


  // Wait before sending new data
  delay(5000);

// Build the JSON payload for POST request
String postData2 = " {"
                  "\"fkSensor\": 6, "
                  "\"fkDevice\": 7, "
                  "\"fkLogStatus\": 1, "
                  "\"value\": " + String(Savenoi) +
                  "}";

// Ensure the temperature value is valid
if (isnan(Savenoi)) {
    Serial.println("Invalid temperature value! Aborting POST request.");
    return;
}

// Send POST request
if (client.connect(server, 80)) {
    Serial.println("Connected to server");

    // Prepare and send the POST request headers
    client.println("POST /api/logs HTTP/1.1");
    client.println("Host: 192.168.1.100");
    client.println("Authorization: Basic QXJkdWlub1Vub3IyOlRlc3Rpbmc=");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(postData2.length());
    client.println();

    // Send the actual data in the body
    client.print(postData2);

    // Read the server response
    String response = "";
    while (client.available()) {
        char c = client.read();
        response += c;
    }

    // Output the server response
    Serial.println(response);

    client.stop();  // Stop the client connection
} else {
    Serial.println("Connection failed!");
}

  // Wait before sending new data
  delay(5000);


Serial.println("get before");
 //Prepare the GET URL with query parameters
 String url ="192.168.1.100/api/logs?Temp=" + String(Savetemp) + "&Hum=" + String(SaveHum) + "&Noi=" + String(Savenoi);
 Serial.println("URL: "+url);
 if (client.connect(server, 80)) {
    Serial.println("GET:Connected to server "); 
} else {
    Serial.println("Connection server failed");
}

// Connect to the server and send GET request
if (client.connect(server, 80)) {
   
      client.println("Host: 192.168.1.100/api/logs");
     client.println("Authorization: Basic QXJkdWlub1Vub3IyOlRlc3Rpbmc=");
    client.println("Content-Type: application/json");
    client.print("Connection: close\r\n\r\n");
    
     // Read the server response
    String response = "";
    while (client.available()) {
        char c = client.read();
        response += c;
    }


    // Close the connection
    client.stop();
} else {
    Serial.println("GET request failed: Could not connect to server");
}


if (hours >= 18 || hours < 7) {
    // Temperature section
    if (currentMillis - lastTempSentTime >= 6000) {
        lastTempSentTime = currentMillis;
        lcd.setCursor(0, 1);  // Set cursor to start of the first line
        if (Savetemp > 18) {
            lcd.print(F("Alert:Temp_high!"));
            Serial.println(F("Alert: Temp too high!"));
            lcd.setRGB(255, 0, 0); 
        } else if (Savetemp < 7) {
            lcd.print(F("Alert:Temp_low!"));
            Serial.println(F("Alert: Temp too low!"));
            lcd.setRGB(0, 0, 255); 
        } else {
        
        }
         postData = " {"
                    "\"fkSensor\": 5, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(Savetemp) +
                    "}";
    }

    // Humidity section
    if (currentMillis - lastHumSentTime >= 9000) {
        lastHumSentTime = currentMillis;
        lcd.setCursor(0, 0);  // Set cursor to start of the second line
        if (SaveHum > 50) {
          lcd.clear();
            lcd.print(F("Alert:Hum_high!"));
             Serial.println(F("Alert: Humidity too high!"));
            lcd.setRGB(255, 165, 0); 
        } else if (SaveHum < 30) {
            lcd.clear();
            lcd.print(F("Alert:Hum_low!"));
              Serial.println(F("Alert: Humidity too low!"));
            lcd.setRGB(0, 0, 255); 
        } else {
         
        }
         postData = " {"
                    "\"fkSensor\": 4, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(SaveHum) +
                    "}";
    }

    // Noise section
    if (currentMillis - lastNoiSentTime >= 9000) {
        lastNoiSentTime = currentMillis;
        lcd.setCursor(0, 1);  // Set cursor to start of the second line
        if (Savenoi > 500) {
            lcd.print(F("Alert:TooLoud!"));
            Serial.println(F("Alert: too Loud!"));
            lcd.setRGB(255, 165, 0); 
        } else {
         
        }
         postData = " {"
                    "\"fkSensor\": 6, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(Savenoi) +
                    "}";
    }

    // Time interval for checking
    if (millis() - lastMillis >= 10000) {
        lastMillis = millis();
    }
} else 
  {
    // Alternative condition for the time range outside 7AM-6PM
    if (currentMillis - lastTempSentTime >= 6000) {
        lastTempSentTime = currentMillis;
        lcd.setCursor(0, 1);  // Reset cursor to line 0
        if (Savetemp > 18) {
            lcd.print(F("Alert:Temp_high!"));
            Serial.println(F("Alert: Temp too high!"));
            lcd.setRGB(255, 0, 0); 
        } else if (Savetemp < 15) {
            lcd.print(F("Alert:Temp_low!"));
             Serial.println(F("Alert: Temp too low!"));
            lcd.setRGB(0, 0, 255); 
        } else {
         
        }
       postData = " {"
                    "\"fkSensor\": 5, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(Savetemp) +
                    "}";
    }

    if (currentMillis - lastHumSentTime >= 9000) 
    {
        lastHumSentTime = currentMillis;
        lcd.setCursor(0, 1);  // Set cursor to start of the second line
        if(SaveHum > 30) {
            lcd.print(F("Alert:Hum_high!"));
              Serial.println(F("Alert: Humidity too high!"));
            lcd.setRGB(255, 165, 0); 
        } else if (SaveHum < 25) {
            lcd.print(F("Alert:Hum_low!"));
            Serial.println(F("Alert: Humidity too low!"));
            lcd.setRGB(0, 0, 255); 
        } else {
     
        }
         postData = " {"
                    "\"fkSensor\": 4, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(SaveHum) +
                    "}";
    }

    if (currentMillis - lastNoiSentTime >= 9000) 
    {
        lastNoiSentTime = currentMillis;
        lcd.setCursor(0, 1);  // Set cursor to start of the second line
        if (Savenoi > 500) {
            lcd.print(F("Alert:Too Loud!"));
            Serial.println(F("Alert: too Loud!"));
            lcd.setRGB(255, 165, 0); 
        } else {
        
        }
         postData = " {"
                    "\"fkSensor\": 6, "
                    "\"fkDevice\": 7, "
                    "\"fkLogStatus\": 1, "
                    "\"value\": " + String(Savenoi) +
                    "}";
    }
    }

    if (millis() - lastMillis >= 10000) 
    {
        lastMillis = millis();
    }
  }






