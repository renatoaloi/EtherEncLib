
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

#include <Arduino.h>
#include "UdpStack.h"

#ifndef ETHERENCLIBUDP_H
#define ETHERENCLIBUDP_H

#define DEBUGLIBUDP           0
#define UDP_TX_PACKET_MAX_SIZE 24

class EtherEncLibUdp
{
  
  private:
	unsigned int m_port;
	UdpStack m_stack;

	void          print      (char c);
    
  public:
    EtherEncLibUdp(unsigned int port) : m_port(port), m_stack(UdpStack()) { };
   
    void          begin      (unsigned char *ip, unsigned char *mac);
    char	  read       (void);

  // Start processing the next available incoming packet
  // Returns the size of the packet in bytes, or 0 if no packets are available
  int parsePacket(void);

  // Return the IP address of the host who sent the current incoming packet
  unsigned char * remoteIP(void) { 
	uchar ip[] = { 0, 0, 0, 0 };
	uchar *p = &ip[0];
	p = m_stack.getRemoteIp(); 
	return p; 
	};

  long parseInt(); // { return 0L; }; // returns the first valid (long) integer value from the current position.
  // initial characters that are not digits (or the minus sign) are skipped
  // integer is terminated by the first character that is not a digit.

  // Start building up a packet to send to the remote host specific in ip and port
  // Returns 1 if successful, 0 if there was a problem with the supplied IP address or port
  int beginPacket(unsigned char *ip, uint16_t port);
  
  // Finish off this packet and send it
  // Returns 1 if the packet was sent successfully, 0 if there was an error
  int endPacket();

  void setTimeout(unsigned long timeout) { };  // sets maximum milliseconds to wait for stream data, default is 1 second

  void          print      (float val);
  void          print      (char *rd);
  void          print      (unsigned int val);
  void          print      (int val);

};

#endif
