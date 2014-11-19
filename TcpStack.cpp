/*
 TCP Stack for New EtherEnc based on Enc28Core
 by Renato Aloi NOV 2014
 * seriallink.com.br
 */

#include "TcpStack.h"


extern "C" {
#include "enc28j60.h"
#include "checksum.h"
}


void TcpStack::handleStack(void)
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
		if (DEBUG) { Serial.println("Got Packet!"); }

		// Getting Packet into buffer
		MACReadRXBuffer(m_tcpData, DATA_SIZE);
		/*
		if (DEBUG)
		{
			Serial.print("m_tcpData: ");
			for (unsigned int i = 0; i < DATA_SIZE + 1; i++)
			{
				Serial.print("0x");
				Serial.print(m_tcpData[i], HEX);
				Serial.print(" ");
			}
			Serial.println();
		}
		*/


		arp  = ((m_tcpData[ETH_TYPE_H_P] & 0xFF00) | m_tcpData[ETH_TYPE_L_P] == ((ETHTYPE_ARP_H_V & 0xFF00) | ETHTYPE_ARP_L_V));
		tcpip = ((m_tcpData[ETH_TYPE_H_P] & 0xFF00) | m_tcpData[ETH_TYPE_L_P] == ((ETHTYPE_IP_H_V & 0xFF00) | ETHTYPE_IP_L_V));

		if (arp)
		{
			if (DEBUG) Serial.println("Got ARP!");

			ipCheckOk = ((m_tcpData[ETH_ARP_DST_IP_P + 0] == m_ipAddr[0]) && (m_tcpData[ETH_ARP_DST_IP_P + 1] == m_ipAddr[1])
				&& (m_tcpData[ETH_ARP_DST_IP_P + 2] == m_ipAddr[2]) && (m_tcpData[ETH_ARP_DST_IP_P + 3] == m_ipAddr[3]));

			if (ipCheckOk)
			{
				if (DEBUG) Serial.println("Got ARP for my IP!");
				returnArp();
			}
		}
		else if (tcpip)
		{
			//

			ipV4LenOk = (m_tcpData[IP_HEADER_LEN_VER_P] == IP_HEADER_LEN_VER_V);
			ipCheckOk = ((m_tcpData[IP_DST_P + 0] == m_ipAddr[0]) && (m_tcpData[IP_DST_P + 1] == m_ipAddr[1])
					&& (m_tcpData[IP_DST_P + 2] == m_ipAddr[2]) && (m_tcpData[IP_DST_P + 3] == m_ipAddr[3]));

			if (ipV4LenOk && ipCheckOk)
			{
				//if (DEBUG) Serial.println("Got IPV4 OK And IpIsMine!");

				icmp = (m_tcpData[IP_PROTO_P] == IP_PROTO_ICMP_V && m_tcpData[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V);

				if (icmp)
				{
					if (DEBUG) Serial.println("Got ICMP!");
					returnIcmp();
				}
				else
				{
					isTcpPacket = (m_tcpData[IP_PROTO_P] == IP_PROTO_TCP_V);
					isTcpMyPort = (m_tcpData[TCP_DST_PORT_H_P] == 0 && m_tcpData[TCP_DST_PORT_L_P] == m_serverPort);

					if (isTcpPacket && isTcpMyPort)
					{
						//if (DEBUG) Serial.println("Got TCPIP!");

						synFlag = (m_tcpData[TCP_FLAGS_P] == TCP_FLAGS_SYN_V);
						ackFlag = (m_tcpData[TCP_FLAGS_P] == TCP_FLAGS_ACK_V);
						pshAckFlag = (m_tcpData[TCP_FLAGS_P] == TCP_FLAGS_PSHACK_V);
						finFlag = (m_tcpData[TCP_FLAGS_P] == TCP_FLAGS_FINACK_V);

						tmpPort = (((uint)m_tcpData[TCP_SRC_PORT_H_P]<<8) & 0xFF00)
								| ((m_tcpData[TCP_SRC_PORT_L_P]) & 0x00FF);

						if (synFlag)
						{
							if (DEBUG) Serial.println("Got SYN!");

							if (!isSession())
							{
								if (DEBUG) Serial.println("new session!");
								// new session
								m_sessionPort = (uint)(((uint)(m_tcpData[TCP_SRC_PORT_H_P]<<8) & 0xFF00) | (m_tcpData[TCP_SRC_PORT_L_P] & 0x00FF));
								m_seqNum.b[0] = m_tcpData[TCP_SEQ_P+3];
								m_seqNum.b[1] = m_tcpData[TCP_SEQ_P+2];
								m_seqNum.b[2] = m_tcpData[TCP_SEQ_P+1];
								m_seqNum.b[3] = m_tcpData[TCP_SEQ_P+0];
								m_ackNum.b[0] = m_tcpData[TCP_ACK_P+3];
								m_ackNum.b[1] = m_tcpData[TCP_ACK_P+2];
								m_ackNum.b[2] = m_tcpData[TCP_ACK_P+1];
								m_ackNum.b[3] = m_tcpData[TCP_ACK_P+0];

								if (DEBUG)
								{
									Serial.println();
									Serial.print("m_sessionPort: ");
									Serial.println(m_sessionPort, DEC);
									Serial.print("m_seqNum: ");
									Serial.println(m_seqNum.l, DEC);
									Serial.print("m_ackNum: ");
									Serial.println(m_ackNum.l, DEC);
									Serial.println();
								}

								returnSyn();
							}
							else
							{
								// session running already
								//sendFinToPorOutOfSession();
							}
						}

						if (ackFlag)
						{
							if (DEBUG) Serial.println(F("-------got ack!"));

							if (isSession() && m_sessionPort == tmpPort)
							{
								// session exists!
								if (!m_buffering && !m_responding)
								{
									if (DEBUG) Serial.println(F("syn revc!"));

									// we are established,
									// but buffering incomming data
									m_buffering = true;
									m_established = true;
								}
								else if (m_responding)
								{
									if (DEBUG) Serial.println(F("responding!"));
								}
							}

						}

						if (pshAckFlag)
						{
							if (isSession() && m_sessionPort == tmpPort)
							{
								// session exists and is buffering!
								if (m_buffering)
								{
									if (DEBUG) Serial.println("buffering!");

									// REV 3
									// DMA Zero-Copy to socket space

									// Getting http data payload
									m_recvPayload = ((((uint)m_tcpData[IP_TOTLEN_H_P]) << 8) & 0xFF00)
										| (((uint)m_tcpData[IP_TOTLEN_L_P]) & 0x00FF);

									// Getting source IP and MAC
									for (unsigned i = 0; i < MAC_SIZE; i++)
										m_clientMacAddr[i] = m_tcpData[ETH_SRC_MAC_P + i];
									for (unsigned i = 0; i < IP_SIZE; i++)
										m_clientIpAddr[i] = m_tcpData[IP_SRC_P + i];


									if (DEBUG) {
										Serial.print(F("m_recvPayload: "));
										Serial.println(m_recvPayload, DEC);
									}

									// Copy packet from
									// ENC28J60's RX buffer to socket RX buffer
									// TODO: Manage sockets: 0 - socket 1; 1 - socket 2
									DMACopyTo(RX, SOCKET_RX_START(0), m_recvPayload + ETH_BUFF_SIZE);
									waitForDMACopy();

									// Subtracting packet size from total = data size
									// It is -40, not -54; because source is IP datagram
									m_recvPayload -= (IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V);

									// Now we can discard packet freeing space from
									// RXTX FIFO, because we already moved the data
									// to Socket FIFO, far away from being overwriten
									if (DEBUG) Serial.println("buffering done!");

									// respond ack for push at once
									// to avoid server become anoying
									returnPush();

									// now we are ready
									m_buffering = false;
									m_responding = true;
								}
							}
						}

						if (finFlag)
						{
							if (isSession() && m_sessionPort == tmpPort)
							{
								// session exists and is closing!
								if (m_closing)
								{
									if (DEBUG) Serial.println("closing!");

									// Let client know that we know
									// it is the way it works
									returnFin();

									m_rxPointer = 0;
									//m_txPointer = 0; // For future use
									m_seqNum.l = 0;
									m_ackNum.l = 0;
									// cleaning up the mess!
									m_recvPayload = 0;
									m_sendPayload = 0;
									//m_packetId = 0; // no need, it will overflow sometime
									m_sessionPort = 0;
									m_buffering = false;
									m_established = false;
									m_responding = false;
									m_closing = false;
								}
							}
						}
					}
				}
			}
		}
		if (DEBUG) Serial.println("Descartando pacote tratado!");
		MACDiscardRx();
	}
}

void TcpStack::returnArp(void)
{
	// Filling send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_tcpData[ETH_SRC_MAC_P + i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
		m_sendData[ETH_ARP_DST_MAC_P + i] = m_tcpData[ETH_ARP_SRC_MAC_P + i];
        m_sendData[ETH_ARP_SRC_MAC_P + i] = m_macAddr[i];
	}
	m_sendData[ETH_ARP_OPCODE_H_P] = ETH_ARP_OPCODE_REPLY_H_V;
	m_sendData[ETH_ARP_OPCODE_L_P] = ETH_ARP_OPCODE_REPLY_L_V;
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[ETH_ARP_DST_IP_P + i] = m_tcpData[ETH_ARP_SRC_IP_P + i];
		m_sendData[ETH_ARP_SRC_IP_P + i] = m_ipAddr[i];
	}

	MACWriteTXBuffer(m_sendData, ETH_ARP_LEN);
	MACSendTx();
}

void TcpStack::returnIcmp(void)
{
	// Filling send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_tcpData[ETH_SRC_MAC_P + i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[IP_DST_P + i] = m_tcpData[IP_SRC_P + i];
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

void TcpStack::returnSyn(void)
{
	if (DEBUG) Serial.println(F("Returning SYN!"));

	// Filling send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		m_sendData[ETH_DST_MAC_P + i] = m_tcpData[ETH_SRC_MAC_P + i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	m_sendData[IP_TOTLEN_H_P] = 0;
	m_sendData[IP_TOTLEN_L_P] = IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + TCP_OPT_LEN_V;

	m_packetId++;
	m_sendData[IP_ID_H_P] = (m_packetId>>8)&0xFF;
	m_sendData[IP_ID_L_P] = (m_packetId)&0xFF;

	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		m_sendData[IP_DST_P + i] = m_tcpData[IP_SRC_P + i];
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

	m_sendData[TCP_FLAGS_P] = TCP_FLAGS_SYNACK_V;

	m_sendData[TCP_DST_PORT_H_P] = m_tcpData[TCP_SRC_PORT_H_P];
	m_sendData[TCP_DST_PORT_L_P] = m_tcpData[TCP_SRC_PORT_L_P];
	m_sendData[TCP_SRC_PORT_H_P] = 0;
	m_sendData[TCP_SRC_PORT_L_P] = 0;
	m_sendData[TCP_SRC_PORT_H_P] = ((m_serverPort>>8) & 0xFF);
	m_sendData[TCP_SRC_PORT_L_P] = ((m_serverPort) & 0xFF);



	// 0xC0C756EF
	//_bufferWr[TCP_SEQ_P]            = _bufferRd[TCP_ACK_P]
	if (m_ackNum.l == 0L) m_ackNum.l = 0xC0C756EF;
	m_sendData[TCP_SEQ_P+0] = m_ackNum.b[3];
	m_sendData[TCP_SEQ_P+1] = m_ackNum.b[2];
	m_sendData[TCP_SEQ_P+2] = m_ackNum.b[1];
	m_sendData[TCP_SEQ_P+3] = m_ackNum.b[0];
	m_seqNum.l++;
	m_sendData[TCP_ACK_P+0] = m_seqNum.b[3];
	m_sendData[TCP_ACK_P+1] = m_seqNum.b[2];
	m_sendData[TCP_ACK_P+2] = m_seqNum.b[1];
	m_sendData[TCP_ACK_P+3] = m_seqNum.b[0];

	if (DEBUG) Serial.print(F("m_ackNum: "));
	if (DEBUG) Serial.println(m_ackNum.l, DEC);
	if (DEBUG) Serial.print(F("m_seqNum: "));
	if (DEBUG) Serial.println(m_seqNum.l, DEC);

	m_sendData[TCP_CHECKSUM_H_P] = 0x00;
	m_sendData[TCP_CHECKSUM_L_P] = 0x00;

	m_sendData[TCP_OPTIONS_P+0] = 0x02;
	m_sendData[TCP_OPTIONS_P+1] = 0x04;
	m_sendData[TCP_OPTIONS_P+2] = 0x05;
	m_sendData[TCP_OPTIONS_P+3] = 0x80;

	m_sendData[TCP_HEADER_LEN_P] = 0x60;

	ck = checksum(&m_sendData[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN_V + 4, 2);
	m_sendData[TCP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[TCP_CHECKSUM_L_P] = ((ck)&0xFF);

	MACWriteTXBuffer(m_sendData, IP_HEADER_LEN_V
		                          + TCP_HEADER_LEN_PLAIN_V
		                          + 4
		                          + ETH_HEADER_LEN_V);
	MACSendTx();
	if (DEBUG) Serial.println();
}

void TcpStack::returnPush(void)
{
	if (DEBUG) Serial.println(F("Returning PUSH!"));

	// Filling send buffer
	// for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];
	// not this time!
	// Clearing send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;

	//if (DEBUG) Serial.print(F("Client's MAC: 0x"));

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientMacAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[ETH_DST_MAC_P + i] = m_clientMacAddr[i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	//if (DEBUG) Serial.println();

	if (DEBUG) {
		Serial.print(F("m_recvPayload: "));
		Serial.println(m_recvPayload, DEC);
	}

	// Packet Type
	m_sendData[ETH_TYPE_H_P] = 0x08;
	m_sendData[ETH_TYPE_L_P] = 0x00;
	// Header len
	m_sendData[IP_P] = 0x45;
	// Total len
	// TODO: It's conceptually wrong! Bad code!
	m_sendData[IP_TOTLEN_H_P] = 0;
	m_sendData[IP_TOTLEN_L_P] = IP_HEADER_LEN_V
							  + TCP_HEADER_LEN_PLAIN_V;

	m_packetId++;
	m_sendData[IP_ID_H_P] = (m_packetId>>8)&0xFF;
	m_sendData[IP_ID_L_P] = (m_packetId)&0xFF;

	if (DEBUG) {
		Serial.print(F("m_packetId: "));
		Serial.println(m_packetId, DEC);
	}

	//if (DEBUG) Serial.print(F("Client's IP: 0x"));
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientIpAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[IP_DST_P + i] = m_clientIpAddr[i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}
	//if (DEBUG) Serial.println();

	m_sendData[IP_CHECKSUM_P] = 0;
    m_sendData[IP_CHECKSUM_P+1] = 0;
    m_sendData[IP_FLAGS_P] = 0x40; // don't fragment
    m_sendData[IP_FLAGS_P + 1] = 0;  // fragement offset
    m_sendData[IP_TTL_P] = 64; // ttl
	m_sendData[IP_TTL_P+1] = 6; // TODO: damn bug!

    // calculate the checksum:
    uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    m_sendData[IP_CHECKSUM_P] = ck >> 8;
    m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	m_sendData[TCP_FLAGS_P] = TCP_FLAGS_ACK_V;

	m_sendData[TCP_DST_PORT_H_P] = (m_sessionPort>>8)&0xFF;
	m_sendData[TCP_DST_PORT_L_P] = (m_sessionPort)&0xFF;
	m_sendData[TCP_SRC_PORT_H_P] = ((m_serverPort>>8) & 0xFF);
	m_sendData[TCP_SRC_PORT_L_P] = ((m_serverPort) & 0xFF);

	m_ackNum.l++;
	m_sendData[TCP_SEQ_P+0] = m_ackNum.b[3];
	m_sendData[TCP_SEQ_P+1] = m_ackNum.b[2];
	m_sendData[TCP_SEQ_P+2] = m_ackNum.b[1];
	m_sendData[TCP_SEQ_P+3] = m_ackNum.b[0];
	m_seqNum.l += m_recvPayload;
	m_sendData[TCP_ACK_P+0] = m_seqNum.b[3];
	m_sendData[TCP_ACK_P+1] = m_seqNum.b[2];
	m_sendData[TCP_ACK_P+2] = m_seqNum.b[1];
	m_sendData[TCP_ACK_P+3] = m_seqNum.b[0];

	if (DEBUG) Serial.print(F("m_ackNum: "));
	if (DEBUG) Serial.println(m_ackNum.l, DEC);
	if (DEBUG) Serial.print(F("m_seqNum: "));
	if (DEBUG) Serial.println(m_seqNum.l, DEC);

	m_sendData[TCP_CHECKSUM_H_P] = 0x00;
	m_sendData[TCP_CHECKSUM_L_P] = 0x00;

	m_sendData[TCP_HEADER_LEN_P] = 0x50;

	m_sendData[TCP_WINDOW_H_P] = 0x72;
	m_sendData[TCP_WINDOW_L_P] = 0x10;

	ck = checksum(&m_sendData[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN_V, 2);
	m_sendData[TCP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[TCP_CHECKSUM_L_P] = ((ck)&0xFF);

	//if (DEBUG) Serial.print(F("m_tcpData: 0x"));
	//if (DEBUG) { for (unsigned i = 0; i < DATA_SIZE; i++)
	//	{ Serial.print(m_sendData[i], HEX); Serial.print(F(" 0x")); } }
	//if (DEBUG) Serial.println();

	// Filling TX Buffer
	MACWriteTXBuffer(m_sendData, ETH_HEADER_LEN_V
		           + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V);
	MACSendTx();
	if (DEBUG) Serial.println();
}

void TcpStack::returnHttp(uchar* _buf, uint _size)
{
	if (DEBUG) Serial.println(F("Printing to HTTP!"));

	// Filling send buffer
	// for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];
	// not this time!
	// Clearing send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;

	// Loading data
	for (unsigned i = 0; i < _size; i++)
	{
		m_sendData[ETH_HEADER_LEN_V
		           + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V
		           + i] = _buf[i];
	}

	//if (DEBUG) Serial.print(F("Client's MAC: 0x"));

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientMacAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[ETH_DST_MAC_P + i] = m_clientMacAddr[i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	//if (DEBUG) Serial.println();

	if (DEBUG) {
		Serial.print(F("m_sendPayload: "));
		Serial.println(m_sendPayload, DEC);
	}

	// Packet Type
	m_sendData[ETH_TYPE_H_P] = 0x08;
	m_sendData[ETH_TYPE_L_P] = 0x00;
	// Header len
	m_sendData[IP_P] = 0x45;
	// Total len
	// TODO: It's conceptually wrong! Bad code!
	m_sendData[IP_TOTLEN_H_P] = 0;
	m_sendData[IP_TOTLEN_L_P] = IP_HEADER_LEN_V
							  + TCP_HEADER_LEN_PLAIN_V + _size;

	m_packetId++;
	m_sendData[IP_ID_H_P] = (m_packetId>>8)&0xFF;
	m_sendData[IP_ID_L_P] = (m_packetId)&0xFF;

	if (DEBUG) {
		Serial.print(F("m_packetId: "));
		Serial.println(m_packetId, DEC);
	}

	//if (DEBUG) Serial.print(F("Client's IP: 0x"));
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientIpAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[IP_DST_P + i] = m_clientIpAddr[i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}
	//if (DEBUG) Serial.println();

	m_sendData[IP_CHECKSUM_P] = 0;
    m_sendData[IP_CHECKSUM_P+1] = 0;
    m_sendData[IP_FLAGS_P] = 0x40; // don't fragment
    m_sendData[IP_FLAGS_P + 1] = 0;  // fragement offset
    m_sendData[IP_TTL_P] = 64; // ttl
	m_sendData[IP_TTL_P+1] = 6; // TODO: damn bug!

    // calculate the checksum:
    uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    m_sendData[IP_CHECKSUM_P] = ck >> 8;
    m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	m_sendData[TCP_FLAGS_P] = TCP_FLAGS_ACK_V;

	m_sendData[TCP_DST_PORT_H_P] = (m_sessionPort>>8)&0xFF;
	m_sendData[TCP_DST_PORT_L_P] = (m_sessionPort)&0xFF;
	m_sendData[TCP_SRC_PORT_H_P] = ((m_serverPort>>8) & 0xFF);
	m_sendData[TCP_SRC_PORT_L_P] = ((m_serverPort) & 0xFF);

	m_ackNum.l+=m_sendPayload;
	m_sendData[TCP_SEQ_P+0] = m_ackNum.b[3];
	m_sendData[TCP_SEQ_P+1] = m_ackNum.b[2];
	m_sendData[TCP_SEQ_P+2] = m_ackNum.b[1];
	m_sendData[TCP_SEQ_P+3] = m_ackNum.b[0];
	m_sendData[TCP_ACK_P+0] = m_seqNum.b[3];
	m_sendData[TCP_ACK_P+1] = m_seqNum.b[2];
	m_sendData[TCP_ACK_P+2] = m_seqNum.b[1];
	m_sendData[TCP_ACK_P+3] = m_seqNum.b[0];

	if (DEBUG) Serial.print(F("m_ackNum: "));
	if (DEBUG) Serial.println(m_ackNum.l, DEC);
	if (DEBUG) Serial.print(F("m_seqNum: "));
	if (DEBUG) Serial.println(m_seqNum.l, DEC);

	m_sendData[TCP_CHECKSUM_H_P] = 0x00;
	m_sendData[TCP_CHECKSUM_L_P] = 0x00;

	m_sendData[TCP_HEADER_LEN_P] = 0x50;

	m_sendData[TCP_WINDOW_H_P] = 0x72;
	m_sendData[TCP_WINDOW_L_P] = 0x10;

	ck = checksum(&m_sendData[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN_V + _size, 2);
	m_sendData[TCP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[TCP_CHECKSUM_L_P] = ((ck)&0xFF);



	/*if (DEBUG) Serial.print(F("m_tcpData: 0x"));
	if (DEBUG) { for (unsigned i = 0; i < DATA_SIZE; i++)
		{ Serial.print(m_sendData[i], HEX); Serial.print(F(" 0x")); } }
	if (DEBUG) Serial.println();*/

	// Filling TX Buffer
	MACWriteTXBuffer(m_sendData, ETH_HEADER_LEN_V
		           + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + _size);

	// Updating last sent len
	// We must keep up with SEQ/ACK
	m_sendPayload = _size;

	if (DEBUG) Serial.println();
}

void TcpStack::returnClose(void)
{
	// Filling send buffer
	// for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];
	// not this time!
	// Clearing send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;

	if (DEBUG) Serial.print(F("Client's MAC: 0x"));

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		if (DEBUG) { Serial.print(m_clientMacAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[ETH_DST_MAC_P + i] = m_clientMacAddr[i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	if (DEBUG) Serial.println();

	if (DEBUG) {
		Serial.print(F("m_sendPayload: "));
		Serial.println(m_sendPayload, DEC);
	}

	// Packet Type
	m_sendData[ETH_TYPE_H_P] = 0x08;
	m_sendData[ETH_TYPE_L_P] = 0x00;
	// Header len
	m_sendData[IP_P] = 0x45;
	// Total len
	// TODO: It's conceptually wrong! Bad code!
	m_sendData[IP_TOTLEN_H_P] = 0;
	m_sendData[IP_TOTLEN_L_P] = IP_HEADER_LEN_V
							  + TCP_HEADER_LEN_PLAIN_V;

	m_packetId++;
	m_sendData[IP_ID_H_P] = (m_packetId>>8)&0xFF;
	m_sendData[IP_ID_L_P] = (m_packetId)&0xFF;

	if (DEBUG) Serial.print(F("Client's IP: 0x"));
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		if (DEBUG) { Serial.print(m_clientIpAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[IP_DST_P + i] = m_clientIpAddr[i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}
	if (DEBUG) Serial.println();

	m_sendData[IP_CHECKSUM_P] = 0;
    m_sendData[IP_CHECKSUM_P+1] = 0;
    m_sendData[IP_FLAGS_P] = 0x40; // don't fragment
    m_sendData[IP_FLAGS_P + 1] = 0;  // fragement offset
    m_sendData[IP_TTL_P] = 64; // ttl
	m_sendData[IP_TTL_P+1] = 6; // TODO: damn bug!

    // calculate the checksum:
    uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    m_sendData[IP_CHECKSUM_P] = ck >> 8;
    m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	m_sendData[TCP_FLAGS_P] = TCP_FLAGS_FINACK_V;

	m_sendData[TCP_DST_PORT_H_P] = (m_sessionPort>>8)&0xFF;
	m_sendData[TCP_DST_PORT_L_P] = (m_sessionPort)&0xFF;
	m_sendData[TCP_SRC_PORT_H_P] = ((m_serverPort>>8) & 0xFF);
	m_sendData[TCP_SRC_PORT_L_P] = ((m_serverPort) & 0xFF);

	m_ackNum.l+=m_sendPayload;
	m_sendData[TCP_SEQ_P+0] = m_ackNum.b[3];
	m_sendData[TCP_SEQ_P+1] = m_ackNum.b[2];
	m_sendData[TCP_SEQ_P+2] = m_ackNum.b[1];
	m_sendData[TCP_SEQ_P+3] = m_ackNum.b[0];
	m_sendData[TCP_ACK_P+0] = m_seqNum.b[3];
	m_sendData[TCP_ACK_P+1] = m_seqNum.b[2];
	m_sendData[TCP_ACK_P+2] = m_seqNum.b[1];
	m_sendData[TCP_ACK_P+3] = m_seqNum.b[0];

	if (DEBUG) Serial.print(F("m_ackNum: "));
	if (DEBUG) Serial.println(m_ackNum.l, DEC);
	if (DEBUG) Serial.print(F("m_seqNum: "));
	if (DEBUG) Serial.println(m_seqNum.l, DEC);

	m_sendData[TCP_CHECKSUM_H_P] = 0x00;
	m_sendData[TCP_CHECKSUM_L_P] = 0x00;

	m_sendData[TCP_HEADER_LEN_P] = 0x50;

	m_sendData[TCP_WINDOW_H_P] = 0x72;
	m_sendData[TCP_WINDOW_L_P] = 0x10;

	ck = checksum(&m_sendData[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN_V, 2);
	m_sendData[TCP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[TCP_CHECKSUM_L_P] = ((ck)&0xFF);

	// Filling TX Buffer
	MACWriteTXBuffer(m_sendData, ETH_HEADER_LEN_V
		           + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V);
	MACSendTx();
}

void TcpStack::returnFin(void)
{
	if (DEBUG) Serial.println(F("Returning FIN!"));

	// Filling send buffer
	// for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = m_tcpData[i];
	// not this time!
	// Clearing send buffer
	for (unsigned i = 0; i < DATA_SIZE; i++) m_sendData[i] = 0;

	//if (DEBUG) Serial.print(F("Client's MAC: 0x"));

	for (unsigned i = 0; i < MAC_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientMacAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[ETH_DST_MAC_P + i] = m_clientMacAddr[i];
		m_sendData[ETH_SRC_MAC_P + i] = m_macAddr[i];
	}
	//if (DEBUG) Serial.println();

	// Packet Type
	m_sendData[ETH_TYPE_H_P] = 0x08;
	m_sendData[ETH_TYPE_L_P] = 0x00;
	// Header len
	m_sendData[IP_P] = 0x45;
	// Total len
	// TODO: It's conceptually wrong! Bad code!
	m_sendData[IP_TOTLEN_H_P] = 0;
	m_sendData[IP_TOTLEN_L_P] = IP_HEADER_LEN_V
							  + TCP_HEADER_LEN_PLAIN_V;

	m_packetId++;
	m_sendData[IP_ID_H_P] = (m_packetId>>8)&0xFF;
	m_sendData[IP_ID_L_P] = (m_packetId)&0xFF;

	//if (DEBUG) Serial.print(F("Client's IP: 0x"));
	for(unsigned i = 0; i < IP_SIZE; i++)
	{
		//if (DEBUG) { Serial.print(m_clientIpAddr[i], HEX); Serial.print(F(" 0x"));}

		m_sendData[IP_DST_P + i] = m_clientIpAddr[i];
		m_sendData[IP_SRC_P + i] = m_ipAddr[i];
	}
	//if (DEBUG) Serial.println();

	m_sendData[IP_CHECKSUM_P] = 0;
    m_sendData[IP_CHECKSUM_P+1] = 0;
    m_sendData[IP_FLAGS_P] = 0x40; // don't fragment
    m_sendData[IP_FLAGS_P + 1] = 0;  // fragement offset
    m_sendData[IP_TTL_P] = 64; // ttl
	m_sendData[IP_TTL_P+1] = 6; // TODO: damn bug!

    // calculate the checksum:
    uint ck = checksum(&m_sendData[IP_P], IP_HEADER_LEN_V, 0);
    m_sendData[IP_CHECKSUM_P] = ck >> 8;
    m_sendData[IP_CHECKSUM_P+1] = ck & 0xff;

	m_sendData[TCP_FLAGS_P] = TCP_FLAGS_ACK_V;

	m_sendData[TCP_DST_PORT_H_P] = (m_sessionPort>>8)&0xFF;
	m_sendData[TCP_DST_PORT_L_P] = (m_sessionPort)&0xFF;
	m_sendData[TCP_SRC_PORT_H_P] = ((m_serverPort>>8) & 0xFF);
	m_sendData[TCP_SRC_PORT_L_P] = ((m_serverPort) & 0xFF);

	m_ackNum.l++;
	m_sendData[TCP_SEQ_P+0] = m_ackNum.b[3];
	m_sendData[TCP_SEQ_P+1] = m_ackNum.b[2];
	m_sendData[TCP_SEQ_P+2] = m_ackNum.b[1];
	m_sendData[TCP_SEQ_P+3] = m_ackNum.b[0];
	m_seqNum.l++;
	m_sendData[TCP_ACK_P+0] = m_seqNum.b[3];
	m_sendData[TCP_ACK_P+1] = m_seqNum.b[2];
	m_sendData[TCP_ACK_P+2] = m_seqNum.b[1];
	m_sendData[TCP_ACK_P+3] = m_seqNum.b[0];

	if (DEBUG) Serial.print(F("m_ackNum: "));
	if (DEBUG) Serial.println(m_ackNum.l, DEC);
	if (DEBUG) Serial.print(F("m_seqNum: "));
	if (DEBUG) Serial.println(m_seqNum.l, DEC);

	m_sendData[TCP_CHECKSUM_H_P] = 0x00;
	m_sendData[TCP_CHECKSUM_L_P] = 0x00;

	m_sendData[TCP_HEADER_LEN_P] = 0x50;

	m_sendData[TCP_WINDOW_H_P] = 0x72;
	m_sendData[TCP_WINDOW_L_P] = 0x10;

	ck = checksum(&m_sendData[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN_V, 2);
	m_sendData[TCP_CHECKSUM_H_P] = ((ck>>8)&0xFF);
	m_sendData[TCP_CHECKSUM_L_P] = ((ck)&0xFF);

	//if (DEBUG) Serial.print(F("m_tcpData: 0x"));
	//if (DEBUG) { for (unsigned i = 0; i < DATA_SIZE; i++)
	//	{ Serial.print(m_sendData[i], HEX); Serial.print(F(" 0x")); } }
	//if (DEBUG) Serial.println();

	// Filling TX Buffer
	MACWriteTXBuffer(m_sendData, ETH_HEADER_LEN_V
		           + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V);
	MACSendTx();
	if (DEBUG) Serial.println();
}


// Wait for DMA Copy to finish
//
void TcpStack::waitForDMACopy(void)
{
    ulong timerSendTrigger = millis();
	while(!IsDMACopyDone()) if (timerSendTrigger + 1000 < millis()) break;
}


// PUBLIC

void TcpStack::open(uint serverPort)
{
	m_serverPort = serverPort;

	// Socket and Eth layer init
	// old Pascal Stang / Guido Schocher Method
	//socketInit(_mac);
	// new Renato Aloi / Howard Schundler Method
	// REV 3: Merging Enc28CoreLib into EtherEncLib

	// Ethernet begin
	delay(300);

	// initialize I/O
	// ss as output:
	pinMode(ENC28J60_CONTROL_CS, OUTPUT);
	//CS Passive mode
	CSPASSIVE;
	pinMode(SPI_MOSI, OUTPUT);
	pinMode(SPI_SCK, OUTPUT);
	pinMode(SPI_MISO, INPUT);
	digitalWrite(SPI_MOSI, LOW);
	digitalWrite(SPI_SCK, LOW);

	// initialize SPI interface
	// master mode and Fosc/2 clock:
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);

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
		Serial.println(F("Software Rev.: 3"));
	}

}

char TcpStack::read(void)
{
	// Getting data from socket fifo, not RXTX fifo
	// Local buffer
	uchar localBuf[1] = { 0 };

	// Getting global rxPointer
	if (m_rxPointer == 0)
	{
		SOCKETSetRxPointer(SOCKET_RX_START(0) + 54);
		m_rxPointer = SOCKETGetRxPointer();
	}

	if (m_established && !m_buffering && !m_closing)
	{
		// Reading socket's buffer
		SOCKETReadBuffer(localBuf, 1, m_rxPointer); //localRxPointer);
		m_rxPointer++;
	}
	else return -1;

	//if (DEBUG)  { Serial.print((char)localBuf[0]); Serial.print(", "); }
	//if (DEBUG)  { Serial.print((char)localBuf[0]); }

	if (m_recvPayload-- == 0)
		return -1;

	return (char)localBuf[0];
}

void TcpStack::send(void)
{
	// You may asking why ???
	// For future, TX will be gathered from socket fifo
	// rather than directly from TX fifo
	// So.. Must be an aparted send function
	MACSendTx();
	//delay(10);
}

// Remember! write() and close() already done at .h file!

