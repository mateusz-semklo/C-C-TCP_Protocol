/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "lwip.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stm32_tcp_echoserver.h"
#include "ethernet.h"
#include "cJSON.h"
#include "cJSON_Utils.h"
#include "stdint.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#define SYSCLK_FREQ      160000000uL

////////// TIMER 8 /////////////////////////////////////////////////////////////
#define TIM8_ARR  (65500-1)
#define TIM8_PSC  (10-1)
#define freq_TIM8 (SYSCLK_FREQ/(TIM8_PSC+1))  //obr/min mechaniczne
#define revolution_per_min ((freq_TIM8*60.0)/(720.0))
////////// TIMER 8 /////////////////////////////////////////////////////////////

////////// TIMER 3 encoder /////////////////////////////////////////////////////////////
#define TIM3_ARR  719
#define TIM3_PSC  0
////////// TIMER 3 /////////////////////////////////////////////////////////////

////////// TIMER 1 PWM input /////////////////////////////////////////////////////////////
#define TIM1_ARR  0xFFFF
#define TIM1_PSC  800
////////// TIMER 1 PWM input /////////////////////////////////////////////////////////////


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
int32_t adc_Ia,adc_Ib,adc_Ic,count,adc_off_Ia,adc_off_Ib,adc_off_Ic;
float Ia,Ib,Ic;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin==GPIO_PIN_13)
	{


	}

}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	 adc_Ia= HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
   //while((hadc1.Instance->SR &= (0x1<<5))!=0){}
    adc_Ib =HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
	//while((hadc1.Instance->SR &= (0x1<<5))!=0){}
    adc_Ic =HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_3);

    count++;
    if(count<5)
    {

    }
    else if(count==5)
    {
   	 adc_off_Ia=adc_Ia;
   	 adc_off_Ib=adc_Ib;
   	 adc_off_Ic=adc_Ic;

    }
    else
    {
    count=10;
    Ia=-(adc_Ia-adc_off_Ia)* 0.004355;
    Ib=-(adc_Ib-adc_off_Ib)* 0.004355;
    Ic=-(adc_Ic-adc_off_Ic)* 0.004355;
    }


  HAL_ADCEx_InjectedStart_IT(&hadc1);

}




/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_LWIP_Init();
  MX_TIM3_Init();
  MX_TIM8_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM9_Init();
  /* USER CODE BEGIN 2 */
  tcp_echoserver_init();

 TIM3->ARR=TIM3_ARR;
 TIM3->PSC=TIM3_PSC;
 HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_1);
 HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_2);

 TIM8->PSC=TIM8_PSC;
 TIM8->ARR=TIM8_ARR;
 HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_1);

 TIM1->PSC=TIM1_PSC;
 TIM1->ARR=TIM1_ARR;
 HAL_TIM_Base_Start(&htim1);
 HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_1);


 HAL_ADCEx_InjectedStart_IT(&hadc1);




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  ethernetif_input(&gnetif);
	      sys_check_timeouts();

	  	capture_tim8_ccr1= TIM8->CCR1;
	  	capture_tim3_ccr1= TIM3->CCR1;


	  	if(capture_tim8_ccr1 <= 50)
	  		speed=0;
	  	else
	  		speed=revolution_per_min/capture_tim8_ccr1;




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
