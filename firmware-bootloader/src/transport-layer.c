#include "transport-layer.h"
#include "core/uart.h"
#include "core/crc8.h"

#define SEGMENT_BUFFER_LENGTH (8)

typedef enum tl_state_t {
    TL_State_Segment_Length,
    TL_State_Segment_Type,
    TL_State_Data,
    TL_State_Segment_CRC,
} tl_state_t;

static tl_state_t state = TL_State_Segment_Length;
static uint8_t data_byte_count = 0;

static tl_segment_t temp_segment = { .segment_length = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t retx_segment = { .segment_length = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t ack_segment = { .segment_length = 0, .data = {0}, .segment_crc = 0 };
static tl_segment_t last_transmitted_segment = { .segment_length = 0, .data = {0}, .segment_crc = 0 };

static tl_segment_t segment_buffer[SEGMENT_BUFFER_LENGTH];
static uint32_t segment_read_index = 0;
static uint32_t segment_write_index = 0;
static uint32_t segment_buffer_mask = SEGMENT_BUFFER_LENGTH - 1;

static bool tl_is_retx_segment(const tl_segment_t* segment) {
    if (segment->segment_length != 0) {
        return false;
    }

    if (segment->segment_type != 1) {
        return false;
    }

    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

static bool tl_is_ack_segment(const tl_segment_t* segment) {
    if (segment->segment_length != 0) {
        return false;
    }

    if (segment->segment_type != 2) {
        return false;
    }

    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

static void tl_segment_copy(const tl_segment_t* src, tl_segment_t* dest) {
    dest->segment_length = src->segment_length;
    dest->segment_type = src->segment_type;
    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        dest->data[i] = src->data[i];
    }
    dest->segment_crc = src->segment_crc;
}

void tl_setup(void) {
    retx_segment.segment_length = 0;
    retx_segment.segment_type = SEGMENT_RETX;
    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        retx_segment.data[i] = 0xff;
    }
    retx_segment.segment_crc = tl_compute_crc(&retx_segment);

    ack_segment.segment_length = 0;
    ack_segment.segment_type = SEGMENT_ACK;
    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        ack_segment.data[i] = 0xff;
    }
    ack_segment.segment_crc = tl_compute_crc(&ack_segment);
}

void tl_update(void) {
    while (uart_data_available()) {
        switch (state) {
            case TL_State_Segment_Length: {
                temp_segment.segment_length = uart_read_byte();
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
                    state = TL_State_Segment_Length;
                    break;
                }

                if (tl_is_retx_segment(&temp_segment)) {
                    tl_write(&last_transmitted_segment);
                    state = TL_State_Segment_Length;
                    break;
                }

                if (tl_is_ack_segment(&temp_segment)) {
                    state = TL_State_Segment_Length;
                    break;
                }

                uint32_t next_write_index = (segment_write_index + 1) & segment_buffer_mask;
                if (next_write_index == segment_read_index) {
                    __asm__("BKPT #0");
                }
                
                tl_segment_copy(&temp_segment, &segment_buffer[segment_write_index]);
                segment_write_index = next_write_index;
                tl_write(&ack_segment);
                state = TL_State_Segment_Length;
            } break;

            default: {
                state = TL_State_Segment_Length;
            }
        }
    }
}

bool tl_segment_available(void) {
    return segment_read_index != segment_write_index;
}

void tl_write(tl_segment_t* segment) {
    uart_write((uint8_t*)segment, SEGMENT_LENGTH);
    last_transmitted_segment.segment_length = segment->segment_length;
    last_transmitted_segment.segment_type = segment->segment_type;
    for (uint8_t i = 0; i < SEGMENT_DATA_SIZE; i++) {
        last_transmitted_segment.data[i] = segment->data[i];
    }
    last_transmitted_segment.segment_crc = segment->segment_crc;
}

void tl_read(tl_segment_t* segment) {
    tl_segment_copy(&segment_buffer[segment_read_index], segment);
    segment_read_index = (segment_read_index + 1) & segment_buffer_mask;
}

uint8_t tl_compute_crc(tl_segment_t* segment) {
    return crc8((uint8_t*)segment, SEGMENT_LENGTH - SEGMENT_CRC_SIZE);
}
