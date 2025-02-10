/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usart.h"
#include "string.h"
#include "event_groups.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 内存池配置
#define POOL_SIZE 10  // 内存池中的块数量
#define BLOCK_SIZE 32 // 每个内存块的大小（字节）

// 内存测试配置
#define TEST_SMALL_SIZE 32   // 小块内存大小
#define TEST_MEDIUM_SIZE 128 // 中等内存大小
#define TEST_LARGE_SIZE 512  // 大块内存大小
#define TEST_COUNT 50        // 每种大小测试次数

// 添加中断事件标志位定义
#define INT_LED1_EVENT_BIT 0x04
#define INT_LED2_EVENT_BIT 0x08

// LED控制相关定义
#define LED_MODE_STATIC 0 // 静态模式：LED常亮/常灭
#define LED_MODE_BLINK 1  // 闪烁模式：LED闪烁
#define LED_MODE_BREATH 2 // 呼吸模式：LED渐变
#define LED_MODE_MAX 3    // 模式数量

// 中断事件标志位定义
#define EVT_TIMER_UPDATE 0x01 // 定时器更新事件
#define EVT_KEY_PRESS 0x02    // 按键按下事件
#define EVT_UART_CMD 0x04     // 串口命令事件

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern osThreadId_t LED1TaskHandle;
extern osThreadId_t LED2TaskHandle;
extern osThreadId_t KEY_TaskHandle;
extern osThreadId_t MonitorTaskHandle;
extern osThreadId_t MemMonitorTaskHandle;
extern osThreadId_t MemPoolTestTaskHandle;
extern osThreadId_t MemTestTaskHandle;
extern osThreadId_t IntLedTaskHandle;
extern osThreadId_t LEDControlTaskHandle;

extern osMessageQueueId_t KeyQueueHandle;
extern osSemaphoreId_t LED_SemHandle;
extern osEventFlagsId_t LED_EventHandle;
extern osTimerId_t Print_TimerHandle;
extern osTimerId_t LED_Breath_TimerHandle;

const osThreadAttr_t LED1Task_attributes = {
    .name = "LED1Task",
    .stack_size = 256,
    .priority = (osPriority_t)osPriorityNormal,
};

const osThreadAttr_t LED2Task_attributes = {
    .name = "LED2Task",
    .stack_size = 256,
    .priority = (osPriority_t)osPriorityNormal,
};

const osThreadAttr_t KEY_Task_attributes = {
    .name = "KEYTask",
    .stack_size = 128,
    .priority = (osPriority_t)osPriorityNormal,
};

const osThreadAttr_t MonitorTask_attributes = {
    .name = "MonitorTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityLow,
};

const osThreadAttr_t MemMonitor_attributes = {
    .name = "MemMonitorTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityLow,
};

osMessageQueueId_t KeyQueueHandle;
const osMessageQueueAttr_t KeyQueue_attributes = {
    .name = "KeyQueue"};

osSemaphoreId_t LED_SemHandle;
const osSemaphoreAttr_t LED_Sem_attributes = {
    .name = "LEDSem"};

osEventFlagsId_t LED_EventHandle;
const osEventFlagsAttr_t LED_Event_attributes = {
    .name = "LEDEvent"};

// 事件标志位定义
#define LED1_EVENT_BIT 0x01
#define LED2_EVENT_BIT 0x02

// 添加内存池相关变量
static uint8_t memPool[POOL_SIZE][BLOCK_SIZE]; // 内存池数组 占32*10=320字节
static uint8_t memPoolUsed[POOL_SIZE] = {0};   // 内存块使用状态 0-未使用 1-已使用
static osMutexId_t memPoolMutexHandle;         // 内存池互斥量
// 内存池互斥量属性
const osMutexAttr_t memPoolMutex_attributes = {
    .name = "MemPoolMutex"};

// 添加内存池测试任务句柄
osThreadId_t MemPoolTestTaskHandle;
// 内存池测试任务属性
const osThreadAttr_t MemPoolTest_attributes = {
    .name = "MemPoolTestTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityNormal,
};

// 添加内存测试任务句柄
osThreadId_t MemTestTaskHandle;

// 内存测试任务属性
const osThreadAttr_t MemTest_attributes = {
    .name = "MemTestTask",
    .stack_size = 1024, // 增加堆栈大小以适应测试
    .priority = (osPriority_t)osPriorityNormal,
};

// 添加中断控制任务句柄
osThreadId_t IntLedTaskHandle;

// 中断控制任务属性
const osThreadAttr_t IntLed_attributes = {
    .name = "IntLedTask",
    .stack_size = 256,
    .priority = (osPriority_t)osPriorityNormal,
};

// 添加LED控制相关变量
static uint8_t LED_Mode = LED_MODE_STATIC; // LED当前工作模式
static uint8_t LED_Brightness = 0;         // LED亮度（用于呼吸效果）
static uint8_t LED_BlinkCount = 0;         // LED闪烁计数
static uint8_t LED_BrightnessUp = 1;       // LED亮度增加/减少标志

// 添加LED控制任务句柄
osThreadId_t LEDControlTaskHandle;

// LED控制任务属性
const osThreadAttr_t LEDControl_attributes = {
    .name = "LEDControlTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityNormal,
};

// 添加软件定时器句柄
osTimerId_t Print_TimerHandle;
osTimerId_t LED_Breath_TimerHandle;
// 定时器属性定义
const osTimerAttr_t Print_Timer_attributes = {
    .name = "PrintTimer"};
const osTimerAttr_t LED_Breath_Timer_attributes = {
    .name = "LEDBreathTimer"};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void LED1_Task(void *argument);
extern void LED2_Task(void *argument);
extern void KEY_Task(void *argument);
extern void Monitor_Task(void *argument);
extern void MemMonitor_Task(void *argument);
extern void MemPoolTest_Task(void *argument);
extern void MemTest_Task(void *argument);
extern void IntLed_Task(void *argument);
extern void LEDControl_Task(void *argument);
extern void Print_Timer_Callback(void *argument);
extern void LED_Breath_Timer_Callback(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  // 初始化串口互斥量
  UART_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  memPoolMutexHandle = osMutexNew(&memPoolMutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  LED_SemHandle = osSemaphoreNew(1, 1, &LED_Sem_attributes);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  // 创建LED呼吸灯定时器（周期性，1000ms）
  LED_Breath_TimerHandle = osTimerNew(LED_Breath_Timer_Callback, osTimerPeriodic, NULL, &LED_Breath_Timer_attributes);
  if (LED_Breath_TimerHandle == NULL)
  {
    DEBUG_Print("LED Breath Timer creation failed!\r\n");
    Error_Handler();
  }

  // 创建状态打印定时器（周期性，1000ms）
  Print_TimerHandle = osTimerNew(Print_Timer_Callback, osTimerPeriodic, NULL, &Print_Timer_attributes);
  if (Print_TimerHandle == NULL)
  {
    DEBUG_Print("Print Timer creation failed!\r\n");
    Error_Handler();
  }

  // 启动定时器
  osTimerStart(Print_TimerHandle, 1000);       // 500ms周期
  osTimerStart(LED_Breath_TimerHandle, 1000); // 1000ms周期
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  KeyQueueHandle = osMessageQueueNew(4, sizeof(uint8_t), &KeyQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  LED1TaskHandle = osThreadNew(LED1_Task, NULL, &LED1Task_attributes);
  LED2TaskHandle = osThreadNew(LED2_Task, NULL, &LED2Task_attributes);
  KEY_TaskHandle = osThreadNew(KEY_Task, NULL, &KEY_Task_attributes);
  MonitorTaskHandle = osThreadNew(Monitor_Task, NULL, &MonitorTask_attributes);
  MemMonitorTaskHandle = osThreadNew(MemMonitor_Task, NULL, &MemMonitor_attributes);
  MemPoolTestTaskHandle = osThreadNew(MemPoolTest_Task, NULL, &MemPoolTest_attributes);
  MemTestTaskHandle = osThreadNew(MemTest_Task, NULL, &MemTest_attributes);
  IntLedTaskHandle = osThreadNew(IntLed_Task, NULL, &IntLed_attributes);
  LEDControlTaskHandle = osThreadNew(LEDControl_Task, NULL, &LEDControl_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  LED_EventHandle = osEventFlagsNew(&LED_Event_attributes);
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
