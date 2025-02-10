/*
 * Firmware for the Portenta X8 STM32H747AIIX/Cortex-M7 core.
 * Copyright (C) 2022 Arduino (http://www.arduino.cc/)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <stdio.h>

#include "peripherals.h"
#include "can.h"
#include "can_handler.h"
#include "rpc.h"
#include "adc.h"
#include "adc_handler.h"
#include "uart.h"
#include "uart_handler.h"
#include "virtual_uart.h"
#include "virtual_uart_handler.h"
#include "pwm.h"
#include "pwm_handler.h"
#include "gpio.h"
#include "gpio_handler.h"
#include "timer.h"
#include "rtc.h"
#include "rtc_handler.h"
#include "spi.h"
#include "system.h"
#include "h7_handler.h"
#include "watchdog.h"
#include "m4_util.h"

/**************************************************************************************
 * FUNCTION DEFINITION
 **************************************************************************************/

void peripheral_init()
{
  peripheral_register_callback(PERIPH_H7, &h7_handler);

  uart_init();
  peripheral_register_callback(PERIPH_UART, &uart_handler);

  virtual_uart_init();
  peripheral_register_callback(PERIPH_VIRTUAL_UART, &virtual_uart_handler);

  gpio_init();
  peripheral_register_callback(PERIPH_GPIO, &gpio_handler);

  pwm_init();
  peripheral_register_callback(PERIPH_PWM, &pwm_handler);

  dma_init();

  timer_init();

  rtc_init();
  peripheral_register_callback(PERIPH_RTC, &rtc_handler);

  spi_init();

  dma_load(false);

  adc_init();
  peripheral_register_callback(PERIPH_ADC, &adc_handler);

  peripheral_register_callback(PERIPH_FDCAN1, &fdcan1_handler);
  peripheral_register_callback(PERIPH_FDCAN2, &fdcan2_handler);
}

void handle_data()
{
  __WFI();

  watchdog_refresh();

  if (uart_data_available())
    uart_handle_data();

  if (virtual_uart_data_available())
    virtual_uart_handle_data();

  can_handle_data();
  gpio_handle_data();
  dma_handle_data();

  {
    /* Enter critical section. */
    uint32_t primask_bit = __get_PRIMASK();
    __set_PRIMASK(1) ;

    if (!is_nirq_low() && !is_ncs_low() && (get_tx_packet_size() > 0))
    {
      dma_load(true);
      set_nirq_low();
    }

    /* Exit critical section: restore previous priority mask */
    __set_PRIMASK(primask_bit);
  }
}

/**************************************************************************************
 * MAIN
 **************************************************************************************/

int main(void)
{
  gpio_init_nirq();

  system_init();

  peripheral_init();

  disableCM4Autoboot();

  extern char const REAL_VERSION_FLASH[];
  printf("Portenta X8 - STM32H7 companion fw - %s\n", REAL_VERSION_FLASH);

  try_execute_m4_app();

  watchdog_init(IWDG_PRESCALER_16);

  gpio_init_ncs();

  for(;;) {
    handle_data();
  }

  /* Never return. */
}
