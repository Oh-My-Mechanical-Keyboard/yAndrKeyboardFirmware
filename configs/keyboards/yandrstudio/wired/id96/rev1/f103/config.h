/* Copyright 2021 JasonRen(biu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "config_common.h"
/* RGB Matrix */
#ifdef RGB_MATRIX_ENABLE
#    define WS2812_PWM_DRIVER PWMD2  // default: PWMD2
#    define WS2812_PWM_CHANNEL 4  // default: 2
#    define WS2812_DMA_STREAM STM32_DMA1_STREAM4  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#    define WS2812_DMA_CHANNEL 4  // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#endif


#ifdef RGBLIGHT_ENABLE
#    define WS2812_PWM_DRIVER PWMD2  // default: PWMD2
#    define WS2812_PWM_CHANNEL 4  // default: 2
#    define WS2812_DMA_STREAM STM32_DMA1_STREAM4  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#    define WS2812_DMA_CHANNEL 4  // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#endif
