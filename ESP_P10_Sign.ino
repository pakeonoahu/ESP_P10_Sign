/*
   ESP Sign.
   1/27/18
   Require liberary from https://github.com/2dom/P10_matrix, change header file for 1/4 or 1/8 scan
   From FPP use curl in an event script to update sign:
   curl -d "messages=This is a new sign" -X POST http://<my.ip.address>/msg

   This program is provided free for you to use in any way that you wish,
   subject to the laws and regulations where you are using it.  Due diligence
   is strongly suggested before using this code.  Please give credit where due.

   The Author makes no warranty of any kind, express or implied, with regard
   to this program or the documentation contained in this document.  The
   Author shall not be liable in any event for incidental or consequential
   damages in connection with, or arising out of, the furnishing, performance
   or use of these programs.

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "ESPSign.h"
#include "helpers.h"
#include <P10_matrix.h>
#include <Ticker.h>
#include <ESP8266HTTPUpdateServer.h>
/* Web pages and handlers */
#include "page_header.h"
#include "page_root.h"
#include "page_admin.h"
#include "page_config_net.h"
#include "page_status_net.h"

/*************************************************/
/*      BEGIN - User Configuration Defaults      */
/*************************************************/
//P10 settings
// Pins for LED MATRIX
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
P10_MATRIX display( P_LAT, P_OE, P_A, P_B, P_C);

Ticker display_ticker;

String msg = "The Show Will Begin Again At 5:30 PM"; //your default sign.
int    textX   = display.width(),
       textMin = (msg.length() + 1) * -10,
       Loc  = 4,
       green     = 0,
       blue    = 255;

void display_updater()
{
  display.display(50); //brightness
  //display.displayTestPattern(70);
}
uint16_t myCYAN = display.color565(255, 255, 0);
uint16_t myColor = display.color565(255, 255, 255);
ESP8266HTTPUpdateServer httpUpdater;
/* REQUIRED */
const char ssid[] = "ssid";             /* Replace with your SSID */
const char passphrase[] = "password";   /* Replace with your WPA2 passphrase */

/*************************************************/
/*       END - User Configuration Defaults       */
/*************************************************/

void setup() {

  Serial.begin(115200);
  delay(10);
  display.begin();
  display.setTextWrap(false); // Allow text to run off right edge
  display.setTextSize(1);
  display.flushDisplay();
  Serial.println("");

  Serial.print(F("ESP Sign"));
  for (uint8_t i = 0; i < strlen_P(VERSION); i++)
    Serial.print((char)(pgm_read_byte(VERSION + i)));
  Serial.println("");

  /* Load configuration from EEPROM */
  EEPROM.begin(sizeof(config));
  loadConfig();

  /* Fallback to default SSID and passphrase if we fail to connect */
  int status = initWifi();
  if (status != WL_CONNECTED) {
    Serial.println(F("*** Timeout - Reverting to default SSID ***"));
    strncpy(config.ssid, ssid, sizeof(config.ssid));
    strncpy(config.passphrase, passphrase, sizeof(config.passphrase));
    status = initWifi();
  }

  //TODO: Change this to switch to softAP mode
  /* If we fail again, reboot */
  if (status != WL_CONNECTED) {
    Serial.println(F("**** FAILED TO ASSOCIATE WITH AP ****"));
    ESP.restart();
  }

  /* Configure and start the web server */
  httpUpdater.setup(&web);
  initWeb();
  /* Setup DNS-SD */
  /* -- not working
      if (MDNS.begin("esp8266")) {
          MDNS.addService("e131", "udp", E131_DEF_PORT);
          MDNS.addService("http", "tcp", HTTP_PORT);
      } else {
          Serial.println(F("** Error setting up MDNS responder **"));
      }
  */

}

int initWifi() {
  /* Switch to station mode and disconnect just in case */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.print(config.ssid);

  WiFi.begin(config.ssid, config.passphrase);

  uint32_t timeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (Serial)
      Serial.print(".");
    if (millis() - timeout > CONNECT_TIMEOUT) {
      if (Serial) {
        Serial.println("");
        Serial.println(F("*** Failed to connect ***"));
      }
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    if (config.dhcp) {
      Serial.print(F("Connected DHCP with IP: "));
    }  else {
      /* We don't use DNS, so just set it to our gateway */
      WiFi.config(IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]),
                  IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]),
                  IPAddress(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]),
                  IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3])
                 );
      Serial.print(F("Connected with Static IP: "));

    }
    Serial.println(WiFi.localIP());

  }

  return WiFi.status();
}

/* Read a page from PROGMEM and send it */
void sendPage(const char *data, int count, const char *type) {
  int szHeader = sizeof(PAGE_HEADER);
  char *buffer = (char*)malloc(count + szHeader);
  if (buffer) {
    memcpy_P(buffer, PAGE_HEADER, szHeader);
    memcpy_P(buffer + szHeader - 1, data, count);   /* back up over the null byte from the header string */
    web.send(200, type, buffer);
    free(buffer);
  } else {
    Serial.print(F("*** Malloc failed for "));
    Serial.print(count);
    Serial.println(F(" bytes in sendPage() ***"));
  }
}

/* Configure and start the web server */
void initWeb() {
  /* HTML Pages */

  web.on("/", []() {
    sendPage(PAGE_ROOT, sizeof(PAGE_ROOT), PTYPE_HTML);
  });
  web.on("/sign", HTTP_GET, handleSign);        // Call the 'handleSign' function when a client requests URI "/"
  web.on("/msg", HTTP_POST, handleMsg); // Call the 'handleMsg' function when a POST request is made to URI "/login"
  web.on("/admin.html", send_admin_html);
  web.on("/config/net.html", send_config_net_html);
  web.on("/status/net.html", []() {
    sendPage(PAGE_STATUS_NET, sizeof(PAGE_STATUS_NET), PTYPE_HTML);
  });
  /* AJAX Handlers */
  web.on("/rootvals", send_root_vals);
  web.on("/adminvals", send_admin_vals);
  web.on("/config/netvals", send_config_net_vals);
  web.on("/config/connectionstate", send_connection_state_vals);
  web.on("/status/netvals", send_status_net_vals);

  /* Admin Handlers */
  web.on("/reboot", []() {
    sendPage(PAGE_ADMIN_REBOOT, sizeof(PAGE_ADMIN_REBOOT), PTYPE_HTML);
    ESP.restart();
  });

  web.onNotFound([]() {
    web.send(404, PTYPE_HTML, "Page not Found");
  });
  web.begin();

  Serial.print(F("- Web Server started on port "));
  Serial.println(HTTP_PORT);
}

void handleSign() {                          // When URI / is requested, send a web page
  String content = "<form action=\"/msg\" method=\"POST\"><input type=\"text\" name=\"messages\" placeholder=\"messages\"></br><input type=\"submit\" value=\"Submit\"></form> <br>Current Sign: " + msg;
  content += "<hr><a href=\"/\" >Menu</a><br>";
  web.send(200, "text/html", content);
}

void handleMsg() {                         // If a POST request is made to URI /login
  msg = web.arg("messages");
  if (msg.length() <= 1) {
    msg = "The Show Will Begin Again At 5:30 PM";
    display.setTextSize(1);
    Loc = 4;
    textMin = (msg.length() + 1) * -10;
    String content = "<form action=\"/msg\" method=\"POST\"><input type=\"text\" name=\"messages\" placeholder=\"messages\"></br><input type=\"submit\" value=\"Submit\"></form> <br>New Sign is: " + msg;
    content += "<hr><a href=\"/\">Menu</a><br>";
    web.send(200, "text/html", content);
    return;
  }
  display.setTextSize(2);
  Loc = 1;
  textMin = (msg.length() + 1) * -12;
  String content = "<form action=\"/msg\" method=\"POST\"><input type=\"text\" name=\"messages\" placeholder=\"messages\"></br><input type=\"submit\" value=\"Submit\"></form> <br>New Sign is: " + msg;
  content += "<hr><a href=\"/\">Menu</a><br>";
  web.send(200, "text/html", content);
}


/* Initialize configuration structure */
void initConfig() {
  memset(&config, 0, sizeof(config));
  memcpy_P(config.id, CONFIG_ID, sizeof(config.id));
  config.version = CONFIG_VERSION;
  strncpy(config.name, "ESPSign", sizeof(config.name));
  strncpy(config.ssid, ssid, sizeof(config.ssid));
  strncpy(config.passphrase, passphrase, sizeof(config.passphrase));
  config.ip[0] = 0; config.ip[1] = 0; config.ip[2] = 0; config.ip[3] = 0;
  config.netmask[0] = 0; config.netmask[1] = 0; config.netmask[2] = 0; config.netmask[3] = 0;
  config.gateway[0] = 0; config.gateway[1] = 0; config.gateway[2] = 0; config.gateway[3] = 0;
  config.dhcp = 1;

}

/* Attempt to load configuration from EEPROM.  Initialize or upgrade as required */
void loadConfig() {
  EEPROM.get(EEPROM_BASE, config);
  if (memcmp_P(config.id, CONFIG_ID, sizeof(config.id))) {
    Serial.println(F("- No configuration found."));

    /* Initialize config structure */
    initConfig();

    /* Write the configuration structure */
    EEPROM.put(EEPROM_BASE, config);
    EEPROM.commit();
    Serial.println(F("* Default configuration saved."));
  } else {
    if (config.version < CONFIG_VERSION) {
      /* Major struct changes in V3 for alignment require re-initialization */
      if (config.version < 3) {
        char ssid[32];
        char pass[64];
        strncpy(ssid, config.ssid - 1, sizeof(ssid));
        strncpy(pass, config.passphrase - 1, sizeof(pass));
        initConfig();
        strncpy(config.ssid, ssid, sizeof(config.ssid));
        strncpy(config.passphrase, pass, sizeof(config.passphrase));
      }

      EEPROM.put(EEPROM_BASE, config);
      EEPROM.commit();
      Serial.println(F("* Configuration upgraded."));
    } else {
      Serial.println(F("- Configuration loaded."));
    }
  }

}

void saveConfig() {
  /* Write the configuration structre */
  EEPROM.put(EEPROM_BASE, config);
  EEPROM.commit();
  Serial.println(F("* New configuration saved."));
}

/* Main Loop */
void loop() {
  /* Handle incoming web requests if needed */
  web.handleClient();
  //clear screen background
  display.fillScreen(0);

  // Draw big scrolly text on top
  if (msg.length() > 1) {
    display.drawRect(0, 0, 32, 16, myCYAN);
  }
  display.setTextColor(myColor);
  display.setCursor(textX, Loc);
  display.print(msg);
  if ((--textX) < textMin) textX = display.width();
  green += 4;
  blue -= 4;
  if (green >= 255) green -= 255;
  if (blue <= 1) blue += 255;
  myColor = display.color565(255, green, blue);
  // Update display
  display_ticker.attach(0.001, display_updater);
  delay(40);

}
