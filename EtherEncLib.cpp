
/*
  EtherEncLib.cpp - Library for Ethernet ENC29J60 Module.
  Created by Renato Aloi, August 27, 2013.
  Released into the public domain.

  Description ::
  HTTP 1.0/1.1 GET/POST Listener for web browsing integration.

  What DO you get ::
  - ARP and ICMP treatment
  - HTTP GET and POST action over TCP-IP
  - Very responsive engine, and very fast!
  - HTTPd engine with "One Way" implementation
    - One way flow = SYS..ACK..PUSH..FIN
  - Possible to send various PUSH statments trhough print() function
    - SYS..ACK..PUSH1..PUSH2..PUSH3..PUSHn..FIN
  - Avoid the limit imposed by others ENC28J60 libraries
    - Other libs sends all data in one TCP PUSH packet, need large buffer
    - EtherEncLib.h sends data in little chunks, NO need large buffer
  - available(), print() and close() functions makes easy
      coding and "thinking outside the box"
  - getParams() extracts the command line parameters ex:
      http://192.168.1.25/?param1=0.231&param2=0.34
      will result: ?param1=0.231&param2=0.34
  - Timered loops to avoid get stucked waiting a client response


  What do you NOT get ::
  - This is a very limited implementation of TCP-IP protocol
    - Do not expect implementations like FTP, SMTP, etc
    - Only one aspect of TCP/HTTP service was implemented
  - OK! by Renato Aloi and SKA, May 2015
  - Faulty handle for diferent Source ports
    - The system will only handle one source port incomming connection
    - Other request arriving from other ports will be handled only at end of current request
    - Only FIN+ACK arriving from other ports will be handled
  - UDP NOT implemented

*/

#include <Arduino.h>
#include "EtherEncLib.h"
#include <avr/pgmspace.h>



// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Public Methods
//
// --------------------------------------------------------------------------------------------------------------------------------------------


// Begin method
// Configures the lib and load initial params
void EtherEncLib::begin(unsigned char *ip, unsigned char *mac)
{
	m_stack.setMacAddr(&mac[0]);
	m_stack.setIpAddr(&ip[0]);
	m_stack.open(m_port);
}

// Available method
// Returns true when initial handshake is done
unsigned char EtherEncLib::available(void)
{
	if (m_stack.established() && !m_stack.closing())
	{
		isGet = 0;
		isPost = 0;
		isIndexHtml = 0;
		if (!m_stack.buffering()) return analize();
	}
	return 0;
}

// Print method
// Sends data to client on byte at time
void EtherEncLib::print(char c)
{
	// REV 3.1 by Renato Aloi (May 2015)
	//if (DEBUGLIB) Serial.print(F("Now Really Printing: "));
	if (DEBUGLIB) Serial.print(c);
	m_stack.write(c);
}

// Print method
// Sends data to client in small chunks
void EtherEncLib::print(char *rd)
{
    // REV 3.1 by Renato Aloi (May 2015)
    // TODO:
    // it will hang up if it is not a string with null terminator!

    if (DEBUGLIB) Serial.println(F("Printing1: "));
    do { 
	if (*(rd) != 0) 
	{
		print((char)*(rd)); 
	}
    } while(*(rd++) != 0);    
    if (DEBUGLIB) Serial.println();
}

// Print method
// Int values version of print method
void EtherEncLib::print(unsigned int val)
{
    if (DEBUGLIB) Serial.println(F("Printing2: "));
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
    if (DEBUGLIB) Serial.println();
}

// Print method
// Int values version of print method
void EtherEncLib::print(int val)
{
    if (DEBUGLIB) Serial.println(F("Printing2: "));
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
    if (DEBUGLIB) Serial.println();
}

void EtherEncLib::print(char *respondType, unsigned char dataLen) 
{
	// PROGMEM with 1-byte local buffer
	// REV 3.1 by Renato Aloi (May 2015)
	if (DEBUGLIB) Serial.println(F("Printing3: "));
	char respond[] = { 0 };
	for (unsigned char i = 0; i < dataLen; i++)
	{
	  memcpy_P(respond,respondType+i,1);
	  print(respond[0]);
	}
	if (DEBUGLIB) Serial.println();
	if (DEBUGLIB) { Serial.print(F("FreeRAM: ")); Serial.print(freeRam()); }
	if (DEBUGLIB) Serial.println();
}

// Close method
// Self-explaining
void EtherEncLib::close(void)
{
    m_stack.close();
    for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++)  m_httpData[i] = 0;
    if (DEBUGLIB) { Serial.print(F("FreeRAM: ")); Serial.print(freeRam()); }
}

char EtherEncLib::read(void)
{
	return m_stack.read();
}

// GetParams
// Returns char array containing string of URL parameters, including character "?"
// like: ?param1=512&param2=xyz
char *EtherEncLib::getParams(void)
{
    return &m_httpData[0];
}

// moved near to print funcs by Renato Aloi

// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Priv. Funcs
//
// --------------------------------------------------------------------------------------------------------------------------------------------


// Function 'analize'
// returns TRUE when validate that GET or POST methods have came.
unsigned char EtherEncLib::analize(void)
{
	char c = m_stack.read();
	char *p1 = "GET /";
	char *p2 = "POST /";
	char method = -1;
	//char tmpData[BUFFER_PARAMS_LEN]; -- not needed, using m_httpData instead (by Renato Aloi, May 2015)
	char countEnter = 0;
	int  j = -1;
	bool isGET = false;
	bool isPOST = false;
//	char *resposta;

	for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++)  m_httpData[i] = 0;

	while(c != -1)
	{
		//if (DEBUGLIB) Serial.print(c);
		if (method == -1)
		{
			if (c == p1[0])
			{
				// is GET?
				isGET = true;
				for (unsigned i = 1; i <= 4; i++)
				{
					c = m_stack.read();
					if (c!=p1[i])
					{
						isGET = false;
						break;
					}
				}
				if (isGET)
				{
					// OK! We've got GET!
					if (DEBUGLIB) Serial.println(F("Achei GET"));
					method = 0;
					isGet = 1;
					isIndexHtml = 1;
				} else break;
			}
			else if (c == p2[0])
			{
				// is POST?
				isPOST = true;
				for (unsigned i = 1; i <= 5; i++)
				{
					c = m_stack.read();
					if (c!=p2[i])
					{
						isPOST = false;
						break;
					}
				}
				if (isPOST)
				{
					// OK! We've got POST!
					if (DEBUGLIB) Serial.println(F("Achei POST"));
					method = 1;
					isPost = 1;
				} else break;
			} else break;
		}
		else
		{
			if (method == 0) // Getting GET parameters
			{
				j = 0;
				if (c != ' ')
				{
					if (DEBUGLIB) Serial.println(F("GET OK!"));
					isIndexHtml = 0;
					while(c != ' ' && c != -1)
					{
						if (j < BUFFER_PARAMS_LEN - 1)
						{
							m_httpData[j] = c;
						}
						else if (j < BUFFER_PARAMS_LEN)
						{
							m_httpData[j] = '\0';
						}
						j++;
						c = m_stack.read();
						if (c == ' ' || c == -1) m_httpData[j] = '\0';
					}

					if (DEBUGLIB) Serial.print(F("Achei Parametros:"));
					if (DEBUGLIB) Serial.println(m_httpData);

					break;
				} else break;
			}
			else if (method == 1) // Getting POST parameters
			{
				j = 0;
//--- made by SKA ---
	char c2 = c;
	int v;
	while ( countEnter < 1 ) {
		v = 0;
		while ( c != '\r' && c2 != '\n' ) {
			while ( c != '\r' ) {
				c = m_stack.read();
				if ( c == -1 ) break;
//--- uncomment to see HTTP POST request header ---	Serial.print(c);
				v++;
			}
			c2 = m_stack.read();
			if ( c == -1 || c2 == -1 ) break;
			if ( c2 != '\n' ) c = c2;
			else {
//--- uncomment to see HTTP POST request header ---	Serial.println();
				countEnter++;
				}
		}
		if ( v > 1 ) countEnter = 0;
		c = '\t'; c2 = c;
	}
//--- made by SKA ---

				// Waiting for double enter \r\n\r\n
				if (countEnter > 0)
				{
					c = m_stack.read();
					// start gathering post data!
					if (DEBUGLIB) Serial.println(F("POST OK!"));
					while(c != '\r' && c != '\n' && c != -1)
					{
						if (j < BUFFER_PARAMS_LEN - 1)
						{
							m_httpData[j] = c;
						}
						else if (j < BUFFER_PARAMS_LEN)
						{
							m_httpData[j] = '\0';
						}
						j++;
						c = m_stack.read();
						if (c == '\r' || c == '\n' || c == -1) m_httpData[j] = '\0';
					}

					if (DEBUGLIB) Serial.print(F("Achei Parametros POST:"));
					for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++) {
						if (DEBUGLIB) Serial.print(m_httpData[i], DEC);
						if (DEBUGLIB && i < BUFFER_PARAMS_LEN - 1) Serial.print(F(", "));
					}
					if (DEBUGLIB) Serial.println();

					// then break the while, anyway
					break;
				}
			}
		}
		c = m_stack.read();
	}
	if (DEBUGLIB) Serial.println();

	if ( j == -1 ) {
		m_stack.close();
		return 0;
	}
	// removed by Renato Aloi 
	
	return 1;
}




