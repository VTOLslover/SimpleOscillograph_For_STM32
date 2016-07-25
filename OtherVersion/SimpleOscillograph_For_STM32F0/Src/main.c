/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "mydef.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void scan(void);
void send(void);
void on_send_finish(void);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

//interval_time ����ʱ������1/FS
volatile u32 FS         = 100000u;  //��ʼ��ʾ100KHz
//abort flag
volatile u8 is_abort    = 0;        //1���������η���
// scan mode
volatile u8 scan_mode   = 0;        //0��ʵʱ����������ģʽ�� 1���ɼ���ͬʱ�������� �ʺϵ�Ƶ�ź�
volatile u16 value_i    = 0;        //��¼���ڲɼ������εڼ�����  ��Χ��0��(number_of_data-1)
volatile u8 SCAN_IS_OK  = 0;        //����Ƿ�Ϊ�ɼ����
// TriggerMode
volatile u8 trigger_mode= 0;        //0����������Զ����� 1 2 3�ֱ�Ϊ���� �½� ���ش���
volatile EXTITrigger_TypeDef TriggerMode;  //���ڼ�¼�ж�ģʽ(����ģʽ)


// ������ �Ӵ�����ʼ���Բ���Ƶ��FS�洢num_of_data���㣬֮��һ����������
#if USE_CHANNEL_C1
volatile u16 C1_value[num_of_data];
#endif
#if USE_CHANNEL_C2
volatile u16 C2_value[num_of_data];
#endif
#if USE_CHANNEL_C3
volatile u16 C3_value[num_of_data];
#endif

// ADCת���ĵ�ѹֵͨ��MDA��ʽ����SRAM
volatile uint16_t ADC_ConvertedValue[3];

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
  u8 no_use;

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC_Init();
  MX_TIM1_Init();

  /* USER CODE BEGIN 2 */
    SystemCoreClockUpdate();
    printf("SystemCoreClock = %d\r\n", SystemCoreClock);
    
    /* �ȹر�DMA�ж� �����ǳ��ǳ��˷���Դ �������� CubeMXû�н��õ�ѡ������Ϊ����ֲ��������ر� */
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    
    /* enable adc1 and config adc1 to dma mode */
    HAL_ADC_Start_DMA(&hadc, (uint32_t*)ADC_ConvertedValue, 3);
    
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)&no_use, 1);
    
    /* LED */
    LED(ON);

    /* TIM ��ʱ���� */
    TIM_ConfigFrequency(FS);  //��ʼFS(100KHz)
    
    //��ʼ���رմ����ж�
    EXTI_DISABLE;

    PRINTF_LABVIEW("\r\n------ SimpleOscillograph For STM32 ------\r\n");

    //����ADC��Ϣ������λ������adԭʼֵ
    PRINTF_LABVIEW("VC:%f\r\n", VCC_V);  //VCC
    PRINTF_LABVIEW("AD:%d\r\n", ADC_B);  //adcλ��

    //��ʼһ�βɼ������ڵ���
    START_NEW_SCAN;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
        if (scan_mode == 1)
        {
            //ʵʱ����ʵʱ��ʾ
            PRINTF_LABVIEW("C1:%d\r\nC2:%d\r\nC3:%d\r\n", ADC_ConvertedValue[0], ADC_ConvertedValue[1], ADC_ConvertedValue[2]);
        }
        if(SCAN_IS_OK == 1)
        {
            SCAN_IS_OK = 0;
            send();
        }
    }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 3, 0);
}

/* USER CODE BEGIN 4 */
/*
 * ���ݷ��ͺ���
 * ֱ�ӷ���ԭʼֵ��������λ������
 * ������Ϻ������ⲿ�ж��������ٴδ���
 */
void send(void)
{
    u16 i = 0;

    //����ADC��Ϣ������λ������adԭʼֵ
    PRINTF_LABVIEW("VC:%f\r\n", VCC_V);       //VCC
    PRINTF_LABVIEW("AD:%d\r\n", ADC_B);       //adcλ��
    PRINTF_LABVIEW("FS:%d\r\n", FS);          //������
    PRINTF_LABVIEW("NM:%d\r\n", num_of_data); //���ݸ���(���ڻ�ͼ��

    is_abort = 0;

    do
    {
#if USE_CHANNEL_C1
        PRINTF_LABVIEW("C1:%d\r\n", C1_value[i]);
#endif

#if USE_CHANNEL_C2
        PRINTF_LABVIEW("C2:%d\r\n", C2_value[i]);
#endif

#if USE_CHANNEL_C3
        PRINTF_LABVIEW("C3:%d\r\n", C3_value[i]);
#endif
    }
    while((++i < num_of_data) && (is_abort == 0) && (scan_mode == 0));

    PRINTF_LABVIEW("FN:OK\r\n");    //Ҳ������η���ΪADC����ת����ʱ���ȶ�
    on_send_finish();
}


//���������Ҫ����һЩ����������
void on_send_finish(void)
{
    LED(OFF);
    if(scan_mode == 0)
    {
        if(trigger_mode == 0)
        {
            //�Զ���ʼ�µĲɼ�
            START_NEW_SCAN;
        }
        else if(trigger_mode == 4)
        {
            EXTI_DISABLE;
        }
        else
        {
            EXTI_ENABLE;
        }
    }
}

/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/