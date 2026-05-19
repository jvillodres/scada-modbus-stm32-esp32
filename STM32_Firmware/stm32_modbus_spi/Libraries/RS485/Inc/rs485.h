/*
 * rs485.h
 *
 *  Created on: 10/05/2026
 *      Author: Asus
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RS485_INC_RS485_H_
#define RS485_INC_RS485_H_

/* Includes ------------------------------------------------------------------*/
#include "rs485_conf_io.h"

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Definitions ------------------------------------------------------------------*/
#ifndef RS485_UART_PORT
/** @brief	UART port configuration
 */
#define RS485_UART_PORT		huart1
#endif

#ifdef RS485_USE_UART
/** @brief	UART handle configuration
 */
extern UART_HandleTypeDef RS485_UART_PORT;
#endif

/** @defgroup RS485_XE_IO RS485 Driver/Receiver Enable IO
 *  @{
 */
#define RS485_XE_PORT		GPIOA
#define RS485_XE_PIN		GPIO_PIN_8
/**
 *  @}
 */

/* Exported functions ------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup	RS485_Exported_Functions
 * 	@{
 */
/* Peripheral Control functions  ************************************************/
void setTX();
void setRX();

/* IO operation functions *******************************************************/
HAL_StatusTypeDef sendBytes(uint8_t *data, uint16_t len, uint32_t timeout_ms);
HAL_StatusTypeDef recvBytes(uint8_t *data, uint16_t len, uint32_t timeout_ms);
/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif /* RS485_INC_RS485_H_ */
