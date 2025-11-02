#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>

#define CPU_FREQ (32000000)
#define SYSTICK_FREQ (1000)

volatile uint32_t ticks = 0;

void sys_tick_handler(void) {
    ticks++;
}

static uint32_t get_ticks(void) {
    return ticks;
}

static void systick_setup(void) {
    systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
    systick_counter_enable();
    systick_interrupt_enable();
}

const struct rcc_clock_scale pll_32mhz_config = {
    .pll_mul = 1,
    .pll_div = 1,
    .pll_source = RCC_CFGR_PLLSRC_HSI16_CLK,
    .flash_waitstates = 1,
    .voltage_scale = PWR_SCALE1,
    .hpre = 0,
    .ppre1 = 0,
    .ppre2 = 0,
    .ahb_frequency = 32000000,
    .apb1_frequency = 32000000,
    .apb2_frequency = 32000000,
};

static void pll_32mhz_clock_setup(void) {
    rcc_osc_on(RCC_HSI16);
	rcc_wait_for_osc_ready(RCC_HSI16);
    rcc_set_hpre(pll_32mhz_config.hpre);
	rcc_set_ppre1(pll_32mhz_config.ppre1);
	rcc_set_ppre2(pll_32mhz_config.ppre2);
    rcc_periph_clock_enable(RCC_PWR);
	pwr_set_vos_scale(pll_32mhz_config.voltage_scale);
    rcc_osc_off(RCC_PLL);
	while(rcc_is_osc_ready(RCC_PLL));
    flash_prefetch_enable();
	flash_set_ws(pll_32mhz_config.flash_waitstates);
    rcc_set_pll_multiplier(pll_32mhz_config.pll_mul);
	rcc_set_pll_divider(pll_32mhz_config.pll_div);
	rcc_set_pll_source(pll_32mhz_config.pll_source);
    rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	rcc_set_sysclk_source(RCC_PLL);
    while((RCC_CFGR & 0xC) != 0xC);
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

int main(void) {
    pll_32mhz_clock_setup();
    gpio_setup();
    systick_setup();

    uint32_t start_time = get_ticks();

    while (1) {
        if (get_ticks() - start_time >= SYSTICK_FREQ) {
            gpio_toggle(GPIOA, GPIO5);
            start_time = get_ticks();
        }
    }
    
    return 0;
}
