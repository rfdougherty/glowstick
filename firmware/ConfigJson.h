#pragma once

#ifndef ConfigJson_h
#define ConfigJson_h

#include "FS.h"
#include "SPIFFS.h"
#define MYFS SPIFFS
//#include <LittleFS.h>
//#define MYFS LittleFS
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
#include <Streaming.h>

struct FRAME  {
	union {
		struct {
      uint16_t l0;
      uint16_t l1;
      uint16_t l2;
      uint16_t l3;
      uint16_t l4;
      uint16_t l5;
      uint16_t l6;
		};
		uint16_t raw[7];
	};
	FRAME(){}
	FRAME(uint16_t n0, uint16_t n1, uint16_t n2, uint16_t n3, uint16_t n4, uint16_t n5, uint16_t n6){
		l0 = n0;
		l1 = n1;
		l2 = n2;
		l3 = n3;
		l4 = n4;
		l5 = n5;
		l6 = n6;
	}
};

class ConfigJson {
  public:
    float iscale;
    float mod_amp;
    float mod_step;
    uint32_t update_us;
    uint16_t temp_max;
    FRAME led_init;
    float latitude;
    float longitude;
    char ntp_server[32];
    char timezone[32];
    char hostname[32];

    ConfigJson(const char *path) {
      file_name = path;
    }

    void start_fs() {
      Serial.println(F("Starting FS..."));
      if (!MYFS.begin(true)) {
        Serial.println(F("ERROR: file system failed to start"));
      }
    }

    bool load_from_file() {
      set_defaults();
      if (!MYFS.exists(file_name)) {
        Serial.println(F("Config file does not exist; using defaults and saving."));
        save_to_file();
        return true;
      }
      File fp = MYFS.open(file_name, "r");
      if (!fp) {
        Serial << F("Reading config file ") << file_name << F("failed.") << F("\n");
        fp.close();
        return false;
      }
      // Allocate a temporary JsonDocument and deserialize
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, fp);
      if (error) {
        Serial.println(F("Failed to read file, using default configuration"));
        fp.close();
        return false;
      }
      doc_to_config(doc);      
      fp.close();
      Serial << F("Read config from file ") << file_name << F(".\n");
      return true;
    }

    void load_from_doc(JsonDocument doc) {
      doc_to_config(doc);
    }

    bool save_to_file() {
      if (!MYFS.begin()) {
        Serial.println("ERROR: file system failed to start");
        return false;
      }
      File fp = MYFS.open(file_name, "w");
      if (!fp) {
        Serial.println(F("Failed to create config file"));
        return false;
      }
      JsonDocument doc = config_to_doc();
      if (serializeJson(doc, fp) == 0) {
        Serial.println(F("Failed to write config to file"));
        fp.close();
        return false;
      }
      fp.close();
      Serial << F("Config file") << file_name << F(" saved.\n");
      Serial.println(doc["hostname"].as<const char*>());
      return true;
    }

    bool save_to_buffer(char buf[], uint16_t bufsz) {
      JsonDocument doc = config_to_doc();
      Serial.println(doc["ntp_server"].as<const char*>());
      int n_char = serializeJson(doc, buf, bufsz);
      return n_char;
    }

  private:
    const char *file_name;

    void set_defaults() {
      iscale = 1.0; // 0-1, applies a global intenstiry scale factor to LED PWM values
      mod_amp = 0.2; // 0-1, random modulation proportion
      mod_step = 0.05; // 0-1, random modulation step size proportion
      update_us = 2000;
      temp_max = 5000; // millivolts, turn off all leds if temp mv is >= this
      for (uint8_t i; i<7; i++) led_init.raw[i] = 0;
      latitude = 37.47889776969171;
      longitude = -122.24059783307233;
      strlcpy(ntp_server, "pool.ntp.org", sizeof(ntp_server));
      strlcpy(timezone, "PST8PDT,M3.2.0,M11.1.0", sizeof(timezone));
      strlcpy(hostname, "glowstick", sizeof(hostname));
    }

    void doc_to_config(JsonDocument doc) {
      if(doc["iscale"] != nullptr) iscale = doc["iscale"];
      if(doc["mod_amp"] != nullptr) mod_amp = doc["mod_amp"];
      if(doc["mod_step"] != nullptr) mod_step = doc["mod_step"];
      if(doc["upate_us"] != nullptr) update_us = doc["update_us"];
      if(doc["temp_max"] != nullptr) temp_max = doc["temp_max"];
      JsonArray arr = doc[F("led_init")];
      if(arr) for (uint8_t i; i<7; i++) led_init.raw[i] = arr[i];
      //arr = doc[F("minute_rgbw")];
      //if(arr) min_rgbw = CRGBW(arr[0], arr[1], arr[2], arr[3]);
      if(doc["latitude"] != nullptr) latitude = doc["latitude"];
      if(doc["longitude"] != nullptr) longitude = doc["longitude"];
      if(doc["ntp_server"]) strlcpy(ntp_server, doc["ntp_server"], sizeof(ntp_server));
      if(doc["timezone"]) strlcpy(timezone, doc["timezone"], sizeof(timezone));
      if(doc["hostname"]) strlcpy(hostname, doc["hostname"], sizeof(hostname));
    }

     JsonDocument config_to_doc() {
      JsonDocument doc;
      doc["iscale"] = iscale;
      doc["mod_amp"] = mod_amp;
      doc["mod_step"] = mod_step;
      doc["update_us"] = update_us;
      doc["temp_max"] = temp_max;
      JsonArray led_arr = doc["led_init"].to<JsonArray>();
      for(uint8_t i=0; i<7; i++) led_arr.add(led_init.raw[i]);
      doc["latitude"] = latitude;
      doc["longitude"] = longitude;      
      doc["ntp_server"] = ntp_server;
      doc["timezone"] = timezone;
      doc["hostname"] = hostname;
      doc.shrinkToFit();
      return doc;
    }
};
#endif