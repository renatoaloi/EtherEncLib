//--- made by SKA ---
//--- test EtherEncLib 
// adapted by Renato Aloi
// May 2015
// removed SD Card part for future implementation

#include <SPI.h>
#include <EtherEncLib.h>
#include <avr/pgmspace.h>

static unsigned char ipaddr[] = { 192, 168, 0, 125 };
static unsigned char macaddr[] = { 0x00, 0x11, 0x22, 0x44, 0x00, 0x25 };

EtherEncLib eElib(80);

const PROGMEM char resp200Txt[] = {"HTTP/1.0 200 OK\n\rContent-Type: text/html\n\rPragma: no-cache\n\r\n\r"};

void setup() 
{
  pinMode(10,OUTPUT);	//--- ? -- SS pin must be output # by Renato Aloi
  Serial.begin(115200);
  eElib.begin(ipaddr,macaddr);
  Serial.println(F("------ program start -----------"));
  //Serial.println(F("NO SDCARD version")); // by Renato Aloi
}

void loop() {
  if ( eElib.available() ) 
  {
    Serial.println(eElib.getParams());
    eElib.print((char *)&resp200Txt[0],strlen_P(&resp200Txt[0]));
    if (eElib.isIndexHtml)
    {
      eElib.print("<HTML><body><H1>Hello World!</H1>");
      eElib.print("<form method=POST>");
      eElib.print("<input type=text name=nome />");
      eElib.print("<input type=submit value=OK />");
      eElib.print("</form></body>");
      eElib.print("</HTML>");
    }
    else if (eElib.isPost)
    {
      eElib.print("<HTML><body><H1>POST Params: ");
      eElib.print(eElib.getParams());
      eElib.print("</H1></body>");
      eElib.print("</HTML>");
    }
    else if (eElib.isGet)
    {
      eElib.print("<HTML><body><H1>GET Params: ");
      eElib.print(eElib.getParams());
      eElib.print("</H1></body>");
      eElib.print("</HTML>");
    }
    eElib.close();
  }
}
