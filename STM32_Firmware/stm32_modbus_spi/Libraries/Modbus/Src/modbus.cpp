/*
 * modbus.cpp
 *
 *  Created on: 10/05/2026
 *      Author: Juan Alejandro Villodres Perdomo
 */

/* Includes ------------------------------------------------------------------*/
#include "rs485.h"
#include "modbus.hpp"

/* Private function prototypes ------------------------------------------------------------------*/
/* CRC related operations */
static uint16_t calculateCRC(const uint8_t *frame, uint16_t len);
static MB_StatusTypeDef verifyCRC(const uint8_t *frame, uint16_t len);

/* Exported functions ------------------------------------------------------------------*/
/*
 * @brief	Modbus class constructor
 * @note	Set object address as '0' for master mode, greater than '0' for slave mode
 * @param	slave_addr	The node role (Master or Slave)
 * @retval	None
 */
Modbus::Modbus(uint8_t slave_addr) : slave_addr_(slave_addr) {}

/*
 * @brief	Change the role of an existing node
 * @note	Set new role as '0' for master mode, greater than '0' for slave mode
 * @param	role	New role for node
 * @retval	None
 */
void Modbus::setRole(uint8_t role) {
	slave_addr_ = role;
}

/*
 * @brief	Get the role of an existing node
 * @note	If node is master, then role will be '0', any other value means slave role and its respective address
 * @retval	uint8_t Role (returned value corresponds to node id and role)
 */
uint8_t Modbus::getRole() {
	return slave_addr_;
}

/*
 * @brief	Handle request made by master node
 * @note	Only works if node is set as slave mode
 * @retval	None
 */
void Modbus::handle() {
	uint8_t addr = 0;

	if (recvBytes(&addr, 1, 10) != HAL_OK) return;

	if (addr != slave_addr_ && addr != 0x00) return;

	uint8_t func = 0;
	if (recvBytes(&func, 1, 50) != HAL_OK) return;

	memset(rx_buf_, 0, sizeof(rx_buf_));

	switch(func) {
	case MB_FC_READ_REGS:
	case MB_FC_WRITE_SINGLE: {
		if (recvBytes(rx_buf_ + 2, 6, 50) != HAL_OK) return;
		rx_buf_[0] = addr;
		rx_buf_[1] = func;

		if (verifyCRC(rx_buf_, 8) != MB_OK) return;

		uint16_t reg = ((uint16_t)rx_buf_[2] << 8) | rx_buf_[3];
		uint16_t value = ((uint16_t)rx_buf_[4] << 8) | rx_buf_[5];

		respondFC(func, reg, value);
		break;
	}
	case MB_FC_WRITE_MULTI: {
		if (recvBytes(rx_buf_ + 2, 5, 50) != HAL_OK) return;

		rx_buf_[0] = addr;
		rx_buf_[1] = func;

		uint16_t count = ((uint16_t)rx_buf_[4] << 8) | rx_buf_[5];

		uint8_t byte_cnt = rx_buf_[6];
		uint32_t data_timeout = 50 + (uint32_t)byte_cnt * 2;
		uint16_t remaining = byte_cnt + 2;

		if (recvBytes(rx_buf_ + 7, remaining, data_timeout) != HAL_OK) return;

		if (verifyCRC(rx_buf_, remaining + 7) != MB_OK) return;

		uint16_t start = ((uint16_t)rx_buf_[2] << 8) | rx_buf_[3];
		respondFC(func, start, count);
		break;
	}
	default:
		sendException(func, MB_EX_ILLEGAL_FUNC);
		break;

	}
}

/*
 * @brief	Answers a request made by a master node
 * @note	Only works if node is set as slave mode
 * @retval	MB_StatusTypeDef Modbus status
 */
MB_StatusTypeDef Modbus::respondFC(uint8_t func, ...) {

	return MB_OK;
}

/*
 * @brief	Sends a request to a specific slave node
 * @note	Only works if node is set as master mode
 * @param	target	Slave id
 * @param	func	Function code
 * @param	start	Addres to start reading or writing data (For multi codes)
 * @param	count	The amount of data to read or the size of data to write (Read multi and Write multi respectively)
 * @param	_reg	Register to write value (For write single only)
 * @param	values	Value or values to write (Write single and Write multi respectively) and pointer to save reads
 * @retval	MB_StatusTypeDef Modbus status
 */
MB_StatusTypeDef Modbus::sendFC(uint8_t target, uint8_t func, uint16_t start, uint16_t count, uint16_t _reg, uint16_t *values) {
	memset(tx_buf_, 0, sizeof(tx_buf_));

	uint16_t idx = 0;
	tx_buf_[idx++] = target;
	tx_buf_[idx++] = func;

	switch(func) {
		case MB_FC_READ_REGS: {
			tx_buf_[idx++] = (uint8_t)(start >> 8);
			tx_buf_[idx++] = (uint8_t)(start & 0xFF);

			tx_buf_[idx++] = (uint8_t)(count >> 8);
			tx_buf_[idx++] = (uint8_t)(count & 0xFF);
			break;
		}
		case MB_FC_WRITE_SINGLE: {
			tx_buf_[idx++] = (uint8_t)(_reg >> 8);
			tx_buf_[idx++] = (uint8_t)(_reg & 0xFF);

			tx_buf_[idx++] = (uint8_t)((*values) >> 8);
			tx_buf_[idx++] = (uint8_t)((*values) & 0xFF);
			break;
		}
		case MB_FC_WRITE_MULTI: {
			tx_buf_[idx++] = (uint8_t)(start >> 8);
			tx_buf_[idx++] = (uint8_t)(start & 0xFF);

			tx_buf_[idx++] = (uint8_t)(count >> 8);
			tx_buf_[idx++] = (uint8_t)(count & 0xFF);

			tx_buf_[idx++] = (uint8_t)(count * 2);

			for (uint16_t i = 0; i < count; i++) {
				tx_buf_[idx++] = (uint8_t)(values[i] >> 8);
				tx_buf_[idx++] = (uint8_t)(values[i] & 0xFF);
			}
			break;
		}
		default:
			sendException(func, MB_EX_ILLEGAL_FUNC);
			return MB_ERR_EXCEPTION;
	}

	uint16_t crc = calculateCRC(tx_buf_, idx);
	tx_buf_[idx++] = (uint8_t)(crc & 0xFF);
	tx_buf_[idx++] = (uint8_t)(crc >> 8);

	sendBytes(tx_buf_, idx, 200);

	if (func == MB_FC_READ_REGS) idx = 5 + count * 2;

	if (recvBytes(rx_buf_, idx, 100) != HAL_OK) return MB_ERR_TIMEOUT;
	if (verifyCRC(rx_buf_, idx) != MB_OK) return MB_ERR_CRC;
	if (rx_buf_[1] & 0x80) return MB_ERR_EXCEPTION;

	if (func == MB_FC_READ_REGS) for (uint16_t i = 0; i < count; i++) values[i] = ((uint16_t)rx_buf_[3 + i * 2] << 8) | rx_buf_[4 + i *2];

	return MB_OK;
}

/*
 * @brief	Handle Modbus exceptions
 * @param	func	Function code
 * @param	ex_code	Exception code
 * @retval	MB_StatusTypeDef Modbus status
 */
MB_StatusTypeDef Modbus::sendException(uint8_t func, uint8_t ex_code) {
	memset(tx_buf_, 0, sizeof(tx_buf_));

	tx_buf_[0] = slave_addr_;
	tx_buf_[1] = func | 0x80;
	tx_buf_[2] = ex_code;

	uint16_t crc = calculateCRC(tx_buf_, 3);
	tx_buf_[3] = (uint8_t)(crc & 0xFF);
	tx_buf_[4] = (uint8_t)(crc >> 8);

	if (sendBytes(tx_buf_, 5, 100) != HAL_OK) return MB_ERR_TIMEOUT;

	return MB_OK;
}

/* Private functions ------------------------------------------------------------------*/
/*
 * @brief	Calculate CRC-16
 * @param	frame	The frame to calculate CRC
 * @param	len		The length of the frame
 * @retval	uint16_t CRC
 */
static uint16_t calculateCRC(const uint8_t *frame, uint16_t len) {
	uint16_t crc = 0xFFFF;
	for (int i = 0; i < len; i++) {
		crc ^= frame[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 0x0001) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}

	return crc;
}

/*
 * @brief	Verify a frame integrity by CRC
 * @param	frame	The frame to verify
 * @param	len		The length of the frame
 * @retval	MB_StatusTypeDef Modbus status
 */
static MB_StatusTypeDef verifyCRC(const uint8_t *frame, uint16_t len) {
	if (len < 4) return MB_ERR_CRC;

	uint16_t recv = (uint16_t)frame[len - 2] | ((uint16_t)frame[len - 1] << 8);
	uint16_t calc = calculateCRC(frame, len);

	if (recv == calc) return MB_OK;

	return MB_ERR_CRC;
}
