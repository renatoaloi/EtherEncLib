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
#include <inttypes.h>

#ifndef CHECKSUM_H
#define CHECKSUM_H

#define IP_PROTO_TCP_V		 0x06
#define IP_PROTO_UDP_V		 0x11
#define IP_P		         0x0E
#define IP_FLAGS_P               0x14
#define IP_CHECKSUM_P            0x18
#define IP_TTL_P                 0x16
#define IP_HEADER_LEN_V		 0x14

uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type);
void fillChecksum(uint8_t *buf);

#endif
