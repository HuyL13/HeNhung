/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_STEP_DELAY_MS 150U

/* LED 7 thanh common-cathode theo so do PDF.
   PE8=A, PE9=B, PE10=C, PE11=D,
   PE12=E, PE13=F, PE14=G, PE15=DP.
   PG2: hang chuc; PG3: hang don vi.
   Transistor NPN: muc HIGH bat module. */
#define SEGMENT_PINS            ((uint16_t)0xFF00U)
#define DIGIT_TENS_PIN          GPIO_PIN_2
#define DIGIT_UNITS_PIN         GPIO_PIN_3
#define SEG_SCAN_INTERVAL_MS    2U
#define BUTTON_DEBOUNCE_MS      30U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */
int TIM6_Count;
uint8_t LED_Value;

/* Phan tu lam: moi hieu ung chay 2 chu ky. */
static uint8_t LED_Mode = 1;
static uint8_t LED_Step = 0;
static uint8_t LED_Cycles = 0;

/* Bit 7 ung voi PD15, bit 0 ung voi PD8.
   Chieu trai/phai theo thu tu LED trong vi du mau cua PDF. */
static const uint8_t LED_Patterns[4][10] = {
  /* Mode 1: Running spot L. */
  {0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00},

  /* Mode 2: Running spot R. */
  {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00},

  /* Mode 3: Flash. */
  {0x00, 0xFF, 0x00},

  /* Mode 4: Spot bumber; giu cap LED giua trong 2 buoc. */
  {0x00, 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81, 0x00}
};

static const uint8_t LED_PatternLength[4] = {10, 10, 3, 10};

/* Bit 0..6: A..G; bit 7: DP. */
static const uint8_t SEG_DigitCode[10] = {
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F  /* 9 */
};

static uint8_t SEG_DisplayValue = 25;
static uint8_t SEG_NextDigit = 0;
static uint32_t SEG_LastScanTick = 0;
static uint32_t LED_LastStepTick = 0;

static GPIO_PinState B1_LastRawState = GPIO_PIN_RESET;
static GPIO_PinState B1_StableState = GPIO_PIN_RESET;
static uint32_t B1_LastChangeTick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
void DisplayLEDs(int mode);

void Set7SegDisplayValue(int value);
void Run7SegDisplay(void);
static void MX_7SegButton_Init(void);
static void ProcessButtonB1(void);
static void RunLabApplication(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void DisplayLEDs(int mode)
{
  /* Phan bo sung: chon trang thai LED theo mode va buoc hien tai. */
  uint8_t index;

  if ((mode < 1) || (mode > 4))
  {
    return;
  }

  if (LED_Mode != (uint8_t)mode)
  {
    LED_Mode = (uint8_t)mode;
    LED_Step = 0;
    LED_Cycles = 0;
  }

  index = (uint8_t)(mode - 1);
  LED_Value = LED_Patterns[index][LED_Step];

  LED_Step++;
  if (LED_Step >= LED_PatternLength[index])
  {
    LED_Step = 0;
    LED_Cycles++;

    if (LED_Cycles >= 2)
    {
      LED_Cycles = 0;

      if (LED_Mode < 4)
      {
        LED_Mode++;
      }
      else
      {
        LED_Mode = 1;
      }
    }
  }

  /* Da tinh LED_Value bang bang trang thai.
     Gan mode = 0 de bo qua phep dich xoay cua code mau ben duoi.
     Giu nguyen code mau theo yeu cau chi bo sung. */
  mode = 0;

  if (mode == 1)
  {
    LED_Value = (uint8_t)((LED_Value >> 1) | (LED_Value << 7));
  }

  if (LED_Value & 0x80)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

  if (LED_Value & 0x40)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);

  if (LED_Value & 0x20)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

  if (LED_Value & 0x10)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

  if (LED_Value & 0x08)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);

  if (LED_Value & 0x04)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);

  if (LED_Value & 0x02)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);

  if (LED_Value & 0x01)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
}

/* Cau hinh bo sung cho phan 3.2. */
static void MX_7SegButton_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Tat hai module truoc khi cau hinh du lieu thanh. */
  HAL_GPIO_WritePin(GPIOG,
                   DIGIT_TENS_PIN | DIGIT_UNITS_PIN,
                   GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, SEGMENT_PINS, GPIO_PIN_RESET);

  /* PE8..PE15: cac thanh A..G va DP. */
  GPIO_InitStruct.Pin = SEGMENT_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* PG2, PG3: dieu khien transistor chon module. */
  GPIO_InitStruct.Pin = DIGIT_TENS_PIN | DIGIT_UNITS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* Doc B1 bang polling thay cho ngat cua project mau. */
  HAL_NVIC_DisableIRQ(EXTI0_IRQn);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* Dat gia tri hien thi trong khoang 00..99. */
void Set7SegDisplayValue(int value)
{
  if (value < 0)
  {
    value = 0;
  }

  SEG_DisplayValue = (uint8_t)(value % 100);
}

/* Goi lien tuc trong main loop.
   Moi 2 ms doi module duoc bat; khong dung HAL_Delay. */
void Run7SegDisplay(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t digit;
  uint16_t selectPin;

  if ((uint32_t)(now - SEG_LastScanTick) < SEG_SCAN_INTERVAL_MS)
  {
    return;
  }

  SEG_LastScanTick = now;

  /* Tat ca hai module truoc khi thay doi cac thanh. */
  HAL_GPIO_WritePin(GPIOG,
                   DIGIT_TENS_PIN | DIGIT_UNITS_PIN,
                   GPIO_PIN_RESET);

  if (SEG_NextDigit == 0)
  {
    digit = (uint8_t)(SEG_DisplayValue / 10);
    selectPin = DIGIT_TENS_PIN;
  }
  else
  {
    digit = (uint8_t)(SEG_DisplayValue % 10);
    selectPin = DIGIT_UNITS_PIN;
  }

  /* Xoa du lieu cu, sau do xuat ma thanh cho chu so moi.
     Bit DP luon bang 0. */
  HAL_GPIO_WritePin(GPIOE, SEGMENT_PINS, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE,
                   (uint16_t)((uint16_t)SEG_DigitCode[digit] << 8),
                   GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOG, selectPin, GPIO_PIN_SET);

  SEG_NextDigit ^= 1U;
}

/* Chong doi B1: trang thai phai on dinh 30 ms.
   Chi tang so khi chuyen tu nha sang bam. */
static void ProcessButtonB1(void)
{
  uint32_t now = HAL_GetTick();
  GPIO_PinState rawState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

  if (rawState != B1_LastRawState)
  {
    B1_LastRawState = rawState;
    B1_LastChangeTick = now;
  }

  if ((uint32_t)(now - B1_LastChangeTick) >= BUTTON_DEBOUNCE_MS)
  {
    if (B1_StableState != B1_LastRawState)
    {
      B1_StableState = B1_LastRawState;

      if (B1_StableState == GPIO_PIN_SET)
      {
        Set7SegDisplayValue((int)SEG_DisplayValue + 1);
      }
    }
  }
}

/* Chay dong thoi LED don, LED 7 thanh va nut B1. */
static void RunLabApplication(void)
{
  uint32_t now;

  ProcessButtonB1();
  Run7SegDisplay();

  now = HAL_GetTick();

  if ((uint32_t)(now - LED_LastStepTick) >= LED_STEP_DELAY_MS)
  {
    LED_LastStepTick = now;
    DisplayLEDs(LED_Mode);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  TIM6_Count = 0;
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
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  /* VD1 - Hello World: bat sang LED3 tai PG13. */
  /* Dung timer va ngat B1 cua project mau de giu LED sang lien tuc. */
  HAL_TIM_Base_Stop_IT(&htim6);
  HAL_NVIC_DisableIRQ(EXTI0_IRQn);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);

  /* Khoi tao phan tu lam. */
  LED_Mode = 1;
  LED_Step = 0;
  LED_Cycles = 0;

  /* Phan 3.2: ban dau hien thi 25. */
  Set7SegDisplayValue(25);
  SEG_NextDigit = 0;
  SEG_LastScanTick = HAL_GetTick();
  LED_LastStepTick = HAL_GetTick();

  B1_LastRawState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  B1_StableState = GPIO_PIN_RESET;
  B1_LastChangeTick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  LED_Value = 1;
  while (1)
  {
    /* Giu nguyen hai dong code mau, nhung khong chay dong thoi
       voi phan tu lam ben duoi. */
    if (0)
    {
    HAL_Delay(500);
    DisplayLEDs(1);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Phan 3.2: giu lai doan cu, khong dung delay chan quet LED. */
    if (0)
    {
    /* Chay lan luot mode 1 -> 2 -> 3 -> 4 -> 1.
       DisplayLEDs tu chuyen mode sau moi 2 chu ky. */
    DisplayLEDs(LED_Mode);
    HAL_Delay(LED_STEP_DELAY_MS);
    }

    RunLabApplication();
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 89;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PG13 PG14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Bo sung PE8..PE15, PG2, PG3 va cau hinh lai PA0 de doc B1. */
  MX_7SegButton_Init();
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
