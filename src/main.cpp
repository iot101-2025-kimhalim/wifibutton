#include <Arduino.h>
#include <ESPmDNS.h>
#include <ConfigPortal32.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <qrcode_espi.h>

TFT_eSPI display = TFT_eSPI();
QRcode_eSPI qrcode (&display);

char*               ssid_pfix = (char*)"WiFiButton";
String              user_config_html = ""
    "<p><input type='text' name='meta.relayIP' placeholder='RelayWeb Address'>";
char                relayIP[50];

#define             PUSH_BUTTON 43
volatile bool       buttonPressed = false;
volatile long       lastPressed = 0;

IRAM_ATTR void pressed() {
    if (millis() > lastPressed + 200) {
        lastPressed = millis();
        buttonPressed = true;
    }
}

WiFiClient          client;
char                toggleURL[200];

void setup() {
    Serial.begin(115200);
    display.begin();
    qrcode.init();

    char msg[] = "WIFI:S:WiFiButton;;;;";
    // char msg[] = "WIFI:S:CaptivePortal;T:WPA;P:yourpassword;;";
    qrcode.create(msg);

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());
    if(cfg.containsKey("meta") && cfg["meta"].containsKey("relayIP")) {
        sprintf(relayIP, (const char*)cfg["meta"]["relayIP"]);
    }
    pinMode(PUSH_BUTTON, INPUT_PULLUP);
    attachInterrupt(PUSH_BUTTON, pressed, FALLING);

    if (MDNS.begin("wifibutton")) {
        Serial.println("MDNS responder started");
    }
    sprintf(toggleURL, "http://%s/toggle", relayIP);
}

void loop() {
    if (buttonPressed) {
        buttonPressed = false;
        Serial.println("Button pressed");
        HTTPClient http;
        if (http.begin(client, toggleURL)) {
            int httpCode = http.GET();
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = http.getString();
                Serial.println(payload);
            }
            http.end();
        }
    }
}