/*
 * modbus.hpp
 *
 *  Created on: 10/05/2026
 *      Author: Asus
 */

#ifndef MODBUS_INC_MODBUS_HPP_
#define MODBUS_INC_MODBUS_HPP_

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>
#include "rs485.h"

/* Exported types ------------------------------------------------------------------*/
//extern CRC_HandleTypeDef hcrc;

/**
 * 	@brief	 ModBus Status structures definition
 */
typedef enum {
	MB_OK = 0x00U,
	MB_ERR_CRC = 0x01U,
	MB_ERR_EXCEPTION = 0x02U,
	MB_ERR_TIMEOUT = 0x03U
} MB_StatusTypeDef;

/**
 *	@brief	ModBus Function structures definition
 */
typedef enum {
	MB_FC_READ_REGS = 0x03U,
	MB_FC_WRITE_SINGLE = 0x06U,
	MB_FC_WRITE_MULTI = 0x10U
} MB_FunctionTypeDef;

/**
 *	@brief	ModBus Error structures definition
 */
typedef enum {
	MB_EX_ILLEGAL_FUNC = 0x01U,
	MB_EX_ILLEGAL_ADDR = 0x02U
} MB_ErrorTypeDef;

/* Exported macro ------------------------------------------------------------------*/
#define MODBUS_MAX_REGISTERS		18

/* Exported class ------------------------------------------------------------------*/
/*
 * @brief
 * @param
 */
class Modbus {
private:
	uint8_t slave_addr_;
	uint8_t rx_buf_[128];
	uint8_t tx_buf_[128];

	MB_StatusTypeDef respondFC(uint8_t func, ...);
	MB_StatusTypeDef sendException(uint8_t func, uint8_t ex_code);
public:
	Modbus(uint8_t slave_addr);

	void handle();
	void setRole(uint8_t role);
	uint8_t getRole();

	MB_StatusTypeDef sendFC(uint8_t target, uint8_t func, uint16_t start, uint16_t count, uint16_t _reg, uint16_t *values);
};
#endif /* MODBUS_INC_MODBUS_HPP_ */
