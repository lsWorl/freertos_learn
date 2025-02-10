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
#define POOL_SIZE       10    // 内存池中的块数量
#define BLOCK_SIZE      32    // 每个内存块的大小（字节）

// 内存测试配置
#define TEST_SMALL_SIZE   32    // 小块内存大小
#define TEST_MEDIUM_SIZE  128   // 中等内存大小
#define TEST_LARGE_SIZE   512   // 大块内存大小
#define TEST_COUNT        50    // 每种大小测试次数

// 添加中断事件标志位定义
#define INT_LED1_EVENT_BIT  0x04
#define INT_LED2_EVENT_BIT  0x08

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osThreadId_t LED1TaskHandle;
osThreadId_t LED2TaskHandle;
osThreadId_t KEY_TaskHandle;
osThreadId_t MonitorTaskHandle;
osThreadId_t MemMonitorTaskHandle;

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
static uint8_t memPool[POOL_SIZE][BLOCK_SIZE];  // 内存池数组 占32*10=320字节
static uint8_t memPoolUsed[POOL_SIZE] = {0};    // 内存块使用状态 0-未使用 1-已使用
static osMutexId_t memPoolMutexHandle;          // 内存池互斥量
// 内存池互斥量属性
const osMutexAttr_t memPoolMutex_attributes = {
    .name = "MemPoolMutex"
};

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
    .stack_size = 1024,  // 增加堆栈大小以适应测试
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
void LED1_Task(void *argument);
void LED2_Task(void *argument);
void KEY_Task(void *argument);
void Monitor_Task(void *argument);
void MemMonitor_Task(void *argument);

// 添加内存池相关函数声明
void* poolMalloc(void);
void poolFree(void* ptr);
void MemPoolTest_Task(void *argument);

// 添加内存测试任务声明
void MemTest_Task(void *argument);

// 添加中断控制任务声明
void IntLed_Task(void *argument);

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
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  KeyQueueHandle = osMessageQueueNew(4, sizeof(uint8_t), &KeyQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  // LED1TaskHandle = osThreadNew(LED1_Task, NULL, &LED1Task_attributes);
  // LED2TaskHandle = osThreadNew(LED2_Task, NULL, &LED2Task_attributes);
  // KEY_TaskHandle = osThreadNew(KEY_Task, NULL, &KEY_Task_attributes);
  // MonitorTaskHandle = osThreadNew(Monitor_Task, NULL, &MonitorTask_attributes);
  // MemMonitorTaskHandle = osThreadNew(MemMonitor_Task, NULL, &MemMonitor_attributes);
  // if (MemMonitorTaskHandle == NULL)
  // {
  //   DEBUG_Print("MemMonitorTask creation failed!\r\n");
  //   Error_Handler();
  // }
  // MemPoolTestTaskHandle = osThreadNew(MemPoolTest_Task, NULL, &MemPoolTest_attributes);
  // if(MemPoolTestTaskHandle == NULL)
  // {
  //   DEBUG_Print("MemPoolTestTask creation failed!\r\n");
  //   Error_Handler();
  // }

  // 创建内存测试任务
  // MemTestTaskHandle = osThreadNew(MemTest_Task, NULL, &MemTest_attributes);
  // if(MemTestTaskHandle == NULL)
  // {
  //   DEBUG_Print("MemTestTask creation failed!\r\n");
  //   Error_Handler();
  // }

  // 创建中断控制任务
  IntLedTaskHandle = osThreadNew(IntLed_Task, NULL, &IntLed_attributes);
  if(IntLedTaskHandle == NULL)
  {
    DEBUG_Print("IntLedTask creation failed!\r\n");
    Error_Handler();
  }
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
void LED1_Task(void *argument)
{
  for (;;)
  {
    if (osSemaphoreAcquire(LED_SemHandle, osWaitForever) == osOK)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
  }
}

void LED2_Task(void *argument)
{
  uint32_t flags;

  for (;;)
  {
    // 等待LED2的事件标志
    flags = osEventFlagsWait(LED_EventHandle,
                             LED2_EVENT_BIT, // 只等待LED2事件
                             osFlagsWaitAny, // 任意一个事件即可
                             osWaitForever); // 永久等待
    if ((flags & LED2_EVENT_BIT) == LED2_EVENT_BIT)
    {
      DEBUG_Print("LED2 event received!\r\n");
      // 收到事件后，LED2翻转
      HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }
  }
}

/* 按键任务实现 - 同时使用队列、信号量和事件标志 */
void KEY_Task(void *argument)
{
  uint8_t key_state;
  uint8_t key_last_state = 1;

  for (;;)
  {
    key_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);

    // 检测到按键按下（下降沿）
    if (key_state == 0 && key_last_state == 1)
    {
      DEBUG_Print("Key Pressed!\r\n");

      // 1. 发送按键消息到队列
      osMessageQueuePut(KeyQueueHandle, &key_state, 0, 0);

      // 2. 释放信号量，触发LED1闪烁
      osSemaphoreRelease(LED_SemHandle);

      // 3. 设置事件标志，触发LED2翻转
      osEventFlagsSet(LED_EventHandle, LED2_EVENT_BIT);
    }

    key_last_state = key_state;
    osDelay(20); // 按键消抖延时
  }
}

/* 监控任务实现 */
void Monitor_Task(void *argument)
{
  osDelay(1000); // 等待其他任务初始化完成

  for (;;)
  {
    // 获取任务状态的文字描述
    const char *led1_state_str, *led2_state_str, *key_state_str;

    // LED1任务状态
    switch (osThreadGetState(LED1TaskHandle))
    {
    case osThreadReady:
      led1_state_str = "Ready";
      break;
    case osThreadRunning:
      led1_state_str = "Running";
      break;
    case osThreadBlocked:
      led1_state_str = "Blocked";
      break;
    case osThreadTerminated:
      led1_state_str = "Terminated";
      break;
    default:
      led1_state_str = "Unknown";
      break;
    }

    // LED2任务状态
    switch (osThreadGetState(LED2TaskHandle))
    {
    case osThreadReady:
      led2_state_str = "Ready";
      break;
    case osThreadRunning:
      led2_state_str = "Running";
      break;
    case osThreadBlocked:
      led2_state_str = "Blocked";
      break;
    case osThreadTerminated:
      led2_state_str = "Terminated";
      break;
    default:
      led2_state_str = "Unknown";
      break;
    }

    // 按键任务状态
    switch (osThreadGetState(KEY_TaskHandle))
    {
    case osThreadReady:
      key_state_str = "Ready";
      break;
    case osThreadRunning:
      key_state_str = "Running";
      break;
    case osThreadBlocked:
      key_state_str = "Blocked";
      break;
    case osThreadTerminated:
      key_state_str = "Terminated";
      break;
    default:
      key_state_str = "Unknown";
      break;
    }

    DEBUG_Print("\r\nSystemStatus\r\n");
    DEBUG_Print("LED1 Task: ");
    DEBUG_Print(led1_state_str);
    DEBUG_Print("\r\n");
    DEBUG_Print("LED2 Task: ");
    DEBUG_Print(led2_state_str);
    DEBUG_Print("\r\n");
    DEBUG_Print("KEY Task: ");
    DEBUG_Print(key_state_str);
    DEBUG_Print("\r\n");
    DEBUG_Print("=\r\n");

    osDelay(2000); // 增加状态打印的间隔时间
  }
}

/* 内存监控任务实现 */
void MemMonitor_Task(void *argument)
{
  char buf[256];

  // 等待其他任务初始化完成
  osDelay(1000);

  for (;;)
  {
    // 获取当前空闲堆空间
    size_t freeHeap = xPortGetFreeHeapSize();

    // 获取历史最小空闲堆空间
    size_t minHeap = xPortGetMinimumEverFreeHeapSize();

    // 计算已使用的堆空间
    size_t totalHeap = configTOTAL_HEAP_SIZE;
    size_t usedHeap = totalHeap - freeHeap;

    // 格式化并输出内存使用信息
    snprintf(buf, sizeof(buf),
             "\r\n=== Memory Status ===\r\n"
             "Total Heap: %d bytes\r\n"
             "Free Heap: %d bytes\r\n"
             "Used Heap: %d bytes\r\n"
             "Min Ever Free: %d bytes\r\n"
             "Memory Usage: %d%%\r\n"
             "==================\r\n",
             totalHeap,
             freeHeap,
             usedHeap,
             minHeap,
             (usedHeap * 100) / totalHeap);

    DEBUG_Print(buf);

    // 每2秒更新一次内存信息
    osDelay(2000);
  }
}

/* 内存池分配函数 */
void* poolMalloc(void)
{
    void* ptr = NULL;
    
    // 获取互斥量
    if(osMutexAcquire(memPoolMutexHandle, osWaitForever) == osOK)
    {
        // 查找空闲内存块
        for(int i = 0; i < POOL_SIZE; i++)
        {
            if(memPoolUsed[i] == 0)
            {
                memPoolUsed[i] = 1;
                ptr = memPool[i];
                DEBUG_Print("Memory block allocated: ");
                DEBUG_PrintNum("Block", i);
                break;
            }
        }
        
        // 释放互斥量
        osMutexRelease(memPoolMutexHandle);
    }
    
    return ptr;
}

/* 内存池释放函数 */
void poolFree(void* ptr)
{
    // 获取互斥量
    if(osMutexAcquire(memPoolMutexHandle, osWaitForever) == osOK)
    {
        // 查找要释放的内存块
        for(int i = 0; i < POOL_SIZE; i++)
        {
            if(memPool[i] == ptr)
            {
                memPoolUsed[i] = 0;
                DEBUG_Print("Memory block freed: ");
                DEBUG_PrintNum("Block", i);
                break;
            }
        }
        
        // 释放互斥量
        osMutexRelease(memPoolMutexHandle);
    }
}

/* 内存池测试任务 */
void MemPoolTest_Task(void *argument)
{
    void* ptr_array[5] = {NULL};  // 用于存储分配的内存指针
    
    // 等待其他任务初始化完成
    osDelay(1000);
    
    for(;;)
    {
        DEBUG_Print("\r\n=== Memory Pool Test ===\r\n");
        
        // 分配测试
        DEBUG_Print("Allocating memory blocks...\r\n");
        for(int i = 0; i < 5; i++)
        {
            ptr_array[i] = poolMalloc();
            if(ptr_array[i] != NULL)
            {
                // 使用内存块，写入测试数据
                uint8_t* data = (uint8_t*)ptr_array[i];
                for(int j = 0; j < BLOCK_SIZE; j++)
                {
                    data[j] = i + 1;  // 填充测试数据
                }
            }
            osDelay(200);  // 延时以便观察
        }
        
        // 释放测试
        DEBUG_Print("\r\nFreeing memory blocks...\r\n");
        for(int i = 0; i < 5; i++)
        {
            if(ptr_array[i] != NULL)
            {
                poolFree(ptr_array[i]);
                ptr_array[i] = NULL;
            }
            osDelay(200);  // 延时以便观察
        }
        
        DEBUG_Print("\r\n=== Test Complete ===\r\n");
        osDelay(5000);  // 等待5秒后重新开始测试
    }
}

/* 内存分配性能测试任务 */
void MemTest_Task(void *argument)
{
    uint32_t start_time, end_time;
    void *ptr_array[TEST_COUNT];
    char buf[128];
    
    // 等待其他任务初始化完成
    osDelay(1000);
    
    for(;;)
    {
        DEBUG_Print("\r\n=== Memory Allocation Performance Test ===\r\n");
        
        // 1. 测试小块内存分配/释放性能
        DEBUG_Print("\r\nTesting small block allocation...\r\n");
        start_time = osKernelGetTickCount();
        
        // 分配测试
        for(int i = 0; i < TEST_COUNT; i++)
        {
            ptr_array[i] = pvPortMalloc(TEST_SMALL_SIZE);
            if(ptr_array[i] != NULL)
            {
                // 写入一些测试数据
                memset(ptr_array[i], 0xAA, TEST_SMALL_SIZE);
            }
        }
        
        // 释放测试
        for(int i = 0; i < TEST_COUNT; i++)
        {
            if(ptr_array[i] != NULL)
            {
                vPortFree(ptr_array[i]);
            }
        }
        
        end_time = osKernelGetTickCount();
        snprintf(buf, sizeof(buf), "Small blocks (%d bytes) test time: %ldms\r\n", 
                TEST_SMALL_SIZE, end_time - start_time);
        DEBUG_Print(buf);
        
        // 2. 测试中等大小内存块分配/释放性能
        DEBUG_Print("\r\nTesting medium block allocation...\r\n");
        start_time = osKernelGetTickCount();
        
        for(int i = 0; i < TEST_COUNT; i++)
        {
            ptr_array[i] = pvPortMalloc(TEST_MEDIUM_SIZE);
            if(ptr_array[i] != NULL)
            {
                memset(ptr_array[i], 0xBB, TEST_MEDIUM_SIZE);
            }
        }
        
        for(int i = 0; i < TEST_COUNT; i++)
        {
            if(ptr_array[i] != NULL)
            {
                vPortFree(ptr_array[i]);
            }
        }
        
        end_time = osKernelGetTickCount();
        snprintf(buf, sizeof(buf), "Medium blocks (%d bytes) test time: %ldms\r\n", 
                TEST_MEDIUM_SIZE, end_time - start_time);
        DEBUG_Print(buf);
        
        // 3. 测试大块内存分配/释放性能
        DEBUG_Print("\r\nTesting large block allocation...\r\n");
        start_time = osKernelGetTickCount();
        
        for(int i = 0; i < TEST_COUNT; i++)
        {
            ptr_array[i] = pvPortMalloc(TEST_LARGE_SIZE);
            if(ptr_array[i] != NULL)
            {
                memset(ptr_array[i], 0xCC, TEST_LARGE_SIZE);
            }
        }
        
        for(int i = 0; i < TEST_COUNT; i++)
        {
            if(ptr_array[i] != NULL)
            {
                vPortFree(ptr_array[i]);
            }
        }
        
        end_time = osKernelGetTickCount();
        snprintf(buf, sizeof(buf), "Large blocks (%d bytes) test time: %ldms\r\n", 
                TEST_LARGE_SIZE, end_time - start_time);
        DEBUG_Print(buf);
        
        // 输出当前内存状态
        size_t freeHeap = xPortGetFreeHeapSize();
        size_t minHeap = xPortGetMinimumEverFreeHeapSize();
        
        snprintf(buf, sizeof(buf), 
                "\r\nMemory Status:\r\n"
                "Current free heap: %d bytes\r\n"
                "Minimum ever free heap: %d bytes\r\n"
                "=================================\r\n",
                freeHeap, minHeap);
        DEBUG_Print(buf);
        
        // 等待10秒后重新开始测试
        osDelay(10000);
    }
}

/* 中断回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == KEY1_Pin)
    {
        // 从中断发送事件标志
        xEventGroupSetBitsFromISR(
            LED_EventHandle,
            INT_LED1_EVENT_BIT | INT_LED2_EVENT_BIT,
            &xHigherPriorityTaskWoken
        );
        
        DEBUG_Print("Interrupt Triggered!\r\n");
        
        // 如果需要任务切换，触发PendSV中断
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* 中断控制LED任务 */
void IntLed_Task(void *argument)
{
    EventBits_t uxBits;
    
    for(;;)
    {
        // 等待中断触发的事件
        uxBits = xEventGroupWaitBits(
            LED_EventHandle,
            INT_LED1_EVENT_BIT | INT_LED2_EVENT_BIT,
            pdTRUE,    // 清除事件标志
            pdFALSE,   // 任一事件满足即可
            portMAX_DELAY
        );
        
        // 处理事件
        if((uxBits & INT_LED1_EVENT_BIT) != 0)
        {
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            DEBUG_Print("LED1 Toggled by Interrupt\r\n");
        }
        if((uxBits & INT_LED2_EVENT_BIT) != 0)
        {
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
            DEBUG_Print("LED2 Toggled by Interrupt\r\n");
        }
    }
}
/* USER CODE END Application */
