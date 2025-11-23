#include "transport-layer.h"
#include "core/uart.h"
#include "core/crc8.h"

#include "string.h"

#define SEGMENT_BUFFER_LENGTH (8)

typedef enum tl_state_t {
    TL_State_Segment_Data_Size,
    TL_State_Segment_Type,
    TL_State_Data,
    TL_State_Segment_CRC,
} tl_state_t;

static tl_state_t state = TL_State_Segment_Data_Size;
static uint8_t data_byte_count = 0;

static tl_segment_t temp_segment = { .segment_data_size = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t retx_segment = { .segment_data_size = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t ack_segment = { .segment_data_size = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t last_transmitted_segment = { .segment_data_size = 0, .data = {0}, .segment_crc = 0 };

static tl_segment_t segment_buffer[SEGMENT_BUFFER_LENGTH];
static uint32_t segment_read_index = 0;
static uint32_t segment_write_index = 0;
static uint32_t segment_buffer_mask = SEGMENT_BUFFER_LENGTH - 1;

bool tl_is_retx_segment(const tl_segment_t* segment) {
    if (segment->segment_data_size != 0) {
        return false;
    }

    if (segment->segment_type != SEGMENT_RETX) {
        return false;
    }

    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

bool tl_is_ack_segment(const tl_segment_t* segment) {
    if (segment->segment_data_size != 0) {
        return false;
    }

    if (segment->segment_type != SEGMENT_ACK) {
        return false;
    }

    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

bool tl_is_single_byte_segment(const tl_segment_t* segment, const uint8_t byte) {
    if (segment->segment_data_size == 0 && segment->segment_data_size > 1) {
        return false;
    }

    if ((segment->segment_type == SEGMENT_ACK || segment->segment_type == SEGMENT_RETX) && segment->segment_type != 0) {
        return false;
    }

    if (segment->data[0] != byte) {
        return false;
    }

    for (uint8_t i = 1; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

void tl_create_retx_segment(tl_segment_t* segment) {
    memset(segment, 0xff, sizeof(tl_segment_t));
    segment->segment_data_size = 0;
    segment->segment_type = SEGMENT_RETX;
    segment->segment_crc = tl_compute_crc(segment);
}

void tl_create_ack_segment(tl_segment_t* segment) {
    memset(segment, 0xff, sizeof(tl_segment_t));
    segment->segment_data_size = 0;
    segment->segment_type = SEGMENT_ACK;
    segment->segment_crc = tl_compute_crc(segment);
}

void tl_create_single_byte_segment(tl_segment_t* segment, uint8_t byte) {
    memset(segment, 0xff, sizeof(tl_segment_t));
    segment->segment_data_size = 1;
    segment->segment_type = 0;
    segment->data[0] = byte;
    segment->segment_crc = tl_compute_crc(segment);
}

void tl_setup(void) {
    tl_create_retx_segment(&retx_segment);
    tl_create_ack_segment(&ack_segment);
}

void tl_update(void) {
    while (uart_data_available()) {
        switch (state) {
            case TL_State_Segment_Data_Size: {
                temp_segment.segment_data_size = uart_read_byte();
                state = TL_State_Segment_Type;
            }  break;
            
            case TL_State_Segment_Type: {
                temp_segment.segment_type = uart_read_byte();
                state = TL_State_Data;
            } break;

            case TL_State_Data: {
                temp_segment.data[data_byte_count++] = uart_read_byte();
                if (data_byte_count >= SEGMENT_DATA_SIZE) {
                    data_byte_count = 0;
                    state = TL_State_Segment_CRC;
                }
            } break;

            case TL_State_Segment_CRC: {
                temp_segment.segment_crc = uart_read_byte();
                if (temp_segment.segment_crc != tl_compute_crc(&temp_segment)) {
                    tl_write(&retx_segment);
                    state = TL_State_Segment_Data_Size;
                    break;
                }

                if (tl_is_retx_segment(&temp_segment)) {
                    tl_write(&last_transmitted_segment);
                    state = TL_State_Segment_Data_Size;
                    break;
                }

                if (tl_is_ack_segment(&temp_segment)) {
                    state = TL_State_Segment_Data_Size;
                    break;
                }

                uint32_t next_write_index = (segment_write_index + 1) & segment_buffer_mask;
                if (next_write_index == segment_read_index) {
                    __asm__("BKPT #0");
                }
                
                memcpy(&segment_buffer[segment_write_index], &temp_segment, sizeof(tl_segment_t));
                segment_write_index = next_write_index;
                tl_write(&ack_segment);
                state = TL_State_Segment_Data_Size;
            } break;

            default: {
                state = TL_State_Segment_Data_Size;
            }
        }
    }
}

bool tl_segment_available(void) {
    return segment_read_index != segment_write_index;
}

void tl_write(tl_segment_t* segment) {
    uart_write((uint8_t*)segment, SEGMENT_LENGTH);
    memcpy(&last_transmitted_segment, segment, sizeof(tl_segment_t));
}

void tl_read(tl_segment_t* segment) {
    memcpy(segment, &segment_buffer[segment_read_index], sizeof(tl_segment_t));
    segment_read_index = (segment_read_index + 1) & segment_buffer_mask;
}

uint8_t tl_compute_crc(tl_segment_t* segment) {
    return crc8((uint8_t*)segment, SEGMENT_LENGTH - SEGMENT_CRC_SIZE);
}
