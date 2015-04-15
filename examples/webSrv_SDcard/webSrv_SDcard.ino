//--- made by SKA ---
//--- test EtherEncLib + SD-card with html-files ------
//-------------------------------------------------------------------
//--- don't forget to apply patch SD_soft_atmega328 to SD library ---
//--- and copy www dir into a root of SD card ---

#include <SPI.h>
#include <SD.h>
#include <EtherEncLib.h>
#include <avr/pgmspace.h>

#define BUFF_SIZE 99
#define HTML_ROOT "WWW/"

char htmlBuff[BUFF_SIZE+1];
static unsigned char ipaddr[] = { 192, 168, 2, 248 };
static unsigned char macaddr[] = { 0x00, 0x11, 0x22, 0x44, 0x00, 0x25 };
EtherEncLib eElib(80);

File fd;

const PROGMEM char resp200Txt[] = {"HTTP/1.0 200 OK\n\rContent-Type: text/html\n\rPragma: no-cache\n\r\n\r"};
//const PROGMEM char resp200Jpg[] = {"HTTP/1.0 200 OK\n\rContent-Type: image/jpeg\n\rPragma: no-cache\n\r\n\r"};
const PROGMEM char resp200Png[] = {"HTTP/1.0 200 OK\n\rContent-Type: image/png\n\rPragma: no-cache\n\r\n\r"};
const PROGMEM char resp200Gif[] = {"HTTP/1.0 200 OK\n\rContent-Type: image/gif\n\rPragma: no-cache\n\r\n\r"};

void setup() {
byte i;
  for ( i=0 ; i<BUFF_SIZE ; i++ ) htmlBuff[i] = 0x00;
  pinMode(10,OUTPUT);	//--- ?
  Serial.begin(115200);
  eElib.begin(ipaddr,macaddr);
  Serial.println("------ program start -----------");
  if ( SD.begin(4) )
    Serial.println("sd module initialized..");
  if ( SD.exists(HTML_ROOT"INDEX~1.HTM") ) {
    Serial.println("file index.html is present..");
  }
}

void loop() {
  if ( eElib.available() && eElib.analize() ) {

/*--- for debug purpose ---    Serial.print("+");
    if ( eElib.isIndexHtml ) Serial.print("-");
    else if ( eElib.isGet ) Serial.print("*");
    else if ( eElib.isPost ) Serial.print("_");
*/
      Serial.println(eElib.getParams());
    if ( strncmp(eElib.getParams(),"favicon.ico",11) == 0 ) {
      fd = SD.open(HTML_ROOT"VID_4.PNG");
      eElib.printHeader((char *)&resp200Png[0],strlen_P(&resp200Png[0]));
    } else if ( eElib.isIndexHtml ) {
      fd = SD.open(HTML_ROOT"INDEX~1.HTM");
      eElib.printHeader((char *)&resp200Txt[0],strlen_P(&resp200Txt[0]));
    } else {
//--- for DEBUG ---
        eElib.printHeader((char *)&resp200Gif[0],strlen_P(&resp200Gif[0]));
        fd = SD.open(HTML_ROOT"ABC.123");
        Serial.println("file www/ABC.123 does not exists..");
//---
    }
    if ( fd ) {
      unsigned long fSize = fd.size();
      byte j;
      while ( fSize ) {
        if ( fSize > BUFF_SIZE ) {
          for ( j=0; j<BUFF_SIZE; j++) {
            htmlBuff[j] = fd.read();
            fSize--;
          }
        } else {
          for ( j=0; j<fSize; j++) {
            htmlBuff[j] = fd.read();
          }
          fSize = 0;
          htmlBuff[j] = '\0';
        }
        eElib.print(htmlBuff);
      }
      fd.close();
    }
    eElib.close();
  }
}
