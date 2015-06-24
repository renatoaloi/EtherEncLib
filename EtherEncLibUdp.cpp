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
#include <stdlib.h>


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


char EtherEncLibUdp::read(void)
{
	return m_stack.read();
}

long EtherEncLibUdp::parseInt(void)
{
    long val = 0L;
    char sI[] = { 0, 0, 0, 0, 0, 0, 0 };
    char c = read();
    char counter = 0;
    while((c < '0' || c > '9') && counter++ < 7) c = read();
    counter = 0;
    while(c >= '0' && c <= '9' && counter < 7) { sI[counter++] = c; c = read(); }
    val = (long)atoi(sI);
    return val;
}

void EtherEncLibUdp::print(char c)
{
    m_stack.write(c);
}

void EtherEncLibUdp::print(float val)
{
    char floatBuffer[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    dtostrf(val,5,7,floatBuffer);
    print(floatBuffer);
}

// Print method
// Int values version of print method
void EtherEncLibUdp::print(unsigned int val)
{
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
}

// Print method
// Int values version of print method
void EtherEncLibUdp::print(int val)
{
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
}



void EtherEncLibUdp::print(char *rd)
{
    unsigned long limit = millis() + 100;
    do { 
	if (*(rd) != 0) 
	{
		print((char)*(rd)); 
	}
    } while(*(rd++) != 0 && limit > millis()); 
}




// Start processing the next available incoming packet
// Returns the size of the packet in bytes, or 0 if no packets are available
int EtherEncLibUdp::parsePacket()
{
	uint16_t msgSize = m_stack.established();
	if (msgSize)
	{
		// Ok, we got packet
		return msgSize;
	}
	return 0;
}

// Start building up a packet to send to the remote host specific in ip and port
// Returns 1 if successful, 0 if there was a problem with the supplied IP address or port
int EtherEncLibUdp::beginPacket(unsigned char *ip, uint16_t port)
{
	if (!m_stack.isSession())
	{
		m_stack.beginSend(ip, port);
		return 1;
	}
	return 0;
}

// Finish off this packet and send it
// Returns 1 if the packet was sent successfully, 0 if there was an error
int EtherEncLibUdp::endPacket()
{
	m_stack.send();
	return 1;
}

