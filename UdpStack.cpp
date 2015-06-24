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
#include "enc28j60.h"

extern "C" {
#include "checksum.h"
}


void UdpStack::handleStack(void)
{
	bool arp  = false;
	bool icmp = false;
	bool tcpip = false;
	bool ipCheckOk = false;
	bool ipV4LenOk = false;
	bool isUdpPacket = false;
	bool isUdpMyPort = false;
	
	// destination port
	uint udpPort = 0;

	// reseting datasize
	m_dataSize = 0;

	// Check for incoming packets
	if (MACGetPacketCount() > 0)
	{
		if (DEBUGUDP) { Serial.println(F("Got Packet!")); }

		// Getting Packet from enc28 hardware's buffer
		MACReadRXBuffer(m_udpData, DATA_SIZE);

		
		// checking if packet is arp or ip datagram
		arp  = ((m_udpData[ETH_TYPE_H_P] & 0xFF00) | m_udpData[ETH_TYPE_L_P] == ((ETHTYPE_ARP_H_V & 0xFF00) | ETHTYPE_ARP_L_V));
		tcpip = ((m_udpData[ETH_TYPE_H_P] & 0xFF00) | m_udpData[ETH_TYPE_L_P] == ((ETHTYPE_IP_H_V & 0xFF00) | ETHTYPE_IP_L_V));

		if (arp)
		{
			if (DEBUGUDP) Serial.println(F("Got ARP!"));

			ipCheckOk = ((m_udpData[ETH_ARP_DST_IP_P + 0] == m_ipAddr[0]) && (m_udpData[ETH_ARP_DST_IP_P + 1] == m_ipAddr[1])
				&& (m_udpData[ETH_ARP_DST_IP_P + 2] == m_ipAddr[2]) && (m_udpData[ETH_ARP_DST_IP_P + 3] == m_ipAddr[3]));

			if (ipCheckOk)
			{
				if (DEBUGUDP) Serial.println(F("Got ARP for my IP!"));
				returnArp();
			}
		}
		else if (tcpip)
		{
			if (DEBUGUDP) Serial.println(F("Got IP Datagram!"));

			ipV4LenOk = (m_udpData[IP_HEADER_LEN_VER_P] == IP_HEADER_LEN_VER_V);
			ipCheckOk = ((m_udpData[IP_DST_P + 0] == m_ipAddr[0]) && (m_udpData[IP_DST_P + 1] == m_ipAddr[1])
					&& (m_udpData[IP_DST_P + 2] == m_ipAddr[2]) && (m_udpData[IP_DST_P + 3] == m_ipAddr[3]));

			if (ipV4LenOk && ipCheckOk)
			{
				icmp = (m_udpData[IP_PROTO_P] == IP_PROTO_ICMP_V && m_udpData[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V);

				if (icmp)
				{
					if (DEBUGUDP) Serial.println(F("Got ICMP!"));
					returnIcmp();
				}
				else
				{
					isUdpPacket = (m_udpData[IP_PROTO_P] == IP_PROTO_UDP_V);
					udpPort = ((uint)((uint)m_udpData[UDP_DST_PORT_H_P] << 8) & 0xFF00) | ((uint)((uint)m_udpData[UDP_DST_PORT_L_P]) & 0x00FF);
					isUdpMyPort = (udpPort == m_serverPort);

					if (DEBUGUDP) Serial.print(F("UDP Port: "));
					if (DEBUGUDP) Serial.println(udpPort, DEC);


					if (isUdpPacket && isUdpMyPort)
					{
						if (DEBUGUDP) Serial.println(F("Got UDP!"));

						// REV 3
						// DMA Zero-Copy to socket space
						// Getting data payload
						unsigned int m_recvPayload = ((((uint)m_udpData[IP_TOTLEN_H_P]) << 8) & 0xFF00)
							| (((uint)m_udpData[IP_TOTLEN_L_P]) & 0x00FF);

						if (DEBUGUDP) { Serial.print(F("m_recvPayload: ")); Serial.println(m_recvPayload, DEC); }

						// Copy packet from
						// ENC28J60's RX buffer to socket RX buffer
						// TODO: Manage sockets: 0 - socket 1; 1 - socket 2
						DMACopy(RX, SOCKET_RX_START(0), m_recvPayload + ETH_BUFF_SIZE);
						waitForDMACopy();

						// Subtracting packet size from total = data size
						// TODO: Decide wich payload variable between m_recvPayload and m_dataSize
						m_recvPayload -= (IP_HEADER_LEN_V + UDP_HEADER_LEN);

						if (DEBUGUDP) { Serial.print(F("m_recvPayload2: ")); Serial.println(m_recvPayload, DEC); }

						// Now we can discard packet freeing space from
						// RXTX FIFO, because we already moved the data
						// to Socket FIFO, far away from being overwriten
						if (DEBUGUDP) Serial.println(F("buffering done!"));

						if (DEBUGUDP)
						{
							Serial.print(F("m_udpData: "));
							for (unsigned int i = 0; i < DATA_SIZE + 1; i++)
							{
								Serial.print(F("0x"));
								Serial.print(m_udpData[i], HEX);
								Serial.print(F(" "));
							}
							Serial.println();
						}

						// updating remote ip
						for(unsigned i = 0; i < IP_SIZE; i++)
							m_remoteIpAddr[i] = m_udpData[IP_SRC_P + i];

						// updating remote mac address
						for(unsigned i = 0; i < MAC_SIZE; i++)
							m_remoteMacAddr[i] = m_udpData[ETH_SRC_MAC_P + i];


						if (DEBUGUDP) { Serial.print(F("Remote IPAddr: ")); Serial.print(m_remoteIpAddr[0], DEC); Serial.print(F(".")); Serial.print(m_remoteIpAddr[1], DEC); }
						if (DEBUGUDP) { Serial.print(F(".")); Serial.print(m_remoteIpAddr[2], DEC); Serial.print(F(".")); Serial.print(m_remoteIpAddr[3], DEC); Serial.println(); }
	
						
						// getting data len
						m_dataSize = ((uint)((uint)m_udpData[UDP_LEN_H_P] << 8) & 0xFF00) | ((uint)((uint)m_udpData[UDP_LEN_L_P]) & 0x00FF);
						// removing header from size
						m_dataSize -= UDP_HEADER_LEN;

						// Configuring reader pointer
						//					14               + 20              + 8             = 42
						SOCKETSetRxPointer(SOCKET_RX_START(0) + ETH_HEADER_LEN_V + IP_HEADER_LEN_V + UDP_HEADER_LEN);
						m_rxPointer = SOCKETGetRxPointer();
						// Reseting writer pointer
						m_txPointer = 0;
						//

						if (DEBUGUDP) Serial.print(F("m_rxPointer: "));
						if (DEBUGUDP) Serial.println(m_rxPointer, DEC);
						if (DEBUGUDP) Serial.print(F("Data_Len: "));
						if (DEBUGUDP) Serial.println(m_dataSize, DEC);
					}
				}
			}
		}
		if (DEBUGUDP) Serial.println(F("Descartando pacote tratado!"));
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

void UdpStack::returnUdp(void)
{
	if (DEBUGUDP) Serial.println();
	if (DEBUGUDP) Serial.println(F("Printing to UDP!"));

	// Clearing send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_remoteMacAddr[i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}

	if (DEBUGUDP) {
		Serial.print(F("m_sizePayload: "));
		Serial.println(m_sizePayload, DEC);
	}

	// Packet Type
	m_sendData[ETH_TYPE_H_P] = 0x08;
	m_sendData[ETH_TYPE_L_P] = 0x00;
	// Header len
	m_sendData[IP_P] = 0x45;
	// Total len
	m_sendData[IP_TOTLEN_H_P] = high(IP_HEADER_LEN_V + UDP_HEADER_LEN + m_sizePayload);
	m_sendData[IP_TOTLEN_L_P] = low(IP_HEADER_LEN_V + UDP_HEADER_LEN + m_sizePayload);

	m_packetId++;
	m_sendData[IP_ID_H_P] = high(m_packetId);
	m_sendData[IP_ID_L_P] = low(m_packetId);

	if (DEBUGUDP) {
		Serial.print(F("m_packetId: "));
		Serial.println(m_packetId, DEC);
	}

	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[IP_DST_P + i] = m_remoteIpAddr[i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}

	m_sendData[IP_CHECKSUM_P]   = 0x00;
	m_sendData[IP_CHECKSUM_P+1] = 0x00;
	m_sendData[IP_FLAGS_P]      = 0x00;
    	m_sendData[IP_FLAGS_P + 1]  = 0x00;
    	m_sendData[IP_TTL_P]        = 0x80; 
	m_sendData[IP_PROTO_P]      = IP_PROTO_UDP_V;

    	// calculate the checksum:
    	uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    	m_sendData[IP_CHECKSUM_P]   = ck >> 8;
    	m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	// UDP
	m_sendData[UDP_DST_PORT_H_P] = (m_sessionPort>>8);
	m_sendData[UDP_DST_PORT_L_P] = m_sessionPort;
	m_sendData[UDP_SRC_PORT_H_P] = (m_serverPort>>8);
	m_sendData[UDP_SRC_PORT_L_P] = m_serverPort;
	m_sendData[UDP_LEN_H_P] = high(UDP_HEADER_LEN + m_sizePayload);
	m_sendData[UDP_LEN_L_P] = low(UDP_HEADER_LEN + m_sizePayload);
	m_sendData[UDP_CHECKSUM_H_P] = 0;
	m_sendData[UDP_CHECKSUM_L_P] = 0;

	//fill_checksum(UDP_CHECKSUM_H_P, IP_SRC_P, 16 + datalen,1);
	//packetSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen);

	if (DEBUGUDP)  { Serial.print(F("size: ")); Serial.println(m_sizePayload); }

	// Filling data before checksum!
	DMACopy(TX, TXSTART_INIT + ETH_HEADER_LEN_V + IP_HEADER_LEN_V + UDP_HEADER_LEN + 1, m_sizePayload);
	waitForDMACopy();
	//

	ck = checksumDMA(UDP_HEADER_LEN + m_sizePayload);
	m_sendData[UDP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[UDP_CHECKSUM_L_P] = ((ck)&0xFF);

	// strange bug
	// must re-issue header len
	// I think checksum is overriding next byte
	//m_sendData[TCP_HEADER_LEN_P] = 0x50;

	

	// Filling TX Buffer
	MACWriteTXBuffer(m_sendData, ETH_HEADER_LEN_V + IP_HEADER_LEN_V + UDP_HEADER_LEN); // + m_sizePayload);

	MACWriteTXEndPt(ETH_HEADER_LEN_V + IP_HEADER_LEN_V + UDP_HEADER_LEN + m_sizePayload);


	MACSendTx();
	if (DEBUGUDP) Serial.println();
}


unsigned int UdpStack::checksumDMA(unsigned int payload)
{
	// TODO: Test hardware checksum for future implementation!
	//m_sendData[IP_SRC_P]
	unsigned int len = payload;
	unsigned long sum = 0;
	unsigned char buf[] = {0, 0};
	unsigned char *p = &m_sendData[IP_SRC_P];

	sum += IP_PROTO_UDP_V;
	sum += len;

	SOCKETSetTxPointer(SOCKET_TX_START(0));
	m_rxPointer = SOCKETGetTxPointer();

	len+=8;
	payload+=8;
        while(len > 1)
	{
		if ((payload - len) < (ETH_HEADER_LEN_V + IP_HEADER_LEN_V + UDP_HEADER_LEN - (IP_SRC_P + 1))) 
		{ 
			sum += 0xFFFF & (((*p) << 8) | *(p + 1)); 
			
			if (DEBUGUDP) { Serial.print(*p); Serial.print(F(";")); Serial.print(*(p+1)); Serial.print(F(";")); }
			p += 2; 
		}
		else 
		{ 
			SOCKETReadBuffer(buf, 2, m_rxPointer);
			m_rxPointer += 2;
	
			sum += 0xFFFF & (buf[0] << 8 | buf[1]); 

			if (DEBUGUDP) { Serial.print((char)buf[0]); Serial.print(F(",")); Serial.print((char)buf[1]); Serial.print(F(",")); }
			
		}
                len -= 2;
        }

        if (len) {
		
			SOCKETReadBuffer(buf, 2, m_rxPointer);
			sum += (0xFF & buf[0]) << 8;
			if (DEBUGUDP) { Serial.println((char)buf[0]);  }
		
	}



        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
        return( (unsigned int) sum ^ 0xFFFF);
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

	if (DEBUGUDP) { Serial.println(F("Configuring Ethernet Layer...")); }
	if (DEBUGUDP) { Serial.print(F("Server Port: ")); Serial.println(m_serverPort, DEC); }
	if (DEBUGUDP) { Serial.print(F("IPAddr: ")); Serial.print(m_ipAddr[0], DEC); Serial.print(F(".")); Serial.print(m_ipAddr[1], DEC); }
	if (DEBUGUDP) { Serial.print(F(".")); Serial.print(m_ipAddr[2], DEC); Serial.print(F(".")); Serial.print(m_ipAddr[3], DEC); Serial.println(); }
	if (DEBUGUDP) { Serial.print(F("MACAddr: 0x")); Serial.print(m_macAddr[0], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[1], HEX); }
	if (DEBUGUDP) { Serial.print(F(" 0x")); Serial.print(m_macAddr[2], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[3], HEX); }
	if (DEBUGUDP) { Serial.print(F(" 0x")); Serial.print(m_macAddr[4], HEX); Serial.print(F(" 0x")); Serial.print(m_macAddr[5], HEX); Serial.println(); }

	MACInit();
	if (DEBUGUDP) { Serial.println(F("MACInit OK!")); }
	MACInitMacAddr(m_macAddr);
	if (DEBUGUDP) { Serial.println(F("MACInitMacAddr OK!")); }
	MACOpen();
	if (DEBUGUDP) { Serial.println(F("MACOpen OK!")); }
	MACEnableRecv();
	if (DEBUGUDP) { Serial.println(F("MACEnableRecv OK!")); }

	// REV 3
	if (DEBUGUDP)
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

	// Checking if rxPointer is not zero 
	// and if datSize is greather than zero
	if (m_rxPointer != 0 && m_dataSize > 0)
	{
		// Reading socket's buffer
		SOCKETReadBuffer(localBuf, 1, m_rxPointer); 

		// Updating rxPointer
		m_rxPointer++;

		// Updating dataSize
		// and checking if is greather than zero
		if (m_dataSize-- == 0)
			return -1;

		if (DEBUGUDP)  { Serial.print((char)localBuf[0]); }
		return (char)localBuf[0];
	}

	// if we got here, rxPointer is zero
	return -1;
}

void UdpStack::write(char c)
{
	// Local buffer
	uchar localBuf[] = { 0, 0 };
	localBuf[0] = c;

	//if (DEBUGUDP)  { Serial.print(F("m_txPointer: ")); Serial.println(m_txPointer); }

	// Updating send payload size
	if (m_sizePayload < DATA_SIZE_HARDWARE)
  	{
		// Only send if sizePayload reaches max
		m_sizePayload++;
	}
	else
	{
		if (DEBUGUDP)  { Serial.println(F("sending.....")); }
		// reached max packet size, sending
		send();
		// clearing tx pointer 
		m_txPointer = 0;
	}

	// Getting global txPointer
	if (m_txPointer == 0)
	{
		// Writing data to socket fifo, not RXTX fifo
		SOCKETSetTxPointer(SOCKET_TX_START(0));
		m_txPointer = SOCKETGetTxPointer();
	}

	// Writing 2 bytes instead of 1 to avoid ERRATA bug rev 7b
	// Writing at socket's buffer
	SOCKETWriteBuffer(localBuf, 2, m_txPointer); 
	m_txPointer++;

	//if (DEBUGUDP)  { Serial.print(F("size: ")); Serial.println(m_sizePayload); }
	if (DEBUGUDP)  { Serial.print((char)localBuf[0]); }
	
}

void UdpStack::beginSend(uchar* ip, uint port) 
{ 	
 	for (unsigned i = 0; i < IP_SIZE; i++)   m_remoteIpAddr[i] = ip[i]; 
	m_sessionPort = port;
	m_txPointer = 0;
}

// Remember! send() and close() already done at .h file!

