void initApi() {
  app.get("/config", &apiGetConfig);
  app.post("/config", &apiPostConfig);
  app.put("/iscale/:v", &apiSetScale);
  app.put("/mod/:amp/:step", &apiSetModulation);
  app.put("/update/:v", &apiSetUpdate);
  app.get("/leds", &apiGetLeds);
  app.put("/leds/:l0/:l1/:l2/:l3/:l4/:l5/:l6", &apiSetLeds);
  app.get("/sensors", &apiGetSensors);
  app.get("/reboot", &apiGetReboot);
  app.use(staticFiles());
}

// rounds a number to n decimal places
float roundn(float value, int n=1) {
   return (int)(value * 10 * n + 0.5) / (10.0 * n);
}

int getInt(Request &req, const char *name, int min, int max) {
  req.route(name, g_buf, 10);
  return constrain(strtol(g_buf, NULL, 10), min, max);
}

uint32_t getUint(Request &req, const char *name, int min, int max) {
  req.route(name, g_buf, 10);
  return constrain(strtoul(g_buf, NULL, 10), min, max);
}

float getFloat(Request &req, const char *name, float min, float max) {
  req.route(name, g_buf, 10);
  return constrain(atof(g_buf), min, max);
}

void apiGetConfig(Request &req, Response &res) {
  int n_char = g_config.save_to_buffer(g_buf, sizeof(g_buf));
  Serial << F("serialize config wrote ") << n_char << F(" chars.\n");
  res.set("Content-Type", "application/json");
  //res.write((uint8_t*)g_buf, sizeof(g_buf));
  res.print(g_buf);
}

void apiPostConfig(Request &req, Response &res) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, req);
  if (error) {
    Serial << F("deserializeJson() failed: ") << error.f_str() << "\n";
    return;
  }
  g_config.load_from_doc(doc);
  g_config.save_to_file();
  // reset timezone and sunrise/sunset info in case it changed.
  initTime();
  res.sendStatus(200);
}

void apiGetSensors(Request &req, Response &res) {
  JsonDocument doc;
  doc["temp_mv"] = g_temp_mv;
  //doc["temperature"] = roundn(g_temperature);
  //doc["humidity"] = roundn(g_humidity);
  doc["sunrise_mpm"] = g_sunrise_mpm;
  doc["sunset_mpm"] = g_sunset_mpm;
  doc["cur_mpm"] = g_cur_mpm;
  JsonArray led_arr = doc["leds"].to<JsonArray>();
  for(uint8_t i=0; i<7; i++) led_arr.add(g_cur_led.raw[i]);
  doc.shrinkToFit();
  int n_char = serializeJson(doc, g_buf, sizeof(g_buf));
  Serial << F("serialize sensors wrote ") << n_char << F(" chars.\n");
  res.set("Content-Type", "application/json");
  res.print(g_buf);
}

void apiSetScale(Request &req, Response &res) {
  g_config.iscale = getFloat(req, "v", 0.0, 1.0);
  snprintf(g_buf, sizeof(g_buf), "iscale=%0.3f", g_config.iscale);
  res.print(g_buf);
}

void apiSetModulation(Request &req, Response &res) {
  g_config.mod_amp = getFloat(req, "amp", 0.0, 1.0);
  g_config.mod_step = getFloat(req, "step", 0.0, 1.0);
  snprintf(g_buf, sizeof(g_buf), "mod_amp=%0.3f,mod_step=%0.3f", g_config.mod_amp, g_config.mod_step);
  res.print(g_buf);
}

void apiSetUpdate(Request &req, Response &res) {
  g_config.update_us = getUint(req, "v", 10, 9999999);
  snprintf(g_buf, sizeof(g_buf), "update_us=%d", g_config.update_us);
  res.print(g_buf);
}

void apiGetLeds(Request &req, Response &res) {
  JsonDocument doc;
  JsonArray led_arr = doc["leds"].to<JsonArray>();
  for(uint8_t i=0; i<7; i++) led_arr.add(ledcRead(g_led_pins[i])); //led_arr.add(g_cur_led.raw[i]);
  doc.shrinkToFit();
  int n_char = serializeJson(doc, g_buf, sizeof(g_buf));
  Serial << F("serialize wrote ") << n_char << F(" chars.\n");
  res.set("Content-Type", "application/json");
  res.print(g_buf);
}

void apiSetLeds(Request &req, Response &res) {
  g_cur_led.raw[0] = getUint(req, "l0", 0, PWM_MAX);
  g_cur_led.raw[1] = getUint(req, "l1", 0, PWM_MAX);
  g_cur_led.raw[2] = getUint(req, "l2", 0, PWM_MAX);
  g_cur_led.raw[3] = getUint(req, "l3", 0, PWM_MAX);
  g_cur_led.raw[4] = getUint(req, "l4", 0, PWM_MAX);
  g_cur_led.raw[5] = getUint(req, "l5", 0, PWM_MAX);
  g_cur_led.raw[6] = getUint(req, "l6", 0, PWM_MAX);
  snprintf(g_buf, sizeof(g_buf), "%d,%d,%d,%d,%d,%d,%d,pwm_max=%d", g_cur_led.l0, g_cur_led.l1, 
           g_cur_led.l2, g_cur_led.l3, g_cur_led.l4, g_cur_led.l5, g_cur_led.l6, PWM_MAX);
  res.print(g_buf);
}

void apiGetReboot(Request &req, Response &res) {
  snprintf(g_buf, sizeof(g_buf), "rebooting in 3 seconds...");
  res.print(g_buf);
  res.sendStatus(200);
  res.end();
  delay(100);
  ESP.restart();
}

void startOTA() {
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
}

void initTime() {
  Serial.println("Setting up time");
  configTime(0, 0, g_config.ntp_server);  // First connect to NTP server, with 0 TZ offset
  updateTime(TIME_SYNC_TIMEOUT_MS);
  Serial.println("Received time from NTP");
  // Now we can set the real timezone
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  // PST/PDT is PST8PDT,M3.2.0,M11.1.0
  Serial << F("Setting Timezone to ") << g_config.timezone << "\n";
  setenv("TZ", g_config.timezone, 1);
  tzset();
  updateTime(5000); 
  sun.setPosition(g_config.latitude, g_config.longitude, getUtcOffset());
}

bool updateTime(uint32_t timeout_ms) {
  if (!getLocalTime(&g_timeinfo, timeout_ms)) {
    Serial.println("Failed to update NTP time");
    return false;
  }
  return true;
}

uint32_t getEpochSecs() {
  time_t now;
  time(&now);
  return (uint32_t)now;
}

float getUtcOffset() {
  // returns utc_offset in hours. Computes it by getting gmtime and localtime, 
  // leaving the messy tz stuff to the posix lib.
  updateTime(500);
  time_t now;
  time(&now);
  struct tm lt, ut;
  localtime_r(&now, &lt);
  gmtime_r(&now, &ut);
  // year-day diff must be -1, 0 or 1
  float utcoff = 24.0 * (lt.tm_yday - ut.tm_yday) 
                 + (lt.tm_hour - ut.tm_hour) 
                 + (lt.tm_min - ut.tm_min) / 60.0;
  return utcoff;
}

static float getEpochFloat() {
  // Number of seconds since the epoch, but with ~microsecond precision
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (float)tv.tv_sec + tv.tv_usec/1e6;
}

static int64_t getEpochUs() {
  // Number of microseconds since the epoch
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
}