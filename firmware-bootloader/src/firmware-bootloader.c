#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#include "core/system.h"
#include "core/uart.h"
#include "core/timer.h"
#include "transport-layer.h"
#include "bl-flash.h"

#define BOOTLOADER_SIZE (0x8000U) // 32 KByte (32768 Byte)
#define MAIN_APPLICATION_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // 0x08000000 + 0x8000 (0x08008000)

#define FLASH_SIZE (0x10000U)

#define MAX_FIRMWARE_LENGTH (FLASH_SIZE - BOOTLOADER_SIZE)

#define UART_PORT (GPIOA)
#define TX_PIN    (GPIO2)
#define RX_PIN    (GPIO3)

#define DEVICE_ID (0x42)

#define SYNC_SEQ_0 (0xc4)
#define SYNC_SEQ_1 (0x55)
#define SYNC_SEQ_2 (0x7e)
#define SYNC_SEQ_3 (0x10)

#define DEFAULT_TIMEOUT (5000)

typedef enum bl_al_state_t {
    BL_AL_STATE_Sync,
    BL_AL_STATE_WaitForUpdateReq,
    BL_AL_STATE_DeviceIDReq,
    BL_AL_STATE_DeviceIDRes,
    BL_AL_STATE_FirmwareLengthReq,
    BL_AL_STATE_FirmwareLengthRes,
    BL_AL_STATE_EraseApplication,
    BL_AL_STATE_ReceiveFirmware,
    BL_AL_STATE_Done
} bl_al_state_t;

static bl_al_state_t state = BL_AL_STATE_Sync;
static uint32_t fw_size = 0;
static uint32_t bytes_written = 0;
static uint8_t sync_seq[4] = {0};
static timer_t timer;
static tl_segment_t temp_segment;

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(GPIOA, GPIO_AF4, TX_PIN | RX_PIN);
}

static void gpio_setup_reset(void) {
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    rcc_periph_clock_disable(RCC_GPIOA);
}

static void jump_to_main(void) {
    vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APPLICATION_START_ADDRESS);
    main_vector_table->reset();
}

static void bootloading_failed(void) {
    tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_NACK);
    tl_write(&temp_segment);
    state = BL_AL_STATE_Done;
}

static void check_for_timeout(void) {
    if (timer_has_elapsed(&timer)) {
        bootloading_failed();
    }
}

static bool is_device_id_message(const tl_segment_t* segment) {
    if (segment->segment_data_size != 2) {
        return false;
    }

    if ((segment->segment_type == SEGMENT_ACK || segment->segment_type == SEGMENT_RETX) && segment->segment_type != 0) {
        return false;
    }

    if (segment->data[0] != BL_AL_MESSAGE_DEVICE_ID_RES) {
        return false;
    }

    for (uint8_t i = 2; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

static bool is_fw_length_message(const tl_segment_t* segment) {
    if (segment->segment_data_size != 5) {
        return false;
    }

    if ((segment->segment_type == SEGMENT_ACK || segment->segment_type == SEGMENT_RETX) && segment->segment_type != 0) {
        return false;
    }

    if (segment->data[0] != BL_AL_MESSAGE_FW_LENGTH_RES) {
        return false;
    }

    for (uint8_t i = 5; i < SEGMENT_DATA_SIZE; i++) {
        if (segment->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

int main(void) {
    system_setup();
    gpio_setup();
    uart_setup();
    tl_setup();
    timer_setup(&timer, DEFAULT_TIMEOUT, false);

    while (state != BL_AL_STATE_Done) {
        if (state == BL_AL_STATE_Sync) {
            if (uart_data_available()) {
                sync_seq[0] = sync_seq[1];
                sync_seq[1] = sync_seq[2];
                sync_seq[2] = sync_seq[3];
                sync_seq[3] = uart_read_byte();

                bool is_match = sync_seq[0] == SYNC_SEQ_0;
                is_match = is_match && (sync_seq[1] == SYNC_SEQ_1);
                is_match = is_match && (sync_seq[2] == SYNC_SEQ_2);
                is_match = is_match && (sync_seq[3] == SYNC_SEQ_3);
            
                if (is_match) {
                    tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_SEQ_OBSERVED);
                    tl_write(&temp_segment);
                    timer_reset(&timer);
                    state = BL_AL_STATE_WaitForUpdateReq;
                } else {
                    check_for_timeout();
                }
            } else {
                check_for_timeout();
            }
            continue;
        }

        tl_update();

        switch (state) {
            case BL_AL_STATE_WaitForUpdateReq: {
                if (tl_segment_available()) {
                    tl_read(&temp_segment);

                    if (tl_is_single_byte_segment(&temp_segment, BL_AL_MESSAGE_FW_UPDATE_REQ)) {
                        timer_reset(&timer);
                        tl_create_single_byte_segment(&temp_segment,  BL_AL_MESSAGE_FW_UPDATE_RES);
                        tl_write(&temp_segment);
                        state = BL_AL_STATE_DeviceIDReq;
                    } else {
                        bootloading_failed();
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            
            case BL_AL_STATE_DeviceIDReq: {
                timer_reset(&timer);
                tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_DEVICE_ID_REQ);
                tl_write(&temp_segment);
                state = BL_AL_STATE_DeviceIDRes;
            } break;
            
            case BL_AL_STATE_DeviceIDRes: {
                if (tl_segment_available()) {
                    tl_read(&temp_segment);

                    if (is_device_id_message(&temp_segment) && temp_segment.data[1] == DEVICE_ID) {
                        timer_reset(&timer);
                        state = BL_AL_STATE_FirmwareLengthReq;
                    } else {
                        bootloading_failed();
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            
            case BL_AL_STATE_FirmwareLengthReq: {
                timer_reset(&timer);
                tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_FW_LENGTH_REQ);
                tl_write(&temp_segment);
                state = BL_AL_STATE_FirmwareLengthRes;
            } break;
            
            case BL_AL_STATE_FirmwareLengthRes: {
                if (tl_segment_available()) {
                    tl_read(&temp_segment);
                    fw_size = (
                        (temp_segment.data[1])       |
                        (temp_segment.data[2] << 8)  |
                        (temp_segment.data[3] << 16) |
                        (temp_segment.data[4] << 24) 
                    );

                    if (is_fw_length_message(&temp_segment) && (fw_size <= MAX_FIRMWARE_LENGTH) && (fw_size % 4 == 0)) {
                        state = BL_AL_STATE_EraseApplication;
                    } else {
                        bootloading_failed();
                    }
                } else {
                    check_for_timeout();
                }
            } break;
            
            case BL_AL_STATE_EraseApplication: {
                BL_FLASH_ERASE_Main_Application();
                timer_reset(&timer);
                tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_READY_FOR_DATA);
                tl_write(&temp_segment);
                state = BL_AL_STATE_ReceiveFirmware; 
            } break;
            
            case BL_AL_STATE_ReceiveFirmware: {
                if (tl_segment_available()) {
                    tl_read(&temp_segment);
                    
                    for (uint8_t i = 0; i < temp_segment.segment_data_size - 4; i = i + 4) {
                        uint32_t firmware_data = (
                            (temp_segment.data[i])           |
                            (temp_segment.data[i + 1] << 8)  |
                            (temp_segment.data[i + 2] << 16) |
                            (temp_segment.data[i + 3] << 24) 
                        ); 
                        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, MAIN_APPLICATION_START_ADDRESS + bytes_written, firmware_data);
                    }
                    timer_reset(&timer);
                    bytes_written += 4;

                    if (bytes_written >= MAX_FIRMWARE_LENGTH) {
                        tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_UPDATE_SUCCESSFUL);
                        tl_write(&temp_segment);
                        state = BL_AL_STATE_Done;
                    } else {
                        tl_create_single_byte_segment(&temp_segment, BL_AL_MESSAGE_READY_FOR_DATA);
                        tl_write(&temp_segment);
                    }
                } else {
                    check_for_timeout();
                }
            } break;

            default: {
                state = BL_AL_STATE_Sync;
            }
        }
    }
    
    // TODO: Reset all system before passing control over to the main application
    system_delay(500);
    uart_setup_reset();
    gpio_setup_reset();
    system_setup_reset(); 
    jump_to_main();

    // Must never return;
    return 0; 
}
