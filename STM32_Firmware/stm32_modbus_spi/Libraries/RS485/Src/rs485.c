/*
 * rs485.c
 *
 *  Created on: 10/05/2026
 *      Author: Asus
 */

/* Includes ------------------------------------------------------------------*/
#include "rs485.h"
#include <stdint.h>

/* Exported functions ------------------------------------------------------------------*/
/*
 *	@brief	Set RS485 transceiver into transmit mode
 *	@retval	None
 */
void setTX() {
	HAL_GPIO_WritePin(RS485_XE_PORT, RS485_XE_PIN, GPIO_PIN_SET);
}

/*
 *	@brief	Set RS485 transceiver into receive mode
 *	@retval	None
 */
void setRX() {
	HAL_GPIO_WritePin(RS485_XE_PORT, RS485_XE_PIN, GPIO_PIN_RESET);
}

/*
 *	@brief	Sends an amount of data by TX UART and send it through RS485 diff pair A-B
 *	@note	When UART parity is not enabled (PCE = 0), and Word Length is configured to 9 bits (M1-M0 = 01),
  *         the sent data is handled as a set of u16. In this case, Size must indicate the number
  *         of u16 provided through pData.
 *	@param	data  		Pointer to data buffer (u8 or u16 data elements)
 *	@param	len			Amount of data elements (u8 or u16) to be sent
 *	@param	timeout_ms	Timeout duration
 *	@retval	HAL status
 */
HAL_StatusTypeDef sendBytes(uint8_t *data, uint16_t len, uint32_t timeout_ms) {
	setTX();
	HAL_UART_Transmit(&RS485_UART_PORT, data, len, timeout_ms);
	setRX();

	uint8_t dummy;
	for (int i = 0; i < len; i++) {
		HAL_UART_Receive(&RS485_UART_PORT, &dummy, 1, 10);
	}

	return HAL_OK;
}

/*
 *	@brief	Receives an amount of data by RS485 diff pair A-B data and send it through RX UART
 *	@note	When UART parity is not enabled (PCE = 0), and Word Length is configured to 9 bits (M1-M0 = 01),
  *         the received data is handled as a set of u16. In this case, Size must indicate the number
  *         of u16 available through pData.
 *	@param	data		Pointer to data buffer (u8 or u16 data elements)
 *	@param	len			Amount of data elements (u8 or u16) to be received
 *	@param	timeout_ms	Timeout duration
 *	@retval HAL status
 */
HAL_StatusTypeDef recvBytes(uint8_t *data, uint16_t len, uint32_t timeout_ms) {
	return HAL_UART_Receive(&RS485_UART_PORT, data, len, timeout_ms);
}
