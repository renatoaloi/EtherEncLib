/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Altered by Renato Aloi - 2013 August
 * Copyright: GPL V2
 *
 * Based on the net.h file from the AVRlib library by Pascal Stang.
 * For AVRlib See http://www.procyonengineering.com/
 * Used with explicit permission of Pascal Stang.
 *
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/

// notation: _P = position of a field
//           _V = value of a field

//@{

#ifndef NET_H
#define NET_H


// ********************************************************************************************************************************************
// ******* ETH VALUES *******
#define ETHTYPE_ARP_H_V          0x08
#define ETHTYPE_ARP_L_V          0x06
#define ETHTYPE_IP_H_V           0x08
#define ETHTYPE_IP_L_V           0x00
#define ETH_HEADER_LEN_V         0x0E

// ******* ETH POSITIONS *******
// Ethernet type field (2bytes):
#define ETH_TYPE_H_P             0x0C
#define ETH_TYPE_L_P             0x0D
#define ETH_DST_MAC_P            0x00
#define ETH_SRC_MAC_P            0x06

// ******* ETH FUNCS *******
#define isArpPacket()            ((_bufferRd[ETH_TYPE_H_P] & 0xFF00) | _bufferRd[ETH_TYPE_L_P] == ((ETHTYPE_ARP_H_V & 0xFF00) | ETHTYPE_ARP_L_V))
#define isIpPacket()             ((_bufferRd[ETH_TYPE_H_P] & 0xFF00) | _bufferRd[ETH_TYPE_L_P] == ((ETHTYPE_IP_H_V & 0xFF00) | ETHTYPE_IP_L_V))
#define fillDstMac(i)            _bufferWr[ETH_DST_MAC_P + i] = _bufferRd[ETH_SRC_MAC_P + i]
#define fillSrcMac(i)            _bufferWr[ETH_SRC_MAC_P + i] = _mac[i]
// ********************************************************************************************************************************************


// ********************************************************************************************************************************************
// ******* ARP VALUES *******
#define ETH_ARP_OPCODE_REPLY_H_V 0x00
#define ETH_ARP_OPCODE_REPLY_L_V 0x02

// ******* ARP POSITIONS *******
#define ETH_ARP_DST_IP_P         0x26
#define ETH_ARP_OPCODE_H_P       0x14
#define ETH_ARP_OPCODE_L_P       0x15
#define ETH_ARP_SRC_MAC_P        0x16
#define ETH_ARP_SRC_IP_P         0x1C
#define ETH_ARP_DST_MAC_P        0x20
#define ETH_ARP_DST_IP_P         0x26

// ******* ARP FUNCS *******
#define isMyIpByte(i)            (_bufferRd[ETH_ARP_DST_IP_P + i] == _ip[i])
#define fillArpOptCodeH()        _bufferWr[ETH_ARP_OPCODE_H_P]    = ETH_ARP_OPCODE_REPLY_H_V
#define fillArpOptCodeL()        _bufferWr[ETH_ARP_OPCODE_L_P]    = ETH_ARP_OPCODE_REPLY_L_V
#define fillArpDstMac(i)         _bufferWr[ETH_ARP_DST_MAC_P + i] = _bufferRd[ETH_ARP_SRC_MAC_P + i]
#define fillArpSrcMac(i)         _bufferWr[ETH_ARP_SRC_MAC_P + i] = _mac[i]
#define fillArpDstIp(i)          _bufferWr[ETH_ARP_DST_IP_P + i]  = _bufferRd[ETH_ARP_SRC_IP_P + i]
#define fillArpSrcIp(i)          _bufferWr[ETH_ARP_SRC_IP_P + i]  = _ip[i]
// ********************************************************************************************************************************************


// ********************************************************************************************************************************************
// ******* IP VALUES *******
#define IP_PROTO_ICMP_V		 0x01
#define IP_PROTO_TCP_V		 0x06
#define IP_PROTO_UDP_V		 0x11
#define IP_V4_V			 0x40
#define IP_HEADER_LEN_VER_V      0x45
#define IP_HEADER_LEN_V		 0x14

// ******* IP POSITIONS *******
#define IP_P		         0x0E
#define IP_TOTLEN_H_P		 0x10
#define IP_TOTLEN_L_P		 0x11
#define IP_SRC_P                 0x1A
#define IP_DST_P                 0x1E
#define IP_FLAGS_P               0x14
#define IP_FLAGS_H_P		 0x14
#define IP_FLAGS_L_P		 0x15
#define IP_TTL_P                 0x16
#define IP_PROTO_P		 0x17
#define IP_CHECKSUM_P            0x18
#define IP_HEADER_LEN_VER_P      0x0E

// ******* IP FUNCS *******
#define isIpHeaderLenAndV4()     (_bufferRd[IP_HEADER_LEN_VER_P] == IP_HEADER_LEN_VER_V)
#define isMyIpHeaderByte(i)      (_bufferRd[IP_DST_P + i] == _ip[i])
#define fillDstIp(i)             _bufferWr[IP_DST_P + i] = _bufferRd[IP_SRC_P + i];
#define fillSrcIp(i)             _bufferWr[IP_SRC_P + i] = _ip[i];
// ********************************************************************************************************************************************


// ********************************************************************************************************************************************
// ******* ICMP VALUES *******
#define ICMP_TYPE_ECHOREQUEST_V  0x08
#define ICMP_TYPE_ECHOREPLY_V    0x00

// ******* ICMP POSITIONS *******
#define ICMP_TYPE_P              0x22
#define ICMP_CHECKSUM_P          0x24

// ******* ICMP FUNCS *******
#define isIcmpPacket()           (_bufferRd[IP_PROTO_P] == IP_PROTO_ICMP_V && _bufferRd[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
#define fillIcmpTypeReply()      _bufferWr[ICMP_TYPE_P] = ICMP_TYPE_ECHOREPLY_V
#define isChecksumIcmp()         _bufferWr[ICMP_CHECKSUM_P] > (0xFF - ICMP_TYPE_ECHOREQUEST_V)
#define checksumIcmpIncrement()  _bufferWr[ICMP_CHECKSUM_P + 1]++
#define checksumIcmpIncBy8()     _bufferWr[ICMP_CHECKSUM_P] += 0x08
// ********************************************************************************************************************************************


// ********************************************************************************************************************************************
// Initial Seq Num C0C756EF
// ******* TCP VALUES *******
#define TCP_OPT_LEN_V            0x04
#define TCP_FLAGS_FIN_V		 0x01 //
#define TCP_FLAGS_SYN_V		 0x02 //
#define TCP_FLAGS_RST_V          0x04 //
#define TCP_FLAGS_RSTACK_V       0x14 //
#define TCP_FLAGS_PUSH_V         0x08 //
#define TCP_FLAGS_ACK_V		 0x10 //
#define TCP_FLAGS_FINACK_V	 0x11 //
#define TCP_FLAGS_SYNACK_V 	 0x12 //
#define TCP_FLAGS_PSHACK_V       0x18 //
#define TCP_SEQ_NUM_INI_HH_V     0xC0
#define TCP_SEQ_NUM_INI_HL_V     0xC7
#define TCP_SEQ_NUM_INI_LH_V     0x56
#define TCP_SEQ_NUM_INI_LL_V     0xEF
#define TCP_HEADER_LEN_PLAIN_V   0x14

// ******* TCP POSITIONS *******
#define TCP_SEQ_P                0x26 // the tcp seq number is 4 bytes 0x26-0x29
#define TCP_ACK_P                0x2A // the tcp ack number is 4 bytes 0x2A-0x2D
#define TCP_SRC_PORT_H_P         0x22
#define TCP_SRC_PORT_L_P         0x23
#define TCP_DST_PORT_H_P         0x24
#define TCP_DST_PORT_L_P         0x25
#define TCP_FLAGS_P              0x2F
#define TCP_HEADER_LEN_P         0x2E
#define TCP_CHECKSUM_H_P 	 0x32
#define TCP_CHECKSUM_L_P 	 0x33
#define TCP_OPTIONS_P 		 0x36
#define TCP_DATA_P	         0x36 

// ******* TCP FUNCS *******
#define isTcpPacket()             (_bufferRd[IP_PROTO_P] == IP_PROTO_TCP_V)
#define isTcpMyPort()             (_bufferRd[TCP_DST_PORT_H_P] == 0 && _bufferRd[TCP_DST_PORT_L_P] == _port)
#define isTcpFlagRst()            (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_RST_V)
#define isTcpFlagRstAck()         (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_RSTACK_V)
#define isTcpFlagFin()            (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_FIN_V)
#define isTcpFlagFinAck()         (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_FINACK_V)
#define isTcpFlagSyn()            (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_SYN_V)
#define isTcpFlagSynAck()         (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_SYNACK_V)
#define isTcpFlagPsh()            (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_PUSH_V)
#define isTcpFlagPshAck()         (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_PSHACK_V)
#define isTcpFlagAck()            (_bufferRd[TCP_FLAGS_P] == TCP_FLAGS_ACK_V)
#define isAckZero()               (_bufferRd[TCP_ACK_P]   == 0 && _bufferRd[TCP_ACK_P + 1] == 0 && _bufferRd[TCP_ACK_P + 2] == 0 && _bufferRd[TCP_ACK_P + 3] == 0)

#define fillIpTotalLenWithOptH()  _bufferWr[IP_TOTLEN_H_P]        = 0
#define fillIpTotalLenWithOptL()  _bufferWr[IP_TOTLEN_L_P]        = IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + TCP_OPT_LEN_V
#define fillIpTotalLenH()         _bufferWr[IP_TOTLEN_H_P]        = 0
#define fillIpTotalLenL(l)        _bufferWr[IP_TOTLEN_L_P]        = IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + l
#define getIpTotalLenH()          (_bufferRd[IP_TOTLEN_H_P])
#define getIpTotalLenL()          (_bufferRd[IP_TOTLEN_L_P])

#define fillTcpFlagRst()          _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_RST_V
#define fillTcpFlagFin()          _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_FIN_V
#define fillTcpFlagFinAck()       _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_FINACK_V
#define fillTcpFlagSyn()          _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_SYN_V
#define fillTcpFlagSynAck()       _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_SYNACK_V
#define fillTcpFlagPsh()          _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_PSH_V
#define fillTcpFlagPshAck()       _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_PSHACK_V
#define fillTcpFlagAck()          _bufferWr[TCP_FLAGS_P]          = TCP_FLAGS_ACK_V
#define fillTcpDstPort()          _bufferWr[TCP_DST_PORT_H_P + i] = _bufferRd[TCP_SRC_PORT_H_P + i]
#define clearTcpSrcPort()         _bufferWr[TCP_SRC_PORT_H_P + i] = 0
#define fillTcpSrcPort()          _bufferWr[TCP_SRC_PORT_L_P]     = _port

#define fillTcpSeqInitHH()        _bufferWr[TCP_SEQ_P]            = TCP_SEQ_NUM_INI_HH_V
#define fillTcpSeqInitHL()        _bufferWr[TCP_SEQ_P + 1]        = TCP_SEQ_NUM_INI_HL_V
#define fillTcpSeqInitLH()        _bufferWr[TCP_SEQ_P + 2]        = TCP_SEQ_NUM_INI_LH_V
#define fillTcpSeqInitLL()        _bufferWr[TCP_SEQ_P + 3]        = TCP_SEQ_NUM_INI_LL_V
#define fillTcpSeqAckHH()         _bufferWr[TCP_SEQ_P]            = _bufferRd[TCP_ACK_P]
#define fillTcpSeqAckHL()         _bufferWr[TCP_SEQ_P + 1]        = _bufferRd[TCP_ACK_P + 1]
#define fillTcpSeqAckLH()         _bufferWr[TCP_SEQ_P + 2]        = _bufferRd[TCP_ACK_P + 2]
#define fillTcpSeqAckLL()         _bufferWr[TCP_SEQ_P + 3]        = _bufferRd[TCP_ACK_P + 3]

#define getTcpReadSeq(i)          (_bufferRd[TCP_SEQ_P + i])
#define fillTcpWriteAck(i, a)     _bufferWr[TCP_ACK_P + i]        = a & 0xFF
#define clearTcpChecksumH()       _bufferWr[TCP_CHECKSUM_H_P]     = 0x00
#define clearTcpChecksumL()       _bufferWr[TCP_CHECKSUM_L_P]     = 0x00
#define fillTcpMmsOptHH()         _bufferWr[TCP_OPTIONS_P]        = 0x02
#define fillTcpMmsOptHL()         _bufferWr[TCP_OPTIONS_P + 1]    = 0x04
#define fillTcpMmsOptLH()         _bufferWr[TCP_OPTIONS_P + 2]    = 0x05 
#define fillTcpMmsOptLL()         _bufferWr[TCP_OPTIONS_P + 3]    = 0x80
#define fillTcpHeaderLen()        _bufferWr[TCP_HEADER_LEN_P]     = 0x50
#define fillTcpHeaderLenWithOpt() _bufferWr[TCP_HEADER_LEN_P]     = 0x60
#define fillTcpChecksumH(c)       _bufferWr[TCP_CHECKSUM_H_P]     = c
#define fillTcpChecksumL(c)       _bufferWr[TCP_CHECKSUM_L_P]     = c
#define getTcpSrcPortH()          (_bufferRd[TCP_SRC_PORT_H_P])
#define getTcpSrcPortL()          (_bufferRd[TCP_SRC_PORT_L_P])


//#define getTcpData(i)             (_bufferRd[TCP_DATA_P + i])
//#define getTcpDataPos()           (TCP_DATA_P)
#define fillTcpData(i, c)         _bufferWr[TCP_DATA_P + i]       = c

#define getTcpRequestAction()     ((_bufferRd[TCP_DATA_P] == 'G' && _bufferRd[TCP_DATA_P + 1] == 'E' && _bufferRd[TCP_DATA_P + 2] == 'T') ? "GET" : ((_bufferRd[TCP_DATA_P] == 'P' && _bufferRd[TCP_DATA_P + 1] == 'O' && _bufferRd[TCP_DATA_P + 2] == 'S' && _bufferRd[TCP_DATA_P + 3] == 'T') ? "POST" : "???") )
#define getTcpRequestData(i)      (strncmp(getTcpRequestAction(), "GET", 3) == 0 ? _bufferRd[TCP_DATA_P + i + 5] : (strncmp(getTcpRequestAction(), "POST", 4) == 0 ? _bufferRd[TCP_DATA_P + i + 6] : '\0'))
#define getTcpPayload()           (((((unsigned int)_bufferRd[IP_TOTLEN_H_P])<<8)|(_bufferRd[IP_TOTLEN_L_P]&0xff)-IP_HEADER_LEN_V)-((_bufferRd[TCP_HEADER_LEN_P]>>4)*4))
#define getTcpSrcPort()           ((unsigned int)((((unsigned int)_bufferRd[TCP_SRC_PORT_H_P])<<8) | (_bufferRd[TCP_SRC_PORT_L_P] & 0xFF)))
#define getTcpDstPort()           ((unsigned int)((((unsigned int)_bufferRd[TCP_DST_PORT_H_P])<<8) | (_bufferRd[TCP_DST_PORT_L_P] & 0xFF))) 

// ********************************************************************************************************************************************


// ********************************************************************************************************************************************
// ******* UDP VALUES *******

// ******* UDP POSITIONS *******

// ******* UDP FUNCS *******
#define isUdpPacket()            (_bufferRd[IP_PROTO_P] == IP_PROTO_UDP_V)
// ********************************************************************************************************************************************




#endif
//@}