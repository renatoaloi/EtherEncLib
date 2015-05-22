/*
  HtmlButtonAllDigitalPortsAvailable.ino - Sample Code consuming Library for Ethernet ENC29J60 Module.
             Prints Html page with buttons for all digital port available on Arduino UNO
  Created by Renato Aloi, August, 2013.
  Released into the public domain.
  
  -- May 2015
  Modified by Renato Aloi according to changes at library level, made by Suchkov (SKA)

*/

#include <EtherEncLib.h>
#include <SPI.h>
#include <avr/pgmspace.h>

const PROGMEM char resp200Txt[] = {"HTTP/1.0 200 OK\n\rContent-Type: text/html\n\rPragma: no-cache\n\r\n\r"};
const PROGMEM char respLinkIni[] = {"<br><a href=/?pin="};
const PROGMEM char respLinkFim[] = {"!</a>"};

unsigned char arduinoUnoPins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  14, 15, 16, 17, 18, 19 } ;
                                   /*  Digital ports                         */   /*  Analogic ports   */

unsigned char arduinoUnoUsedPins[] = { 0, 1, 2, 10, 11, 12, 13 } ;

unsigned char arduinoUnoAvailablePins[10];
unsigned char arduinoUnoAvailablePinsState[10];

int arduinoUnoAvailableLength = 0;
int arduinoUnoAvailableIndex = -1;

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
    
    //
    // Configuring available pins
    //
    for (int i = 0; i < 20; i++)
    {
        char isThere = 0;
        for (int j = 0; j < 7; j++)
        {
            if (arduinoUnoPins[i] == arduinoUnoUsedPins[j])
            {
                isThere = 1;
                break;
            }
        }
        
        if (!isThere)
        {
            if (arduinoUnoAvailableLength < 10)
            {
                arduinoUnoAvailablePins[arduinoUnoAvailableLength] = arduinoUnoPins[i];
                arduinoUnoAvailablePinsState[arduinoUnoAvailableLength] = 0;
                arduinoUnoAvailableLength++;
            }
            else
            {
                break;
            }
        }
    }
    
    //
    // Configuring output pins
    //
    for (int i = 0; i < arduinoUnoAvailableLength; i++)
    {
        pinMode(arduinoUnoAvailablePins[i], OUTPUT);
        digitalWrite(arduinoUnoAvailablePins[i], LOW);
    }
    
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
        //
        
        // GET parameters
        char *params = lib.getParams();
        
        Serial.print("Params: ");
        Serial.println(params);
        
        lib.print((char *)&resp200Txt[0],strlen_P(&resp200Txt[0]));
        
        if ( strncmp(lib.getParams(),"favicon.ico",11) == 0 ) 
        {
          // printing favicon
          // for future version
        }
        else if (lib.isIndexHtml || lib.isGet)
        {
        
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
          lib.print("<p>");
          //
          // Checking parameters
          //
          if (strncmp(&params[1], "pin", 3) == 0 )
          {
              //
              // Checking for pin value
              //
              for (int i = 0; i < arduinoUnoAvailableLength; i++)
              {
                  configureStateBuffer(params, arduinoUnoAvailablePins[i], arduinoUnoAvailablePins[i] / 10);
              }
              
          }
          
          
          for (int i = 0; i < arduinoUnoAvailableLength; i++)
          {
              Serial.print(arduinoUnoAvailablePinsState[i], DEC);
              Serial.print(",");
          }
          Serial.println();
          
          
          for (int i = 0; i < arduinoUnoAvailableLength; i++)
          {
              configurePinAndHtmlDetail(arduinoUnoAvailablePinsState[i], arduinoUnoAvailablePins[i]);
          }
          
          lib.print("</p>");
          
          // Printing HTML footer
          lib.print("</BODY>");
          lib.print("</HTML>");
          
          //
          // Closing connection
          // Put the EtherEncLib in listen state again
          //
        }
        else if (lib.isPost)
        {
          // deal with post
        }
        else
        {
          // for debug
        }
        lib.close();
    }
}

unsigned char isAvailable(int j)
{
    unsigned char ret = 0;
    for (int i = 0; i < arduinoUnoAvailableLength; i++)
    {
        if (arduinoUnoAvailablePins[i] == j)
        {
            ret = 1;
            break;
        }
    }
    return ret;
}

void configureStateBuffer(char *params, int i, int mod)
{
    // till 99 pins
    if (!mod)
    {
        if (params[5] == i + 48 && isAvailable(i))
        {
            if (params[13] == 'H')
            {
                fillStatusBuffer(i, HIGH);
            }
            else
            {
                fillStatusBuffer(i, LOW);
            }
        }
    }
    else 
    {
        if (params[5] == mod + 48 && params[6] == (i - (mod * 10)) + 48 && isAvailable(i))
        {
            if (params[14] == 'H')
            {
                fillStatusBuffer(i, HIGH);
            }
            else
            {
                fillStatusBuffer(i, LOW);
            }
        }
    }
}

void fillStatusBuffer(int pin, unsigned char state)
{
    for (int i = 0; i < arduinoUnoAvailableLength; i++)
    {
        if (arduinoUnoAvailablePins[i] == pin)
        {
            arduinoUnoAvailablePinsState[i] = state;
        }
    }
}

void configurePinAndHtmlDetail(unsigned char state, int i)
{
    if (state)
    {
        digitalWrite(i, HIGH);
        lib.print((char *)&respLinkIni[0],strlen_P(&respLinkIni[0]));
        lib.print(i);
        lib.print("&state=L>Deactivate Pin ");
        lib.print(i);
        lib.print((char *)&respLinkFim[0],strlen_P(&respLinkFim[0]));
    }
    else
    {
        digitalWrite(i, LOW);
        lib.print((char *)&respLinkIni[0],strlen_P(&respLinkIni[0]));
        lib.print(i);
        lib.print("&state=H>Activate Pin ");
        lib.print(i);
        lib.print((char *)&respLinkFim[0],strlen_P(&respLinkFim[0]));
    }
}

