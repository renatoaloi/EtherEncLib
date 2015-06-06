/*
  HelloWorld.ino - Sample Code consuming Library for Ethernet ENC29J60 Module.
                   Prints Html page with welcome message
  Created by Renato Aloi, August, 2013.
  Released into the public domain.

  -- May 2015
  Modified by Renato Aloi according to changes at library level, 
  made by Suchkov (SKA)

*/

#include <SPI.h>
#include <EtherEncLib.h>
#include <avr/pgmspace.h>

const PROGMEM char resp200Txt[] = {"HTTP/1.0 200 OK\n\rContent-Type: text/html\n\rPragma: no-cache\n\r\n\r"};
const PROGMEM char respHtml_0[] = {"<HTML><HEAD><TITLE>Arduino EtherEncLib.h</TITLE></HEAD><BODY>"};
const PROGMEM char respHtml_1[] = {"<h3>Welcome to EtherEncLib.h library!</h3>"};
const PROGMEM char respHtml_2[] = {"</BODY></HTML>"};
const PROGMEM char respHtml_N[] = {"Fixed text/html must be here for RAM sake!"};

EtherEncLib lib(80);

static unsigned char ipaddr[] = { 192, 168, 0, 125 };
static unsigned char macaddr[] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x25 };

void setup()
{
    Serial.begin(115200);

    pinMode(10,OUTPUT);	//--- ? -- SS pin must be output # by Renato Aloi
    
    //
    // Starting the lib
    //
    lib.begin(ipaddr, macaddr);
    
    Serial.println(F("EtherEncLib.h started!"));
}

void loop()
{
    //
    // Check if request has arrived
    //
    if (lib.available() )
    {
	// 
	// Print HTTP 200 OK
	//
	lib.print((char *)&resp200Txt[0],strlen_P(&resp200Txt[0]));

	//
	// Printing HTML header
	//
	lib.print((char *)&respHtml_0[0],strlen_P(&respHtml_0[0]));
		
	//
	// Printing welcome message
	// KEEP IT @ FLASH via PROGMEM (by Renato Aloi) May 2015
	//
	lib.print((char *)&respHtml_1[0],strlen_P(&respHtml_1[0]));
	
	//	
	// Printing HTML footer
	//
	lib.print((char *)&respHtml_2[0],strlen_P(&respHtml_2[0]));
		
	//
	// Closing connection
	// Put the EtherEncLib in listen state again
	//
        lib.close();
    }
}
