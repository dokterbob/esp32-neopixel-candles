#include <FastLED.h>
#include <Candle.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <HTTPUpdateServer.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Candles";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "candles";

#define ONBOARD_LED 2

#define NUM_STRIPS 5
#define NUM_LEDS_PER_STRIP 64
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP

// Use 3 LED's to make white
#define HUE_DEGREES 360/3

// How long to wait before increasing hue
#define HUE_DELAY 100

// How long to wait between candle frames
#define CANDLE_DELAY 16

#define CONFIG_VERSION "01"

// -- Method declarations.
void handleRoot();
void configSaved();

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

iotwebconf::ParameterGroup group1 = iotwebconf::ParameterGroup("group1", "Light settings");

iotwebconf::CheckboxTParameter darknessParam =
   iotwebconf::Builder<iotwebconf::CheckboxTParameter>("darkness").
   label("Be darkness").
   defaultValue(false).
   build();

iotwebconf::CheckboxTParameter candleParam =
   iotwebconf::Builder<iotwebconf::CheckboxTParameter>("candle").
   label("Be candles").
   defaultValue(true).
   build();

iotwebconf::CheckboxTParameter rainbowParam =
   iotwebconf::Builder<iotwebconf::CheckboxTParameter>("rainbow").
   label("Be a rainbow").
   defaultValue(true).
   build();

iotwebconf::IntTParameter<uint8_t> brightnessParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint8_t>>("brightness").
  label("Brightness").
  defaultValue(255).
  min(0).
  max(255).
  step(1).
  placeholder("0..255").
  build();

// Set these to your desired credentials.
const char *ssid = "Candles";
const char *password = "candles";

CRGB leds[NUM_LEDS];
candle::Candle candles[NUM_LEDS];
candle::Candle saturation[NUM_LEDS];

void setupFastLED() {
  // https://www.reddit.com/r/FastLED/comments/gpu1fv/best_pins_to_use_with_esp32/
  FastLED.addLeds<NEOPIXEL, 13>(leds, 0 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 14>(leds, 1 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 15>(leds, 2 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 18>(leds, 3 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 19>(leds, 4 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);

  FastLED.setCorrection(TypicalPixelString);
  FastLED.setTemperature(Candle);

  // Reset LED values
  FastLED.clear();
  FastLED.show();
}

void setupIotWebConf() {
  // -- Define how to handle updateServer calls.
  iotWebConf.setupUpdateServer(
    [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
    [](const char* userName, char* password) {
      httpUpdater.updateCredentials(userName, password);
    }
  );

  group1.addItem(&darknessParam);
  group1.addItem(&candleParam);
  group1.addItem(&rainbowParam);
  group1.addItem(&brightnessParam);
  iotWebConf.addParameterGroup(&group1);
  iotWebConf.setConfigSavedCallback(&configSaved);

  // -- Initializing the configuration.
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
}

void setupCandles() {
  for (size_t i=0; i<NUM_LEDS; i++) {
      // Initialize candles
      candles[i].init(96, 255, 2, 96);

      // Initialize random saturation
      saturation[i].init(112, 255, 20, 62);

  }
}

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting up...");

    setupIotWebConf();
    setupFastLED();
    setupCandles();

    // Signal that we're alive!
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);

    Serial.println("Ready.");
}


uint8_t hue = 0;
bool beACandle = true;
bool beARainbow = true;

void loop()
{
    EVERY_N_MILLIS(CANDLE_DELAY) {
      for (size_t i=0; i<NUM_LEDS; i++) {
        leds[i] = CHSV(
          beARainbow ? add8(hue, mul8(i, HUE_DEGREES)) : 0,
          beACandle ? saturation[i].get_next_brightness() : 255,
          beACandle ? candles[i].get_next_brightness() : 255
        );
      }
    }

    FastLED.show();

    iotWebConf.doLoop();

    EVERY_N_MILLIS(HUE_DELAY) {
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
  s += "<title>Je moeder (is the best light controller)</title>";
  s += "<style>.de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input,select{padding:5px;font-size:1em;} input{width:95%;} select{width:100%} input[type=checkbox]{width:auto;scale:1.5;margin:10px;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}</style>";
  s += "</head><body>";
  s += "<h1>Je moeder</h1><h2>(is the best light controller)</h2>";

  // Current config
  s += "<h3>Current config:<h3><ul>";
  if (darknessParam.value()) {
    s += "<li>I am darkness.</li>";
  }

  if (beACandle) {
    s += "<li>I am candles.</li>";
  }

  if (beARainbow) {
    s += "<li>I am a rainbow.</li>";
  }

  s += "<li>My brightness is ";
  s += brightnessParam.value();
  s += " of 255.</li>";
  s += "</ul>";

  s += "<p>Go to <a href='config'>configure page</a> to change settings.</p>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void configSaved() {
  if (darknessParam.value()) {
    FastLED.setBrightness(0);
  } else {
    FastLED.setBrightness(brightnessParam.value());
  }

  beACandle = candleParam.value();
  beARainbow = rainbowParam.value();
}
