/*
    EtherEncLibUdp.cpp - Library for Ethernet ENC28J60 Module.
    Created by Renato Aloi, August 27, 2013.

-- LICENSE INFO

    EtherEncLib - Ethernet ENC28J60 Library for Arduino
    Copyright (C) 2015  Renato Aloi
    renato.aloi@gmail.com -- http://www.seriallink.com.br

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

-- END LICENSE INFO

  Description ::
  UDP implementation over IP Stack

  What DO you get ::
  - UDP layer

*/

#include <Arduino.h>
#include "EtherEncLibUdp.h"
#include <avr/pgmspace.h>



// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Public Methods
//
// --------------------------------------------------------------------------------------------------------------------------------------------


// Begin method
// Configures the lib and load initial params
void EtherEncLibUdp::begin(unsigned char *ip, unsigned char *mac)
{
	m_stack.setMacAddr(&mac[0]);
	m_stack.setIpAddr(&ip[0]);
	m_stack.open(m_port);
}

// Available method
// Returns true when initial handshake is done
unsigned char EtherEncLibUdp::available(void)
{
	if (m_stack.established() && !m_stack.closing())
	{
		
	}
	return 0;
}

// Print method
// Sends data to client on byte at time
void EtherEncLibUdp::print(char c)
{
	// REV 3.1 by Renato Aloi (May 2015)
	//if (DEBUGLIBUDP) Serial.print(F("Now Really Printing: "));
	if (DEBUGLIBUDP) Serial.print(c);
	m_stack.write(c);
}

// Print method
// Sends data to client in small chunks
void EtherEncLibUdp::print(char *rd)
{
    // REV 3.1 by Renato Aloi (May 2015)
    // TODO:
    // it will hang up if it is not a string with null terminator!

    if (DEBUGLIBUDP) Serial.println(F("Printing1: "));
    do { 
	if (*(rd) != 0) 
	{
		print((char)*(rd)); 
	}
    } while(*(rd++) != 0);    
    if (DEBUGLIBUDP) Serial.println();
}

// Print method
// Int values version of print method
void EtherEncLibUdp::print(unsigned int val)
{
    if (DEBUGLIBUDP) Serial.println(F("Printing2: "));
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
    if (DEBUGLIBUDP) Serial.println();
}

// Print method
// Int values version of print method
void EtherEncLibUdp::print(int val)
{
    if (DEBUGLIBUDP) Serial.println(F("Printing2: "));
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
    if (DEBUGLIBUDP) Serial.println();
}

void EtherEncLibUdp::print(char *respondType, unsigned char dataLen) 
{
	// PROGMEM with 1-byte local buffer
	// REV 3.1 by Renato Aloi (May 2015)
	if (DEBUGLIBUDP) Serial.println(F("Printing3: "));
	char respond[] = { 0 };
	for (unsigned char i = 0; i < dataLen; i++)
	{
	  memcpy_P(respond,respondType+i,1);
	  print(respond[0]);
	}
	if (DEBUGLIBUDP) Serial.println();
	if (DEBUGLIBUDP) { Serial.print(F("FreeRAM: ")); Serial.print(freeRam()); }
	if (DEBUGLIBUDP) Serial.println();
}

// Close method
// Self-explaining
void EtherEncLibUdp::close(void)
{
    m_stack.close();
    //for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++)  m_httpData[i] = 0;
    if (DEBUGLIBUDP) { Serial.print(F("FreeRAM: ")); Serial.print(freeRam()); }
}

char EtherEncLibUdp::read(void)
{
	return m_stack.read();
}



