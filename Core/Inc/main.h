/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC_I2C2_SDA_Pin GPIO_PIN_0
#define ADC_I2C2_SDA_GPIO_Port GPIOF
#define ADC_I2C2_SCL_Pin GPIO_PIN_1
#define ADC_I2C2_SCL_GPIO_Port GPIOF
#define CPLD_F48_Pin GPIO_PIN_7
#define CPLD_F48_GPIO_Port GPIOF
#define CPLD_F96_Pin GPIO_PIN_8
#define CPLD_F96_GPIO_Port GPIOF
#define DISP_PWM_Pin GPIO_PIN_0
#define DISP_PWM_GPIO_Port GPIOA
#define DISP_STBY_Pin GPIO_PIN_1
#define DISP_STBY_GPIO_Port GPIOA
#define DISP_RST_Pin GPIO_PIN_2
#define DISP_RST_GPIO_Port GPIOA
#define KEYS_PWM_Pin GPIO_PIN_6
#define KEYS_PWM_GPIO_Port GPIOA
#define XMOS_I2C4_SCL_Pin GPIO_PIN_14
#define XMOS_I2C4_SCL_GPIO_Port GPIOF
#define XMOS_I2C4_SDA_Pin GPIO_PIN_15
#define XMOS_I2C4_SDA_GPIO_Port GPIOF
#define CLIP_4_Pin GPIO_PIN_3
#define CLIP_4_GPIO_Port GPIOG
#define CLIP_3_Pin GPIO_PIN_4
#define CLIP_3_GPIO_Port GPIOG
#define CLIP_2_Pin GPIO_PIN_5
#define CLIP_2_GPIO_Port GPIOG
#define CLIP_1_Pin GPIO_PIN_6
#define CLIP_1_GPIO_Port GPIOG
#define OK_BTN_Pin GPIO_PIN_7
#define OK_BTN_GPIO_Port GPIOG
#define DN_BTN_Pin GPIO_PIN_8
#define DN_BTN_GPIO_Port GPIOG
#define PR_BTN_Pin GPIO_PIN_8
#define PR_BTN_GPIO_Port GPIOC
#define FW_BTN_Pin GPIO_PIN_9
#define FW_BTN_GPIO_Port GPIOC
#define UP_BTN_Pin GPIO_PIN_8
#define UP_BTN_GPIO_Port GPIOA
#define CPLD_RST_Pin GPIO_PIN_10
#define CPLD_RST_GPIO_Port GPIOC
#define CPLD_MUTE_Pin GPIO_PIN_11
#define CPLD_MUTE_GPIO_Port GPIOC
#define CPLD_LOCK_Pin GPIO_PIN_12
#define CPLD_LOCK_GPIO_Port GPIOC
#define ADC_EN_Pin GPIO_PIN_0
#define ADC_EN_GPIO_Port GPIOD
#define ADC_CLIP_Pin GPIO_PIN_5
#define ADC_CLIP_GPIO_Port GPIOD
#define XMOS_2CH_Pin GPIO_PIN_12
#define XMOS_2CH_GPIO_Port GPIOG
#define XMOS_RDY_Pin GPIO_PIN_13
#define XMOS_RDY_GPIO_Port GPIOG
#define EE_I2C1_SCL_Pin GPIO_PIN_6
#define EE_I2C1_SCL_GPIO_Port GPIOB
#define EE_I2C1_SDA_Pin GPIO_PIN_7
#define EE_I2C1_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
