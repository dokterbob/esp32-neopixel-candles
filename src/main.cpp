#include <FastLED.h>
#include <Candle.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <HTTPUpdateServer.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Candles";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "candles12345";

#define STATUS_PIN 32

#define NUM_STRIPS 5
#define NUM_LEDS_PER_STRIP 64
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP

// Use 3 LED's to make white
#define HUE_DEGREES 360/3

#define CONFIG_VERSION "03"

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
   defaultValue(false).
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

iotwebconf::IntTParameter<uint8_t> candleSpeedParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint8_t>>("candle_speed").
  label("Candle speed (FPS)").
  defaultValue(100).
  min(1).
  max(255).
  step(1).
  placeholder("1..255").
  build();

iotwebconf::IntTParameter<uint8_t> hueSpeedParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint8_t>>("hue_speed").
  label("Hue speed (FPS)").
  defaultValue(10).
  min(1).
  max(255).
  step(1).
  placeholder("1..255").
  build();

iotwebconf::IntTParameter<uint8_t> hueRepeatParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint8_t>>("hue_repeat").
  label("Hue light repeat count").
  defaultValue(3).
  min(3).
  max(255).
  step(3).
  placeholder("3..255").
  build();

// Declare runtime configuration.
bool beACandle;
bool beARainbow;
uint8_t candleDelay;
uint8_t hueDelay;
uint8_t hueModulus;

// Declare leds.
CRGB leds[NUM_LEDS];
byte candle_hues[NUM_LEDS];
candle::Candle candles[NUM_LEDS];
candle::Candle saturation[NUM_LEDS];

void setupFastLED() {
  // https://www.reddit.com/r/FastLED/comments/gpu1fv/best_pins_to_use_with_esp32/
  FastLED.addLeds<WS2811, 23, RGB>(leds, 0 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, 22, RGB>(leds, 1 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, 21, RGB>(leds, 2 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, 19, RGB>(leds, 3 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, 18, RGB>(leds, 4 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);

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
  group1.addItem(&candleSpeedParam);
  group1.addItem(&hueSpeedParam);
  group1.addItem(&hueRepeatParam);

  iotWebConf.addParameterGroup(&group1);
  iotWebConf.setConfigSavedCallback(&configSaved);

  // -- Initializing the configuration.
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.init();

  // Set runtime configuration based on config.
  beACandle = candleParam.value();
  beARainbow = rainbowParam.value();
  candleDelay = 1000 / candleSpeedParam.value();
  hueDelay = 1000 / hueSpeedParam.value();
  hueModulus = 360 / hueRepeatParam.value();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
}

void setupCandles() {
  for (size_t i=0; i<NUM_LEDS; i++) {
      candle_hues[i] = random8(26, 60);

      candles[i].init(random8(16, 96), 255, random8(3, 8), random8(6, 22));

      // Initialize random saturation
      saturation[i].init(random8(112, 240), random8(241, 255), random8(10, 20), random8(22, 62));

  }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting up...");

    setupIotWebConf();
    setupFastLED();
    setupCandles();

    Serial.println("Ready.");
}

uint8_t hue = 0;
void loop()
{
    EVERY_N_MILLIS_I(candle_timer, candleDelay) {
      if (candle_timer.getPeriod() != candleDelay) candle_timer.setPeriod(candleDelay);

      for (size_t i=0; i<NUM_LEDS; i++) {
        leds[i] = CHSV(
          beARainbow ? add8(hue, mul8(i, hueModulus)) : candle_hues[i],
          beACandle ? saturation[i].get_next_brightness() : 255,
          beACandle ? candles[i].get_next_brightness() : 255
        );
      }
    }

    FastLED.show();

    iotWebConf.doLoop();

    EVERY_N_MILLIS_I(hue_timer, hueDelay) {
      if (hue_timer.getPeriod() != hueDelay) hue_timer.setPeriod(hueDelay);
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
  s += "<title>Candles</title>";
  s += "<style>.de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input,select{padding:5px;font-size:1em;} input{width:95%;} select{width:100%} input[type=checkbox]{width:auto;scale:1.5;margin:10px;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}</style>";
  s += "</head><body>";
  s += "<h1>Candles</h1>";

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


  s += "<li>My candle speed is ";
  s += candleSpeedParam.value();
  s += " FPS (delay: ";
  s += candleDelay;
  s += " ms).</li>";

  s += "<li>My hue speed is ";
  s += hueSpeedParam.value();
  s += " FPS (delay: ";
  s += hueDelay;
  s += " ms).</li>";

  s += "<li>My hue repeats every ";
  s += hueRepeatParam.value();
  s += " lights (modulo: ";
  s += hueModulus;
  s += " degrees).</li>";


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
  candleDelay = 1000 / candleSpeedParam.value();
  hueDelay = 1000 / hueSpeedParam.value();
  hueModulus = 360 / hueRepeatParam.value();
}
