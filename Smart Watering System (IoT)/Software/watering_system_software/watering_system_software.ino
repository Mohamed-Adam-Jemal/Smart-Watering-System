#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

#define valve_pin 33
#define buzzer_pin 17
#define green_led_pin 13
#define red_led_pin 27

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE); //OLED Screen Initialization


const char SSID[] = ""; //Your Network SSID (name)
const char PASS[] = ""; //Your Network password
const char DEVICE_LOGIN_NAME[] = "";  //Your Device ID (Given by the Arduino IoT Cloud Platform) 
const char DEVICE_KEY[] = ""; // Your Device Secret Key (Given by the Arduino IoT Cloud Platform) 

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

unsigned long lastWeatherUpdate = 0;   // Variable to store the last weather update time
unsigned long lastTimeUpdate = 0;      // Variable to store the last Time update time
unsigned long valveStartTime = 0; // Variable to store the time when the valve was opened
bool valveOpend = false;           // Flag to indicate if the valve is currently open
const unsigned long valveMaxTime = 3600000; //The valve can be opened for a maximum of one hour, measured in milliseconds.
CloudTemperature temp_gauge;
CloudRelativeHumidity hum_gauge;
bool valve_button;



void setup()
{
  pinMode(valve_pin, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(red_led_pin, OUTPUT);

  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 3, "Status: OFF");
  
  red_Led(); // Turn on red LED at startup
  
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

}

void loop()
{
  ArduinoCloud.update();

  unsigned long currentMillis = millis();

  if (valveOpend) {
    // Check if the valve has been open for more than 2 minutes
    if (millis() - valveStartTime >= valveMaxTime) {
      valve_end(); // Close the valve
      valveOpend = false; // Update the flag
      valve_button = false;
    }
  }

  if (currentMillis - lastTimeUpdate >= 10000)
  {
    // Update the last update time
    lastTimeUpdate = currentMillis;

    // Perform the time update
    updateTime();
  }
  
  // Check if it's time to update weather
  if (currentMillis - lastWeatherUpdate >= 60000)
  {
    // Update the last update time
    lastWeatherUpdate = currentMillis;

    // Perform the weather update
    updateWeather();
  }
}



void initProperties()
{
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(valve_button, READWRITE, ON_CHANGE, onValveButtonChange);
  ArduinoCloud.addProperty(hum_gauge, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(temp_gauge, READ, ON_CHANGE, NULL);
}



void green_Led()
{
  digitalWrite(red_led_pin, LOW);
  digitalWrite(green_led_pin, HIGH);
}

void red_Led()
{
  digitalWrite(green_led_pin, LOW);
  digitalWrite(red_led_pin, HIGH);
}

void start_buzzer()
{
  tone(buzzer_pin, 3000);
  delay(100);
  noTone(buzzer_pin);
  delay(100);
  tone(buzzer_pin, 3000);
  delay(100);
  noTone(buzzer_pin);
  delay(3000);
}

void end_buzzer()
{
  tone(buzzer_pin, 3000);
  delay(500);
  noTone(buzzer_pin);
  delay(1000);
}

void valve_start()
{
  digitalWrite(valve_pin, HIGH);
  green_Led();
  start_buzzer();
  u8x8.drawString(0, 3, "           ");
  u8x8.drawString(0, 3, "Status: ON");
}

void valve_end()
{
  digitalWrite(valve_pin, LOW);
  red_Led();
  end_buzzer();
  u8x8.drawString(0, 3, "           ");
  u8x8.drawString(0, 3, "Status: OFF");
}

void updateWeather()
{
   HTTPClient http;

    //Set HTTP Request Final URL with Location and API key information
    //http.begin(URL + ApiKey + "&q=" + city);
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=35.1223&longitude=10.7422&current=temperature_2m,relative_humidity_2m");
    // start connection and send HTTP Request
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {

      //Read Data as a JSON string
      String JSON_Data = http.getString();
      //Serial.println(JSON_Data);

      //Retrieve some information about the weather from the JSON format
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, JSON_Data);
      JsonObject obj = doc.as<JsonObject>();

      //Display the Current Weather Info
      const uint8_t temp = obj["current"]["temperature_2m"].as<uint8_t>();
      const uint8_t hum = obj["current"]["relative_humidity_2m"].as<uint8_t>();
  

      Serial.print(temp);
      Serial.println(" C");
      Serial.print(hum);
      Serial.println(" %");

      if (temp>0 && hum>0 ) { //Prevent the temperature and humidity values from going down to 0 when there is a connection problem and keep the last values.
        temp_gauge = temp;
        hum_gauge = hum;
        
        char tempDisplay[20];
        char humDisplay[20];
      
        sprintf(tempDisplay, "Temp: %u C", temp);
        sprintf(humDisplay, "Humidity: %u", hum);

        u8x8.drawString(0, 5, tempDisplay);
        u8x8.drawString(0, 7, humDisplay);
      }

    } else {
      Serial.println("Getting the weather data...");
    }

    http.end();
}

void updateTime() {
   HTTPClient http;

    //Set HTTP Request Final URL with Location and API key information
    http.begin("https://timeapi.io/api/Time/current/zone?timeZone=Africa/Tunis");
    // start connection and send HTTP Request
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      //Read Data as a JSON string
      String JSON_Data = http.getString();

      //Retrieve some information about the time from the JSON format
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, JSON_Data);
      JsonObject obj = doc.as<JsonObject>();

      //Display the Current Time Info
      const char* time = obj["time"].as<const char*>();

      // Extract day, month, and year
      const uint8_t day = obj["day"].as<uint8_t>();
      const uint8_t month = obj["month"].as<uint8_t>();
      const uint16_t year = obj["year"].as<uint16_t>();

      // Convert day, month, and year to strings
      char dateDisplay[20];
      sprintf(dateDisplay, "%02d/%02d/%d", day, month, year);

      Serial.println(time);
      u8x8.clearLine(1); // Clear the line before printing
      u8x8.drawString(0, 1, time);
      Serial.println(dateDisplay);
      u8x8.drawString(6, 1, dateDisplay);
      
    } else {
      Serial.println("Error getting the time data...");
    }

    http.end();
}

void onValveButtonChange() // Since SmartValve is READ_WRITE variable, onSmartValveChange() is executed every time a new value is received from IoT Cloud.
{
  if (valve_button)
  {
    valve_start();
    valveStartTime = millis(); // Record the time when the valve was opened
    valveOpend = true;          // Set the flag to indicate the valve is open
  }
  else
  {
    valve_end();
    valveOpend = false;         // Reset the flag when the valve is closed
  }
}