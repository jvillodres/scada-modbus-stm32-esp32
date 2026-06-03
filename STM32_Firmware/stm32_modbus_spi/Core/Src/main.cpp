/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "modbus.hpp"
#include <cstdio>
#include <cstring>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_REGS		7
#define SLAVE_ADDR		0x01
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
Modbus mb(0); // Master mode
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_IWDG_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static uint8_t crc8(uint8_t *data, uint8_t len);
static void update_spi_tx_buf(void);
static void update_uart_tx_buf(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// PLC registers starting on 40001
uint16_t mb_regs[NUM_REGS] = {0};

// SPI Buffers
uint8_t spi_rx_buf[10] = {0};
uint8_t spi_tx_buf[10] = {
		0xBB,		// Start byte (ID)
		0x00,		// Motor
		0x00,		// Speed
		0x00,		// Arm
		0x00,		// Presence
		0x00,		// Temperature
		0x00,		// Counter Hi
		0x00,		// Counter Lo
		0x00,		// Alarm
		0x00		// CRC8
};

// UART Buffers
uint8_t uart_rx_buf[10] = {0};
uint8_t uart_tx_buf[10] = {
		0xDD,		// Start byte (ID)
		0x00,		// Motor
		0x00,		// Speed
		0x00,		// Arm
		0x00,		// Presence
		0x00,		// Temperature
		0x00,		// Counter Hi
		0x00,		// Counter Lo
		0x00,		// Alarm
		0x00		// CRC8
};

// Pending changes from SCADA to PLC
volatile uint8_t scada_pending_flag = 0x00; // Flag
volatile uint8_t scada_pending_motor = 0x00; // Value queued
volatile uint8_t scada_pending_speed = 0x00;
volatile uint8_t scada_pending_arm = 0x00;

// Pending changes from HMI to PLC
volatile uint8_t hmi_pending_flag = 0x00; // Flag
volatile uint8_t hmi_pending_motor = 0x00; // Value queued
volatile uint8_t hmi_pending_speed = 0x00;
volatile uint8_t hmi_pending_arm = 0x00;
volatile uint8_t hmi_frame_ready = 0x00; // Receive flag
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_Base_Start_IT(&htim2);

  spi_tx_buf[9] = crc8(&spi_tx_buf[1], 8);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_tx_buf, spi_rx_buf, 10);

  uart_tx_buf[9] = crc8(&uart_tx_buf[1], 8);
  HAL_UART_Receive_IT(&huart2, uart_rx_buf, 10);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t now = HAL_GetTick();
  while (1)
  {
	if (hmi_frame_ready) {
		hmi_frame_ready = 0x00;

		if (uart_rx_buf[0] == 0xCC) {
			uint8_t calc = crc8(&uart_rx_buf[1], 8);

			if (calc == uart_rx_buf[9]) {
				if (uart_rx_buf[1] != 0xFF) hmi_pending_motor = uart_rx_buf[1]; // Motor
				if (uart_rx_buf[2] != 0xFF) hmi_pending_speed = uart_rx_buf[2]; // Speed
				if (uart_rx_buf[3] != 0xFF) hmi_pending_arm = uart_rx_buf[3]; // Arm

				if (uart_rx_buf[1] != 0xFF || uart_rx_buf[2] != 0xFF || uart_rx_buf[3] != 0xFF) {
					hmi_pending_flag = 0x01;
				}

				update_uart_tx_buf();
				HAL_StatusTypeDef st = HAL_UART_Transmit(&huart2, uart_tx_buf, 10, 100);

			}
		}
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	if ((HAL_GetTick() - now) >= 200) {
		now = HAL_GetTick();
		uint16_t val;

		if (hmi_pending_flag) {
			if (hmi_pending_motor != 0xFF) {
				val = hmi_pending_motor;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 0, &val);
			}
			if (hmi_pending_speed != 0xFF) {
				val = hmi_pending_speed;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 1, &val);
			}
			if (hmi_pending_arm != 0xFF) {
				val = hmi_pending_arm;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 2, &val);
			}

			hmi_pending_motor = hmi_pending_speed = hmi_pending_arm = 0xFF;
			hmi_pending_flag = 0x00;
		}

		if (scada_pending_flag) {
			if (scada_pending_motor != 0xFF) {
				val = scada_pending_motor;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 0, &val);
			}
			if (scada_pending_speed != 0xFF) {
				val = scada_pending_speed;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 1, &val);
			}
			if (scada_pending_arm != 0xFF) {
				val = scada_pending_arm;
				mb.sendFC(SLAVE_ADDR, MB_FC_WRITE_SINGLE, 0, 1, 2, &val);
			}

			scada_pending_motor = scada_pending_speed = scada_pending_arm = 0xFF;
			scada_pending_flag = 0x00;
		}

		MB_StatusTypeDef st = mb.sendFC(SLAVE_ADDR, MB_FC_READ_REGS, 0, NUM_REGS, 0, mb_regs);

		if (st == MB_OK) {
			update_spi_tx_buf();
		}
	}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 1279;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 10000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 10000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 6398;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Keep_Alive_GPIO_Port, Keep_Alive_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USART1_XE_GPIO_Port, USART1_XE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Keep_Alive_Pin */
  GPIO_InitStruct.Pin = Keep_Alive_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Keep_Alive_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USART1_XE_Pin */
  GPIO_InitStruct.Pin = USART1_XE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USART1_XE_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static uint8_t crc8(uint8_t *data, uint8_t len) {
	uint8_t crc = 0;
	for (int i = 0; i < len; i++) {
		crc ^= data[i];
	}
	return crc;
}

static void update_spi_tx_buf(void) {

	uint8_t tmp[10];

	tmp[0] = 0xBB;
	tmp[1] = (uint8_t) mb_regs[0];				// motor
	tmp[2] = (uint8_t) mb_regs[1];				// speed
	tmp[3] = (uint8_t) mb_regs[2];				// arm
	tmp[4] = (uint8_t) mb_regs[3];				// presence
	tmp[5] = (uint8_t) mb_regs[4];				// temperature
	tmp[6] = (uint8_t) (mb_regs[5] >> 8);		// count hi
	tmp[7] = (uint8_t) (mb_regs[5] & 0xFF);		// count lo
	tmp[8] = (uint8_t) mb_regs[6];				// overheat
	tmp[9] = crc8(&spi_tx_buf[1], 8);

	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	memcpy(spi_tx_buf, tmp, 10);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
}

static void update_uart_tx_buf(void) {
	uint8_t tmp[10];

	tmp[0] = 0xDD;
	tmp[1] = (uint8_t) mb_regs[0];				// motor
	tmp[2] = (uint8_t) mb_regs[1];				// speed
	tmp[3] = (uint8_t) mb_regs[2];				// arm
	tmp[4] = (uint8_t) mb_regs[3];				// presence
	tmp[5] = (uint8_t) mb_regs[4];				// temperature
	tmp[6] = (uint8_t) (mb_regs[5] >> 8);		// count hi
	tmp[7] = (uint8_t) (mb_regs[5] & 0xFF);		// count lo
	tmp[8] = (uint8_t) mb_regs[6];				// overheat
	tmp[9] = crc8(&tmp[1], 8);

	HAL_NVIC_DisableIRQ(USART2_IRQn);
	memcpy(uart_tx_buf, tmp, 10);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == SPI1) {

		// Verify received frame from ESP32
		if (spi_rx_buf[0] == 0xAA) {
			uint8_t calc = crc8(&spi_rx_buf[1], 8);

			if (calc == spi_rx_buf[9]) {

				if (spi_rx_buf[1] != 0xFF) scada_pending_motor = spi_rx_buf[1]; // Motor
				if (spi_rx_buf[2] != 0xFF) scada_pending_speed = spi_rx_buf[2]; // Speed
				if (spi_rx_buf[3] != 0xFF) scada_pending_arm = spi_rx_buf[3]; // Arm

				if (spi_rx_buf[1] != 0xFF || spi_rx_buf[2] != 0xFF || spi_rx_buf[3] != 0xFF) {
					scada_pending_flag = 0x01;
				}
			}
		}

		// Recalculate CRC and arm next transaction
		HAL_SPI_TransmitReceive_IT(&hspi1, spi_tx_buf, spi_rx_buf, 10);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		hmi_frame_ready = 0x01;
		HAL_UART_Receive_IT(&huart2, uart_rx_buf, 10);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1) {
		HAL_GPIO_TogglePin(Keep_Alive_GPIO_Port, Keep_Alive_Pin);
	}

	if (htim->Instance == TIM2) {
		if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK) {
			Error_Handler();
		}

	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
