
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