#ifndef INC_TL_H
#define INC_TL_H

#include "common-defines.h"

#define SEGMENT_DATA_SIZE (32) // Up to 32 Bytes
#define SEGMENT_TYPE_SIZE (1) // 1 Byte
#define SEGMENT_LENGTH_SIZE (1) // 1 Byte
#define SEGMENT_CRC_SIZE (1) // 1 Byte
#define SEGMENT_LENGTH (SEGMENT_DATA_SIZE + SEGMENT_LENGTH_SIZE + SEGMENT_CRC_SIZE + SEGMENT_TYPE_SIZE) // Up to 35 Byte

#define SEGMENT_RETX (1)
#define SEGMENT_ACK (2)

#define BL_AL_MESSAGE_SEQ_OBSERVED (0x20)
#define BL_AL_MESSAGE_FW_UPDATE_REQ (0x31)
#define BL_AL_MESSAGE_FW_UPDATE_RES (0x37)
#define BL_AL_MESSAGE_DEVICE_ID_REQ (0x3C)
#define BL_AL_MESSAGE_DEVICE_ID_RES (0x3F)
#define BL_AL_MESSAGE_FW_LENGTH_REQ (0x42)
#define BL_AL_MESSAGE_FW_LENGTH_RES (0x45)
#define BL_AL_MESSAGE_READY_FOR_DATA (0x48)
#define BL_AL_MESSAGE_UPDATE_SUCCESSFUL (0x54)
#define BL_AL_MESSAGE_NACK (0x59)

typedef struct tl_segment_t {
    uint8_t segment_data_size;
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
bool tl_is_retx_segment(const tl_segment_t* segment);
bool tl_is_ack_segment(const tl_segment_t* segment);
bool tl_is_single_byte_segment(const tl_segment_t* segment, const uint8_t byte);
void tl_create_retx_segment(tl_segment_t* segment);
void tl_create_ack_segment(tl_segment_t* segment);
void tl_create_single_byte_segment(tl_segment_t* segment, uint8_t byte);

#endif
