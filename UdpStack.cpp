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

#include "UdpStack.h"
//--- made by SKA ---
#include "enc28j60.h"

extern "C" {
//--- made by SKA ---#include "enc28j60.h"
#include "checksum.h"
}


void UdpStack::handleStack(void)
{
	bool arp  = false;
	bool icmp = false;
	bool tcpip = false;
	bool ipCheckOk = false;
	bool ipV4LenOk = false;
	bool isTcpPacket = false;
	bool isTcpMyPort = false;
	bool synFlag = false;
	bool ackFlag = false;
	bool pshAckFlag = false;
	bool finFlag = false;
	uint tmpPort = 0;



	// Check for incoming packets
	if (MACGetPacketCount() > 0)
	{
		if (DEBUG) { Serial.println(F("Got Packet!")); }

		// Getting Packet from enc28 hardware's buffer
		MACReadRXBuffer(m_udpData, DATA_SIZE);

		arp  = ((m_udpData[ETH_TYPE_H_P] & 0xFF00) | m_udpData[ETH_TYPE_L_P] == ((ETHTYPE_ARP_H_V & 0xFF00) | ETHTYPE_ARP_L_V));
		tcpip = ((m_udpData[ETH_TYPE_H_P] & 0xFF00) | m_udpData[ETH_TYPE_L_P] == ((ETHTYPE_IP_H_V & 0xFF00) | ETHTYPE_IP_L_V));

		if (arp)
		{
			if (DEBUG) Serial.println(F("Got ARP!"));

			ipCheckOk = ((m_udpData[ETH_ARP_DST_IP_P + 0] == m_ipAddr[0]) && (m_udpData[ETH_ARP_DST_IP_P + 1] == m_ipAddr[1])
				&& (m_udpData[ETH_ARP_DST_IP_P + 2] == m_ipAddr[2]) && (m_udpData[ETH_ARP_DST_IP_P + 3] == m_ipAddr[3]));

			if (ipCheckOk)
			{
				if (DEBUG) Serial.println(F("Got ARP for my IP!"));
				returnArp();
			}
		}
		else if (tcpip)
		{
			
		}
		if (DEBUG) Serial.println(F("Descartando pacote tratado!"));
		MACDiscardRx();
	}
}

void UdpStack::returnArp(void)
{
	// Clearing buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;
	// Filling send buffer
	for (unsigned i = 0; i < ETH_ARP_LEN; i++) m_sendData[i] = m_udpData[i];

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_udpData[ETH_SRC_MAC_P + i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
		m_sendData[ETH_ARP_DST_MAC_P + i] = m_udpData[ETH_ARP_SRC_MAC_P + i];
        	m_sendData[ETH_ARP_SRC_MAC_P + i] = m_macAddr[i];
	}
	m_sendData[ETH_ARP_OPCODE_H_P] = ETH_ARP_OPCODE_REPLY_H_V;
	m_sendData[ETH_ARP_OPCODE_L_P] = ETH_ARP_OPCODE_REPLY_L_V;
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[ETH_ARP_DST_IP_P + i] = m_udpData[ETH_ARP_SRC_IP_P + i];
		m_sendData[ETH_ARP_SRC_IP_P + i] = m_ipAddr[i];
	}

	MACWriteTXBuffer(m_sendData, ETH_ARP_LEN);
	MACSendTx();
}

void UdpStack::returnIcmp(void)
{
	// Filling send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_udpData[i];

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_udpData[ETH_SRC_MAC_P + i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[IP_DST_P + i] = m_udpData[IP_SRC_P + i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}
	m_sendData[IP_CHECKSUM_P] = 0;
    m_sendData[IP_CHECKSUM_P+1] = 0;
    m_sendData[IP_FLAGS_P] = 0x40; // don't fragment
    m_sendData[IP_FLAGS_P + 1] = 0;  // fragement offset
    m_sendData[IP_TTL_P] = 64; // ttl

    // calculate the checksum:
    uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    m_sendData[IP_CHECKSUM_P] = ck >> 8;
    m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	m_sendData[ICMP_TYPE_P] = ICMP_TYPE_ECHOREPLY_V;
	if (m_sendData[ICMP_CHECKSUM_P] > (0xFF - ICMP_TYPE_ECHOREQUEST_V))
	{
		m_sendData[ICMP_CHECKSUM_P + 1]++;
	}
	m_sendData[ICMP_CHECKSUM_P] += 0x08;

	MACWriteTXBuffer(m_sendData, DATA_SIZE);
	MACSendTx();
}




// Wait for DMA Copy to finish
//
void UdpStack::waitForDMACopy(void)
{
    ulong timerSendTrigger = millis();
	while(!IsDMACopyDone()) if (timerSendTrigger + 1000 < millis()) break;
}


// PUBLIC

void UdpStack::open(uint serverPort)
{
	m_serverPort = serverPort;

	// Ethernet begin
	delay(300);

	SPI.begin();
//--- made by SKA ---

	if (DEBUGLT) { Serial.println(F("Configuring Ethernet Layer...")); }
	if (DEBUGLT) { Serial.print(F("IPAddr: ")); Serial.print(m_ipAddr[0], DEC); Serial.print(F(".")); Serial.print(m_ipAddr[1], DEC); }
	if (DEBUGLT) { Serial.print(F(".")); Serial.print(m_ipAddr[2], DEC); Serial.print(F(".")); Serial.print(m_ipAddr[3], DEC); Serial.println(); }
	if (DEBUGLT) { Serial.print(F("MACAddr: 0x")); Serial.print(m_macAddr[0], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[1], HEX); }
	if (DEBUGLT) { Serial.print(F(" 0x")); Serial.print(m_macAddr[2], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[3], HEX); }
	if (DEBUGLT) { Serial.print(F(" 0x")); Serial.print(m_macAddr[4], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[5], HEX); Serial.println(); }

	MACInit();
	if (DEBUGLT) { Serial.println(F("MACInit OK!")); }
	MACInitMacAddr(m_macAddr);
	if (DEBUGLT) { Serial.println(F("MACInitMacAddr OK!")); }
	MACOpen();
	if (DEBUGLT) { Serial.println(F("MACOpen OK!")); }
	MACEnableRecv();
	if (DEBUGLT) { Serial.println(F("MACEnableRecv OK!")); }

	// REV 3
	if (DEBUGLT)
	{
		Serial.print(F("Hardware Rev.: "));
		Serial.println(MACHardwareRevision());
		Serial.println(F("Software Rev.: 3.1"));
	}

}

char UdpStack::read(void)
{
	// Local buffer
	uchar localBuf[1] = { 0 };

	// Getting global rxPointer
	if (m_rxPointer == 0)
	{
		// Getting data from socket fifo, not RXTX fifo
		SOCKETSetRxPointer(SOCKET_RX_START(0) + 54);
		m_rxPointer = SOCKETGetRxPointer();
	}

	if (m_established)
	{
		// Reading socket's buffer
		SOCKETReadBuffer(localBuf, 1, m_rxPointer); 
		m_rxPointer++;
	}
	else return -1;

	//if (DEBUG)  { Serial.print((char)localBuf[0]); Serial.print(F(", ")); }
	if (DEBUG)  { Serial.print((char)localBuf[0]); }

	//if (m_recvPayload-- == 0)
	//	return -1;

	return (char)localBuf[0];
}

void UdpStack::write(char c)
{
	// Local buffer
	uchar localBuf[] = { 0, 0 };
	localBuf[0] = c;

	/*if (m_sizePayload < DATA_SIZE_HARDWARE)
  	{
		// Only send if sizePayload reaches max
		m_sizePayload++;
	}
	else
	{
		// reached max packet size, sending
		send();
	}*/

	// Getting global txPointer
	if (m_txPointer == 0)
	{
		// Writing data to socket fifo, not RXTX fifo
		SOCKETSetTxPointer(SOCKET_TX_START(0));
		m_txPointer = SOCKETGetTxPointer();
	}

	if (m_established )
	{
		// Writing at socket's buffer
		SOCKETWriteBuffer(localBuf, 2, m_txPointer); 
		m_txPointer++;
	}

	//if (DEBUG)  { Serial.print(F("size: ")); Serial.println(m_sizePayload); }
	//if (DEBUG)  { Serial.print((char)localBuf[0]); }

	
}


// Remember! send() and close() already done at .h file!

