#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define MAX_COLORS 1280
#define PIN 12
// Adafruit_NeoPixel related variables
Adafruit_NeoPixel strip = Adafruit_NeoPixel(128, PIN, NEO_GRB + NEO_KHZ800);

// Update these with values suitable for your network.

const char* ssid = "Interpol";
const char* password = "yousureyouwanttoconnectthere?";
const char* mqtt_server = "m24.cloudmqtt.com";
const char* mqttUser = "mqttCloudUser";
const char* mqttPassword = "phrasewithsymbolsandnumbers";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
bool rainbow_mode = 1;
int rainbow_color = 0;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int hex2dec_digit(char c) {

    if (c >= '0' && c <= '9') {
      return c - '0';
    }

    if (c >= 'A' && c <= 'F') {
      return 10 + c - 'A';
    }

    if (c >= 'a' && c <= 'f') {
      return 10 + c - 'a';
    }

    Serial.println("Invalid hex digit, expecting 0-9A-Za-z.");
    return -1;
}

int color_hex2dec(char c1, char c2) {
    return 16*hex2dec_digit(c1) + hex2dec_digit(c2);
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (length == 7 && payload[0] == '#') {
      rainbow_mode = 0;
      int red = color_hex2dec(payload[1], payload[2]);
      int green = color_hex2dec(payload[3], payload[4]);
      int blue = color_hex2dec(payload[5], payload[6]);
      colorWipe(strip.Color(red, green, blue), 50);
  } else {
      Serial.print("Not a hex baby, going rainbow");
      rainbow_mode = 1;
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 11456);
  client.setCallback(callback);
  // Adafruit_NeoPixel part
  strip.begin();
  strip.show();
  strip.setBrightness(32);
  Serial.println("test");

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }

  if (rainbow_mode) {
    for(int i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbow_color) & 255));
    }
    strip.show();
    delay(20);
    rainbow_color = (rainbow_color + 1) % MAX_COLORS;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
