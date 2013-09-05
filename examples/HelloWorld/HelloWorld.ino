/*
  HelloWorld.ino - Sample Code consuming Library for Ethernet ENC29J60 Module.
                   Prints Html page with welcome message
  Created by Renato Aloi, August, 2013.
  Released into the public domain.
*/

#include <EtherEncLib.h>

EtherEncLib lib(80);

static unsigned char ipaddr[] = { 192, 168, 1, 25 };
static unsigned char macaddr[] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x25 };

void setup()
{
    Serial.begin(115200);
    
    //
    // Starting the lib
    //
    lib.begin(ipaddr, macaddr);
    
    Serial.println("EtherEncLib.h started!");
}

void loop()
{
    //
    // Check if request has arrived
    //
    if (lib.available())
    {
        //
        // Printing HTML header
        //
        lib.print("<HTML>");
        lib.print("<HEAD><TITLE>Arduino EtherEncLib.h</TITLE></HEAD>");
        lib.print("<BODY>");
        
        //
        // Printing welcome message
        // KEEP IT SHORT! DO NOT print strings larger than 50 or 60 chars!
        //                Ideal between 15 and 20 chars.
        //
        lib.print("<h3>Welcome to EtherEncLib.h library!</h3>");
        
        // Printing HTML footer
        lib.print("</BODY>");
        lib.print("</HTML>");
        
        //
        // Closing connection
        // Put the EtherEncLib in listen state again
        //
        lib.close();
    }
}