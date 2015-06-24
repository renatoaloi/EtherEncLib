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
#define DEBUGUDP		0
#define ETH_ARP_LEN     	42

typedef uint8_t  uchar;
typedef uint32_t ulong;
typedef uint16_t uint;
union longb { uchar b[4]; ulong l; };

class UdpStack {
private:
	uchar m_udpData[DATA_SIZE]; // need only for UDP header, for data: using enc28's hardware buffer (by Renato Aloi - May 2015)
	uchar m_sendData[DATA_SIZE]; // need only for UDP header, also -- Total RAM consumption: 116 bytes

	uchar m_macAddr[MAC_SIZE]; // No need math, passing through
	uchar m_ipAddr[IP_SIZE];
	uint  m_serverPort;

	uchar m_remoteMacAddr[MAC_SIZE];
	uchar m_remoteIpAddr[IP_SIZE];
	uint  m_sessionPort;
	
	uint  m_rxPointer;
	uint  m_txPointer;
	uint  m_dataSize;
	uint  m_sizePayload;
	uint  m_packetId;

	void  handleStack(void);
	void  returnArp(void);
	void  returnIcmp(void);
	void  returnUdp(void);
	unsigned int checksumDMA(unsigned int payload);
	void  waitForDMACopy(void);

public:
	UdpStack() :  m_rxPointer(0), m_txPointer(0), m_sizePayload(0), m_packetId(1000),
				m_sessionPort(0), m_serverPort(0), m_dataSize(0)
	{
		for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = 0;
		for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = 0;
		for (unsigned i = 0; i < MAC_SIZE; i++)  m_remoteMacAddr[i] = 255; // initial value as broadcast to dhcp
		for (unsigned i = 0; i < IP_SIZE; i++)   m_remoteIpAddr[i] = 255;
		for (unsigned i = 0; i < DATA_SIZE; i++) { m_udpData[i] = 0; m_sendData[i] = 0; }
	};

	void   setMacAddr(uchar* mac) { for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = mac[i]; };
	void   setIpAddr(uchar* ip) { for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = ip[i]; };
	uchar* getRemoteIp(void) { return m_remoteIpAddr; };
	bool   isSession(void) { return m_sessionPort != 0; };
	
	void   open(uint serverPort);
	uint   established(void) { handleStack(); return m_dataSize; };

	void   beginSend(uchar* ip, uint port);

	void   write(char c);  
	char   read(void); // return char or -1 if reachs end

	void   send(void) { returnUdp(); m_sessionPort = 0; m_sizePayload = 0; };

};

#endif //UDPSTACK_H
