/*
  PostDebugger.ino - Test Sample Code consuming Library for Ethernet ENC28J60 Module.
             Tests POST functionality
  Created by Renato Aloi, Nov, 2014.
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
        // EtherEncLib deals with GET and POST methods
        // Obs: POST method not yet implemented (Yes!!! Now it is!)
        //
        
        // POST parameters
        char *params = lib.getParams();
        
        Serial.print("Params: ");
        Serial.println(params);
        
        //
        // Printing HTML header
        //
        lib.print("<HTML>");
        lib.print("<HEAD><TITLE>Arduino EtherEncLib.h</TITLE></HEAD>");
        lib.print("<BODY>");
        
        //
        // Printing welcome message
        //
        lib.print("<h3>Welcome to EtherEncLib.h library!</h3>");
        
        lib.print("<form method=POST>Type your name: ");
        lib.print("<input type=text name=nome ");
        if (strncmp(&params[0], "nome=", 5) == 0 )
        {
          lib.print(" value='");
          lib.print((char*)&params[5]);
          lib.print("' ");
        }
        lib.print(" />");
        lib.print("<input type=submit value='Go!' />");
        lib.print("</form>");
        
        
        if (strncmp(&params[0], "nome=", 5) == 0 )
        {
          lib.print("<H1>Hello ");
          lib.print((char*)&params[5]);
          lib.print("!</H1>");
        }
        
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

