/*
 UDP Stack for New EtherEnc based on Enc28Core
 by Renato Aloi NOV 2014
 * seriallink.com.br
 */

/*
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

*/

#ifndef UDPSTACK_H
#define UDPSTACK_H

#include <Arduino.h>
#include <stdint.h>
//--- made by SKA ---
#include <SPI.h>

#define DATA_SIZE_HARDWARE 	1024 // we can go up to 1500, but for first tests, lets keep it 1K
#define DATA_SIZE		58  // only for UDP Header = ETH_HEADER_LEN_V + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + 4
				    // for data part: enc28's hadware buffer
				    // by Renato Aloi (May 2015)
#define MAC_SIZE		6
#define IP_SIZE			4
#define DEBUG			0
#define DEBUGLT			1
#define DEBUGTST		0 // Debugging why only working with DEBUG flag on. 
				  //now it is ok, see delay at close function
#define ETH_ARP_LEN     	42

typedef uint8_t  uchar;
typedef uint32_t ulong;
typedef uint16_t uint;
union longb { uchar b[4]; ulong l; };

class UdpStack {
private:
	uint  m_sessionPort;
	uint  m_serverPort;
	uchar m_udpData[DATA_SIZE]; // need only for UDP header, for data: using enc28's hardware buffer (by Renato Aloi - May 2015)
	uchar m_sendData[DATA_SIZE]; // need only for UDP header, also -- Total RAM consumption: 116 bytes
	uchar m_macAddr[MAC_SIZE]; // No need math, passing through
	uchar m_clientMacAddr[MAC_SIZE]; // Client's info
	uchar m_clientIpAddr[IP_SIZE];
	uchar m_ipAddr[IP_SIZE];
	bool  m_established;
	bool  m_closing;
	uint  m_rxPointer;
	uint  m_txPointer;


	void  handleStack(void);
	bool  isSession() { return m_sessionPort != 0; };
	void  returnArp(void);
	void  returnIcmp(void);
	
	void  waitForDMACopy(void);
	unsigned int checksumDMA(unsigned int len);

public:
	UdpStack() :  m_rxPointer(0), m_txPointer(0), 
				m_established(false), m_closing(false),
				m_sessionPort(0), m_serverPort(0)
	{
		for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = 0;
		for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = 0;
		for (unsigned i = 0; i < DATA_SIZE; i++) { m_udpData[i] = 0; m_sendData[i] = 0; }
	};
	void   setMacAddr(uchar* mac) { for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = mac[i]; };
	void   setIpAddr(uchar* ip) { for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = ip[i]; };
	void   open(uint serverPort);
	void   write(char c);  
	void   send(void) { /*returnHttp(); m_txPointer = 0; m_sizePayload = 0;*/ };
	char   read(void); // return char or -1 if reachs end
	void   close(void) { /*if (m_sizePayload) send(); returnClose(); m_closing = true;*/ };
	bool   established(void) { /*handleStack(); return m_established;*/ };
	bool   closing(void) { return m_closing; };
};

#endif //UDPSTACK_H
