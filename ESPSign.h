/*
* ESP Sign.
*  1/27/18
  
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

/* Name and version */
const char VERSION[] = "1.0";

#define HTTP_PORT       80      /* Default web server port */
#define EEPROM_BASE     0       /* EEPROM configuration base address */
#ifndef ESPSign_H
#define ESPSign_H
#define CONNECT_TIMEOUT 10000   /* 10 seconds */

/* Configuration ID and Version */
#define CONFIG_VERSION 3
const uint8_t CONFIG_ID[4] PROGMEM = { 'F', 'O', 'R', 'K'};

/* Configuration structure */
typedef struct {
    /* header */
    uint8_t     id[4];          /* Configuration structure ID */
    uint8_t     version;        /* Configuration structure version */

    /* general config */
    char        name[32];       /* Device Name */

    /* network config */
    char        ssid[32];       /* 31 bytes max - null terminated */
    char        passphrase[64]; /* 63 bytes max - null terminated */
    uint8_t     ip[4];
    uint8_t     netmask[4];
    uint8_t     gateway[4];
    uint8_t     dhcp;           /* DHCP enabled boolean */
    uint8_t     multicast;      /* Multicast listener enabled boolean */


} __attribute__((packed)) config_t;

/* Globals */

ESP8266WebServer    web(HTTP_PORT);
config_t            config;


const char PTYPE_HTML[] = "text/html";
const char PTYPE_PLAIN[] = "text/plain";

void saveConfig();
void sendPage(const char *data, int count, const char *type);

#endif
