#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "core/system.h"
#include "core/uart.h"

#define BOOTLOADER_SIZE (0x8000U) // 32 KByte (32768 Byte)

static void vector_setup(void) {
    SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF4, GPIO2 | GPIO3);
}

int main(void) {
    vector_setup();
    system_setup();
    gpio_setup();
    uart_setup();
    
    uint32_t start_time = system_get_ticks();

    while (1) {
        if (system_get_ticks() - start_time >= SYSTICK_FREQ) {
            gpio_toggle(GPIOA, GPIO5);
            start_time = system_get_ticks();
        }

        if (uart_data_available()) {
            uint8_t data = uart_read_byte();
            uart_write_byte(data + 1);
        }
    }
    
    return 0;
}
