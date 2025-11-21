#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#include "core/system.h"
#include "core/uart.h"
#include "transport-layer.h"

#define BOOTLOADER_SIZE (0x8000U) // 32 KByte (32768 Byte)
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // 0x08000000 + 0x8000 (0x08008000)

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF4, GPIO2 | GPIO3);
}

static void jump_to_main(void) {
    vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APP_START_ADDRESS);
    main_vector_table->reset();
}

int main(void) {
    system_setup();
    gpio_setup();
    uart_setup();
    tl_setup();

    tl_segment_t segment = {
        .segment_length = 32,
        .segment_type = 0,
        .data = {[0 ... 31] = 0xff},
        .segment_crc = 0
    };
    segment.segment_crc = tl_compute_crc(&segment);

    while (true) {
        tl_write(&segment);
        system_delay(1000);
    }

    jump_to_main();
    return 0;
}
