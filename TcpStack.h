/*
 TCP Stack for New EtherEnc based on Enc28Core
 by Renato Aloi NOV 2014
 * seriallink.com.br
 */
#ifndef TCPSTACK_H
#define TCPSTACK_H

#include <Arduino.h>
#include <stdint.h>
//--- made by SKA ---
#include <SPI.h>

#define DATA_SIZE_HARDWARE 	1024 // we can go up to 1500, but for first tests, lets keep it 1K
#define DATA_SIZE		58  // only for TCP Header = ETH_HEADER_LEN_V + IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + 4
				    // for data part: enc28's hadware buffer
				    // by Renato Aloi (May 2015)
#define MAC_SIZE		6
#define IP_SIZE			4
#define DEBUG			1
#define DEBUGLT			1
#define ETH_ARP_LEN     	42

typedef uint8_t  uchar;
typedef uint32_t ulong;
typedef uint16_t uint;
union longb { uchar b[4]; ulong l; };

class TcpStack {
private:
	uint  m_packetId;
	uint  m_sessionPort;
	uint  m_serverPort;
	uchar m_tcpData[DATA_SIZE]; // need only for TCP header, for data: using enc28's hardware buffer (by Renato Aloi - May 2015)
	uchar m_sendData[DATA_SIZE]; // need only for TCP header, also -- Total RAM consumption: 116 bytes
	longb m_seqNum; // Need math, sucks
	longb m_ackNum;
	uchar m_macAddr[MAC_SIZE]; // No need math, passing through
	uchar m_clientMacAddr[MAC_SIZE]; // Client's info
	uchar m_clientIpAddr[IP_SIZE];
	uchar m_ipAddr[IP_SIZE];
	bool  m_established;
	bool  m_buffering;
	bool  m_responding;
	bool  m_closing;
	uint  m_recvPayload;
	uint  m_sendPayload;
	uint  m_sizePayload;
	uint  m_rxPointer;
	uint  m_txPointer; // took me 2 years to uncomment this line! Renato Aloi May 2015
	void  handleStack(void);
	bool  isSession() { return m_sessionPort != 0; };
	void  returnArp(void);
	void  returnIcmp(void);
	void  returnSyn(void);
	void  returnPush(void);
	void  returnHttp(void); //(uchar* _buf, uint _size);
	void  returnClose(void);
	void  returnFin(void);
	void  waitForDMACopy(void);
	unsigned int checksumDMA(unsigned int len);

	int freeRam () {
	  extern int __heap_start, *__brkval;
	  int v;
	  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	};

public:
	TcpStack() : m_responding(false), m_packetId(0), m_rxPointer(0), m_txPointer(0), 
				m_recvPayload(0), m_sendPayload(0), m_sizePayload(0),
				m_buffering(false), m_established(false), m_closing(false),
				m_sessionPort(0), m_serverPort(0)
	{
		m_seqNum.l = 0L;
		m_ackNum.l = 0L;
		for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = 0;
		for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = 0;
		for (unsigned i = 0; i < DATA_SIZE; i++) { m_tcpData[i] = 0; m_sendData[i] = 0; }
	};
	void   setMacAddr(uchar* mac) { for (unsigned i = 0; i < MAC_SIZE; i++)  m_macAddr[i] = mac[i]; };
	void   setIpAddr(uchar* ip) { for (unsigned i = 0; i < IP_SIZE; i++)   m_ipAddr[i] = ip[i]; };
	void   open(uint serverPort);
	//void   write(char *rd, uint len) { returnHttp((uchar*)rd, len); };
	void   write(char c);  
	void   send(void) { returnHttp(); m_txPointer = 0; m_sizePayload = 0; };
	char   read(void); // return char or -1 if reachs end
	void   close(void) { if (m_sizePayload) send(); returnClose(); m_closing = true; };
	bool   established(void) { handleStack(); return m_established; };
	bool   closing(void) { return m_closing; };
	bool   buffering(void) { return m_buffering; };
};

#endif //TCPSTACK_H
