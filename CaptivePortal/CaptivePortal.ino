#include "CaptivePortal.h"

enum STATES
{
    CONFIG_WIFI,
    WIFI_CONNECTED,
    DEFAULT_STATE
} STATE;

void setup()
{
    //
    esp_err_t err = FileSystem::read_nvs("SSID", WiFiSetup::ssid, 64);

    Serial.print("Error: ");
    Serial.println(esp_err_to_name(err));
    if (err == ESP_OK)
    {
        // 
        FileSystem::read_nvs("PSK", WiFiSetup::psk, 64);
        WiFi.mode(WIFI_STA);

        WiFi.begin(WiFiSetup::ssid, WiFiSetup::psk);
        delay(1000);
        Serial.println("Connecting.");
        int i = 0;
        while (WiFi.status() != WL_CONNECTED && i <= 41)
        {
            delay(200);
            i++;
            Serial.print(".");
        }
        if (i > 40)
        {
            // WiFi connection was unsuccessful.
            STATE = CONFIG_WIFI;
            WiFiSetup::initCaptivePortal();
        }
        else
        {
            STATE = WIFI_CONNECTED;
        }
        Serial.println(i);
    }
    else
    {
        // No WiFi Credentials were found.
        STATE = CONFIG_WIFI;
        WiFiSetup::initCaptivePortal();
    }
}

void loop()
{
    switch (STATE)
    {
    case CONFIG_WIFI:
    if (WiFiSetup::CaptivePortalHandleClient() == WL_CONNECTED)
    {
      WiFiSetup::endCaptivePortal();
      STATE = WIFI_CONNECTED;
    }
        break;
    case WIFI_CONNECTED:
    // Do anything that needs to occur after WiFi connection is established here
        STATE = DEFAULT_STATE;
    case DEFAULT_STATE:
    // Do normal stuff that needs to happen every loop.
        break;
    }
}