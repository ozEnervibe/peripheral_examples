/***************************************************************************//**
 * @file main_s1_xg12.c
 * @brief Demonstrates USART2 as SPI slave.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable 
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

#define RX_BUFFER_SIZE   10
#define TX_BUFFER_SIZE   RX_BUFFER_SIZE

uint8_t RxBuffer[RX_BUFFER_SIZE];
uint8_t RxBufferIndex = 0;

uint8_t TxBuffer[TX_BUFFER_SIZE] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9};
uint8_t TxBufferIndex = 1;

/**************************************************************************//**
 * @brief USART2 TX IRQ Handler
 *****************************************************************************/
void USART2_TX_IRQHandler(void)
{
  // Send and receive incoming data
  USART2->TXDATA = (uint32_t)TxBuffer[TxBufferIndex];
  TxBufferIndex++;

  // Stop sending once we've gone through the whole TxBuffer
  if (TxBufferIndex == TX_BUFFER_SIZE)
  {
    TxBufferIndex = 0;
  }
}

/**************************************************************************//**
 * @brief USART2 RX IRQ Handler
 *****************************************************************************/
void USART2_RX_IRQHandler(void)
{
  if (USART2->STATUS & USART_STATUS_RXDATAV)
  {

    // Read data
    RxBuffer[RxBufferIndex] = USART_RxDataGet(USART2);
    RxBufferIndex++;

    if (RxBufferIndex == RX_BUFFER_SIZE)
    {
      // Putting a break point here to view the full RxBuffer,
      // The RxBuffer should be: 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
      RxBufferIndex = 0;
    }

  }
}

/**************************************************************************//**
 * @brief Initialize USART2
 *****************************************************************************/
void initUSART2 (void)
{
	CMU_ClockEnable(cmuClock_GPIO, true);
	CMU_ClockEnable(cmuClock_USART2, true);

	// Configure GPIO mode
	GPIO_PinModeSet(gpioPortA, 8, gpioModeInput, 1);    // US2_CLK is input
	GPIO_PinModeSet(gpioPortA, 9, gpioModeInput, 1);    // US2_CS is input
	GPIO_PinModeSet(gpioPortA, 6, gpioModeInput, 1);    // US2_TX (MOSI) is input
	GPIO_PinModeSet(gpioPortA, 7, gpioModePushPull, 1); // US2_RX (MISO) is push pull

	// Start with default config, then modify as necessary
	USART_InitSync_TypeDef config = USART_INITSYNC_DEFAULT;
	config.master    = false;
	config.clockMode = usartClockMode0; // clock idle low, sample on rising/first edge
	config.msbf      = true;            // send MSB first
        config.enable    = usartDisable;    // making sure to keep USART disabled until we've set everything up
	USART_InitSync(USART2, &config);
  USART0->CTRL |= USART_CTRL_SSSEARLY;

	// Set USART pin locations
	USART2->ROUTELOC0 = (USART_ROUTELOC0_CLKLOC_LOC1) | // US2_CLK       on location 1 = PA8 per datasheet section 6.4 = EXP Header pin 8
	                    (USART_ROUTELOC0_CSLOC_LOC1)  | // US2_CS        on location 1 = PA9 per datasheet section 6.4 = EXP Header pin 10
	                    (USART_ROUTELOC0_TXLOC_LOC1)  | // US2_TX (MOSI) on location 1 = PA6 per datasheet section 6.4 = EXP Header pin 4
	                    (USART_ROUTELOC0_RXLOC_LOC1);   // US2_RX (MISO) on location 1 = PA7 per datasheet section 6.4 = EXP Header pin 6

	// Enable USART pins
	USART2->ROUTEPEN = USART_ROUTEPEN_CLKPEN | USART_ROUTEPEN_CSPEN | USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  // Enabling TX interrupts to transfer whenever
  // there is room in the transmit buffer
  // This should immediately trigger to load the first byte of our TX buffer
  USART_IntEnable(USART2, USART_IEN_TXBL);
  NVIC_ClearPendingIRQ(USART2_TX_IRQn);
  NVIC_EnableIRQ(USART2_TX_IRQn);
  
	// Enable USART2 RX interrupts
	USART_IntEnable(USART2, USART_IEN_RXDATAV);
	NVIC_ClearPendingIRQ(USART2_RX_IRQn);
	NVIC_EnableIRQ(USART2_RX_IRQn);

	// Enable USART2
	USART_Enable(USART2, usartEnable);
}

/**************************************************************************//**
 * @brief Main function
 *****************************************************************************/
int main(void)
{
  // Initialize chip
  CHIP_Init();

  // Initialize USART2 as SPI slave
  initUSART2();

  //packets will begin coming in as soon as the master code starts
  while(1);
}

