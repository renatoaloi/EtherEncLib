
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

class EtherEncLibUdp
{
  
  private:
	unsigned int m_port;
	UdpStack m_stack;

	int freeRam () {
	  extern int __heap_start, *__brkval;
	  int v;
	  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	};
    
  public:
    EtherEncLibUdp(unsigned int port) : m_port(port), m_stack(UdpStack()) {
	};
   
    void          begin      (unsigned char *ip, unsigned char *mac);
    unsigned char available  (void);
    void          print      (char c);
    void          print      (char *rd);
    void          print      (unsigned int val);
    void          print      (int val);
    void	  print      (char *, unsigned char); // made by SKA
    void          close      (void);
    char	  read       (void);


};

#endif
