/*

Built for the LOLIN S3 MINI

For analog pin info, see page 16 of the s3 datasheet: 
https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

cyan, red, royal blue, blue, yellow, green, amber
thermistor is on pin 10 (3.3v ref)

@1kHz, each tick is 1ms

curl -X GET 'http://192.168.0.137/config'
curl -X POST  -H "Accept: application/json" -H "Content-Type: application/json" 'http://192.168.0.137/config' \
-d '{"play":[{"ticks":3000,"pwm":[50,0,0,0,0,0,0]},{"ticks":3000,"pwm":[0,50,0,0,0,0,0]}]}'
curl -X POST -H "Accept: application/json" -H "Content-Type: application/json" 'http://192.168.0.137/config' -d '{"iscale":0.9}'
*/

#include <Arduino.h>
#include "ConfigJson.h"
#include <WiFi.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Streaming.h>
#include <time.h>
#include <sunset.h>
#include "aWOT.h"
#include "StaticFiles.h"
//#include <AHTxx.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>

#define VERSION_STRING "glowstick 0.1"

// LED config
const uint8_t g_led_pins[] = {37,5,6,7,8,9,33};

const uint16_t PWM_FREQ = 500;
const uint8_t PWM_BITS = 10;
const uint16_t PWM_MAX = 1023;

// ADC2 is used by the wifi, so be sure to use an adc1 pin (1,2,3,4,5,6,7,8,9,10,14)
#define TEMP_SENSE_PIN 10

// AHTxx is i2c. Pin 35 is SDA and pin 36 is SCL on the lolin 23 mini.
//AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR); //sensor address, sensor type

// getLocalTime is called often, so reduce the default timeout from 5s to .5s
#define TIME_SYNC_TIMEOUT_MS 500
struct tm g_timeinfo;

// Set up the led timeseries array. Currently hard-coded, but can be made configurable.
const uint16_t g_num_frames = 10;
FRAME g_leds[g_num_frames];
FRAME g_cur_led;
ConfigJson g_config("/config.json");

SunSet sun;

bool isNightOn = false;
float g_sunrise_mpm = 0;
float g_sunset_mpm = 0;
float g_cur_mpm;

//float g_temperature;
//float g_humidity;
uint32_t g_temp_mv;
char g_buf[500];

WiFiServer server(80);
Application app;
 
void setup(){
  // disable brownout detector (needed for one of the esp32 boards)
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(100);
  Serial << F("Booting ") << VERSION_STRING << endl;
  delay(1000);

  // Set up LED PWMs
  for(uint8_t i; i<7; i++)
    ledcAttach(g_led_pins[i], PWM_FREQ, PWM_BITS);

  WiFiManager wifiManager;
  wifiManager.autoConnect("glowstick");
  Serial.println(WiFi.localIP());
  initApi();
  server.begin();

  g_config.start_fs();
  g_config.load_from_file();
  Serial << F("Configuring leds...") << endl;
  for (uint8_t i; i<7; i++) ledcWrite(i, g_config.led_init.raw[i]);

  sprintf(g_buf, "%s.local", g_config.hostname);
  MDNS.begin(g_buf);
  MDNS.addService("http", "tcp", 80);

  Serial.println(F("Configuring time..."));
  initTime();
  Serial.println("Starting OTA...");
  startOTA();

  // if(aht10.begin() != true) {
  //   Serial << F("AHT10 sensor failed to start!") << F("\n");
  // } else {
  //   g_temperature = aht10.readTemperature(); // read 6-bytes via I2C, takes 80 milliseconds
  //   g_humidity = aht10.readHumidity(AHTXX_USE_READ_DATA);
  //   Serial << F("AHT10 sensor initialized; temp=") << g_temperature << F("C, hum=") << g_humidity << F("%\n");
  // }
}


void loop() {
  static int curday = 32;
  static bool curdst = true;
  static uint32_t last_update = 0;
  static uint32_t last_mod_update = 0;

  uint32_t cur_us = micros();
  ArduinoOTA.handle();
  WiFiClient client = server.available();

  if (client.connected()) {
    app.process(&client);
  }

  // Update the sunset state, if needed
  if (curdst != g_timeinfo.tm_isdst || curday != g_timeinfo.tm_mday) {
    sun.setTZOffset(getUtcOffset());
    curdst = g_timeinfo.tm_isdst;
    sun.setCurrentDate(g_timeinfo.tm_year+1900, g_timeinfo.tm_mon+1, g_timeinfo.tm_mday);
    curday = g_timeinfo.tm_mday;
    g_sunrise_mpm = sun.calcSunrise();
    g_sunset_mpm = sun.calcSunset();
  }

  // Check temp every 3 sec
  if(cur_us - last_update >= 3000000) {
    //g_temperature = aht10.readTemperature(); // read 6-bytes via I2C, takes 80 milliseconds
    //g_humidity = aht10.readHumidity(AHTXX_USE_READ_DATA);
    g_temp_mv = analogReadMilliVolts(TEMP_SENSE_PIN);
    last_update = cur_us;
  }

  // Set LED PWM 
  if(cur_us - last_mod_update >= g_config.update_us) {
    for (uint8_t i; i<7; i++) {
      float val = g_config.iscale * g_cur_led.raw[i];
      if (val < 0.5) {
        ledcWrite(g_led_pins[i], 0);
        continue;
      }
      val += getRandomModulation(g_config.mod_amp, g_config.mod_step, i); //random(-g_mod * val, g_mod * val + 0.5);
      ledcWrite(g_led_pins[i], (uint32_t)constrain(round(val), 0, PWM_MAX));
    }
    last_mod_update = cur_us;
  }
  
  yield();
}

// Random-walk modulation
float getRandomModulation(float mod, float step, uint8_t i) {
    static float offsets[7] = {0,0,0,0,0,0,0};
    offsets[i] += constrain(random(-step*1000, step*1000 + 1) / 1000.0, -mod, mod);
    return offsets[i];
}
