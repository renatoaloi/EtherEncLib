
#include <inttypes.h>

#ifndef CHECKSUM_H
#define CHECKSUM_H

uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type);
void fillChecksum(uint8_t *buf);

#endif