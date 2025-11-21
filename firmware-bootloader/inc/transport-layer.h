#ifndef INC_TL_H
#define INC_TL_H

#include "common-defines.h"

#define SEGMENT_TYPE_SIZE (1) // 1 Byte
#define SEGMENT_DATA_SIZE (32) // Up to 32 Byte
#define SEGMENT_LENGTH_SIZE (1) // 1 Byte
#define SEGMENT_CRC_SIZE (1) // 1 Byte
#define SEGMENT_LENGTH (SEGMENT_DATA_SIZE + SEGMENT_LENGTH_SIZE + SEGMENT_CRC_SIZE + SEGMENT_TYPE_SIZE) // Up to 35 Byte

#define SEGMENT_RETX (1)
#define SEGMENT_ACK (2)

typedef struct tl_segment_t {
    uint8_t segment_length;
    uint8_t segment_type;
    uint8_t data[SEGMENT_DATA_SIZE];
    uint8_t segment_crc;
} tl_segment_t;

void tl_setup(void);
void tl_update(void);

bool tl_segment_available(void);
void tl_write(tl_segment_t* segment);
void tl_read(tl_segment_t* segment);
uint8_t tl_compute_crc(tl_segment_t* segment);

#endif
