// CaptivePortal.h - Captive Portal WiFi Manager for ESP-32
// Mike Abbott - 2019
#ifndef WiFi_Captive_Portal_h
#define WiFi_Captive_Portal_h

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "FileSystem.h"
namespace WiFiSetup
{
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);

char ssid[64];
char psk[64];

// While the captive portal is running, this should be called every loop.
// It handles the DNS and Web Server.
int CaptivePortalHandleClient()
{
    server.handleClient();
    dnsServer.processNextRequest();
    return WiFi.status();
}

// This is where I'm currently generating the WiFi configuration page.
// This should allow us to easily swap it out with something less ugly.
String responseHTML(const int numNetworks)
{
    String response = "<!DOCTYPE html><html><body><form class=\"form-horizontal\"><fieldset><!-- Form Name --><legend>Form Name</legend><!-- Select Basic --><div class=\"form-group\"> <label class=\"col-md-4 control-label\" for=\"ssidBox\">WiFi Network</label> <div class=\"col-md-4\"> <select id=\"ssidBox\" name=\"ssid\" class=\"form-control\"> ";
    for (int i = 0; i < numNetworks; i++)
    {
        response += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option> ";
    }
    response += "</select> </div></div><!-- Password input--><div class=\"form-group\"> <label class=\"col-md-4 control-label\" for=\"passwordinput\">Password</label> <div class=\"col-md-4\"> <input id=\"passwordinput\" name=\"passwordinput\" type=\"password\" placeholder=\"\" class=\"form-control input-md\"> </div></div><!-- Button --><div class=\"form-group\"> <label class=\"col-md-4 control-label\" for=\"singlebutton\"></label> <div class=\"col-md-4\"> <button id=\"singlebutton\" name=\"singlebutton\" class=\"btn btn-primary\">Submit</button> </div></div></fieldset></form></body></html>";
    return response;
}

// Reads the credentials out of the file system and saves them to global variables.
void setSSID()
{
    char credentials[130];
    for (int i = 0; i < 130; i++)
    {
        credentials[i] = '\0';
    }
    FileSystem::readFile(SPIFFS, "/credentials.txt", credentials, 130);
    int i = 0;
    for (; credentials[i] != '\n'; i++)
    {
        ssid[i] = credentials[i];
    }
    i++;
    for (int p = 0; credentials[p + i]; p++)
    {
        psk[p] = credentials[p + i];
    }
}

// Handles the POST with the WiFi credentials, and serves up the WiFi Config page.
void handleConfigure()
{
    if (server.hasArg("ssid"))
    {
        server.arg("ssid").toCharArray(ssid, 64);
        server.arg("passwordinput").toCharArray(psk, 64);
        Serial.println(server.arg("ssid") + " " + server.arg("passwordinput"));
        WiFi.begin(ssid, psk);
        Serial.println("Connecting.");
        int i = 0;
        WiFi.mode(WIFI_STA);
        while (WiFi.status() != WL_CONNECTED && i <= 40)
        {
            delay(200);
            i++;
            Serial.print(".");
        }
        if (i > 40)
        {
            int numNetworks = WiFi.scanNetworks();
            // This should be changed.  But I'm also not sure that the client will be waiting after 40 seconds.
            // We probably want to send a response immediately?
            server.send(200, "text/html", responseHTML(numNetworks));
            Serial.println("\nFailed to connect.");
        }
        else
        {
            // WiFi connection was successful, write the credentials to the file system.
            char credentialStoreInfo[130];
            server.arg("ssid").toCharArray(ssid, 64);
            server.arg("passwordinput").toCharArray(psk, 64);
            // + "\n" + server.arg("passwordinput")).toCharArray(credentialStoreInfo, 130);
            Serial.println(ssid);
            Serial.println(psk);
            FileSystem::write_nvs("SSID", ssid);
            FileSystem::write_nvs("PSK", psk);
            //FileSystem::writeFile(SPIFFS, "/credentials.txt", credentialStoreInfo);
            Serial.println("\nSuccess!");
            // This doesn't actually work.  Either we need to fix it, or it needs to be removed.
            server.send(303, "text/plain"
                             "Location: http://google.com/");
            Serial.print("IPAddress: ");
            Serial.println(WiFi.localIP());
        }
    }
    else
    {
        Serial.println("Sending HTML");
        int numNetworks = WiFi.scanNetworks();
        server.send(200, "text/html", responseHTML(numNetworks));
    }
}

// Returns 404
void handleNotFound()
{
    server.send(404, "text/plain", "Not found");
}

// Initializes the Captive Portal.
void initCaptivePortal(const char *apSsid = "ESP32", const char *apPsk = "ConfigureMe")
{
    // THIS MUST BE CALLED IF WIFI CONNECTION WAS ATTEMPTED
    // PREVIOUSLY OR THE DHCP SERVER WILL CAUSE A CRASH
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    Serial.println(WiFi.softAP(apSsid, apPsk));
    // You can't configure the access point before it's finished starting.
    delay(200);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", handleConfigure);
    server.onNotFound(handleConfigure);
    server.begin();
}

// Closes the web and DNS server.
void endCaptivePortal()
{
    server.close();
    dnsServer.stop();
}

} // namespace WiFiSetup
#endif