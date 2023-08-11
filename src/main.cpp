#include <FastLED.h>
#include <Candle.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <HTTPUpdateServer.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Jemoeder";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "everythingmustgo";

#define ONBOARD_LED 2
#define NUM_LEDS 12
#define CONFIG_VERSION "dem2"

// -- Method declarations.
void handleRoot();
void configSaved();

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

iotwebconf::ParameterGroup group1 = iotwebconf::ParameterGroup("group1", "Light settings");

iotwebconf::CheckboxTParameter candleParam =
   iotwebconf::Builder<iotwebconf::CheckboxTParameter>("candle").
   label("Be candles").
   defaultValue(true).
   build();

iotwebconf::IntTParameter<uint8_t> brightnessParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint8_t>>("brightnessParam").
  label("Brightness").
  defaultValue(255).
  min(1).
  max(255).
  step(1).
  placeholder("1..255").
  build();

// Set these to your desired credentials.
const char *ssid = "Jemoeder";
const char *password = "allmustgo";

CRGB leds[NUM_LEDS];
candle::Candle candles[NUM_LEDS];
candle::Candle saturation[NUM_LEDS];

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting up...");

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
      [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
      [](const char* userName, char* password) {
        httpUpdater.updateCredentials(userName, password);
      }
    );

    group1.addItem(&candleParam);
    group1.addItem(&brightnessParam);
    iotWebConf.addParameterGroup(&group1);
    iotWebConf.setConfigSavedCallback(&configSaved);

    // -- Initializing the configuration.
    iotWebConf.init();

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.on("/config", []{ iotWebConf.handleConfig(); });
    server.onNotFound([](){ iotWebConf.handleNotFound(); });

    pinMode(ONBOARD_LED, OUTPUT);

    FastLED.addLeds<ESPDMX>(leds, NUM_LEDS);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setTemperature(Candle);
    //  // THIS WORKS SO FUCKING WELL!!!

    for (size_t i=0; i<NUM_LEDS; i++) {
        // Reset LED values
        leds[i] = CRGB::Black;

        // Initialize candles
        candles[i].init(96, 255, 2, 96);

        // Initialize random saturation
        saturation[i].init(112, 255, 20, 62);

    }

    digitalWrite(ONBOARD_LED, HIGH);

    Serial.println("Ready.");
}


uint8_t hue = 0;
bool beACandle = true;

void loop()
{
    EVERY_N_MILLIS(16) {
      for (size_t i=0; i<NUM_LEDS; i++) {
          leds[i] = CHSV(
            add8(hue, mul8(i, 120)),
            beACandle ? saturation[i].get_next_brightness() : 255,
            beACandle ? candles[i].get_next_brightness() : 255
          );
      }
    }

    FastLED.show();

    iotWebConf.doLoop();

    EVERY_N_MILLIS(150) {
      hue++;
    }
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 01 Minimal</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void configSaved() {
  FastLED.setBrightness(brightnessParam.value());
  beACandle = candleParam.value();
}
