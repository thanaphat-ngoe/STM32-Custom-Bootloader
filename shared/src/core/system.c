#include "core/system.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>

static volatile uint32_t ticks = 0;

void sys_tick_handler(void) {
    ticks++;
}

static void systick_setup(void) {
    systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint32_t system_get_ticks(void) {
    return ticks;
}

const struct rcc_clock_scale pll_32mhz_config = {
    .pll_mul = RCC_CFGR_PLLMUL_MUL4,
    .pll_div = RCC_CFGR_PLLDIV_DIV2,
    .pll_source = RCC_CFGR_PLLSRC_HSI16_CLK,
    .flash_waitstates = 1,
    .voltage_scale = PWR_SCALE1,
    .hpre = RCC_CFGR_HPRE_NODIV,
    .ppre1 = RCC_CFGR_HPRE_NODIV,
    .ppre2 = RCC_CFGR_HPRE_NODIV,
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

void system_setup(void) {
    pll_32mhz_clock_setup();
    systick_setup();
}
