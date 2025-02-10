# FreeRTOS任务通信（CMSIS-RTOS V2版本）

## 一、任务通信概述

在实际应用中，任务之间经常需要进行数据交换和同步，FreeRTOS提供了多种任务间通信机制：

1. 队列（Queue）：用于任务间数据传输
2. 信号量（Semaphore）：用于任务同步和资源管理
3. 事件（Event）：用于任务间的事件通知
4. 任务通知（Task Notification）：轻量级的任务间通信机制

## 二、实践项目：多任务通信控制系统

我们将创建一个包含多个任务的项目，演示不同的通信机制：

### 1. 队列通信实验
- 按键检测任务：检测按键状态，发送到队列
- LED控制任务：从队列接收按键信息，控制LED

### 2. 信号量实验
- LED闪烁任务：通过二值信号量控制LED的开关
- 按键控制任务：通过按键释放或获取信号量

### 3. 事件标志组实验
- 多LED控制任务：根据不同事件控制不同LED
- 状态监控任务：监控并显示系统状态

### 硬件准备
- LED1: PC13（板载LED）
- LED2: PB12（外接LED）
- KEY1: PA0（按键输入）
- 串口：PA9(TX)、PA10(RX)

## 三、队列通信实现

### 1. 队列创建与配置
```c
/* 队列句柄定义 */
osMessageQueueId_t KeyQueueHandle;

/* 队列属性定义 */
const osMessageQueueAttr_t KeyQueue_attributes = {
    .name = "KeyQueue",      // 队列名称
    .attr_bits = 0,         // 属性位
    .cb_mem = NULL,         // 控制块内存（NULL表示动态分配）
    .cb_size = 0,          // 控制块大小
    .mq_mem = NULL,        // 消息队列内存（NULL表示动态分配）
    .mq_size = 0,         // 消息队列大小
};

/* 队列创建 */
// 参数1：队列长度为4
// 参数2：每个消息大小为1字节
// 参数3：队列属性
KeyQueueHandle = osMessageQueueNew(4, sizeof(uint8_t), &KeyQueue_attributes);
```

### 2. 队列API说明
- `osMessageQueueNew(count, size, attr)`
  - count: 队列中可以存储的最大消息数
  - size: 每条消息的大小（字节）
  - attr: 队列属性
  - 返回值: 队列句柄

- `osMessageQueuePut(queue_id, msg_ptr, priority, timeout)`
  - queue_id: 队列句柄
  - msg_ptr: 消息指针
  - priority: 消息优先级（0表示正常）
  - timeout: 超时时间（0表示不等待，osWaitForever表示永久等待）

- `osMessageQueueGet(queue_id, msg_ptr, priority, timeout)`
  - queue_id: 队列句柄
  - msg_ptr: 接收消息的缓冲区指针
  - priority: 接收到的消息优先级
  - timeout: 超时时间

## 四、信号量通信实现

### 1. 二值信号量示例
```c
/* 信号量句柄定义 */
osSemaphoreId_t LED_SemHandle;

/* 信号量属性定义 */
const osSemaphoreAttr_t LED_Sem_attributes = {
    .name = "LEDSem",
    .attr_bits = 0,
    .cb_mem = NULL,
    .cb_size = 0,
};

/* 创建二值信号量 */
LED_SemHandle = osSemaphoreNew(1, 1, &LED_Sem_attributes);

/* LED任务 */
void LED_Task(void *argument)
{
    for(;;)
    {
        // 获取信号量
        if(osSemaphoreAcquire(LED_SemHandle, osWaitForever) == osOK)
        {
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            osDelay(500);  // LED亮500ms
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            
            // 释放信号量
            osSemaphoreRelease(LED_SemHandle);
            osDelay(500);  // 等待500ms后再次尝试获取信号量
        }
    }
}

/* 按键控制任务 */
void KEY_Control_Task(void *argument)
{
    uint8_t key_state, last_state = 1;
    
    for(;;)
    {
        key_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
        if(key_state == 0 && last_state == 1)  // 按键按下
        {
            // 释放信号量，允许LED任务运行
            osSemaphoreRelease(LED_SemHandle);
        }
        last_state = key_state;
        osDelay(20);
    }
}
```

### 2. 信号量API说明
- `osSemaphoreNew(max_count, initial_count, attr)`
  - max_count: 最大计数值（二值信号量为1）
  - initial_count: 初始计数值
  - attr: 信号量属性
  - 返回值: 信号量句柄

- `osSemaphoreAcquire(semaphore_id, timeout)`
  - semaphore_id: 信号量句柄
  - timeout: 超时时间
  - 返回值: osOK表示成功

- `osSemaphoreRelease(semaphore_id)`
  - semaphore_id: 信号量句柄
  - 返回值: osOK表示成功

## 五、事件标志组实现

### 1. 事件标志组示例
```c
/* 事件标志组句柄定义 */
osEventFlagsId_t LED_EventHandle;

/* 事件标志定义 */
#define LED1_EVENT_BIT    0x01
#define LED2_EVENT_BIT    0x02

/* 事件标志组属性定义 */
const osEventFlagsAttr_t LED_Event_attributes = {
    .name = "LEDEvent",
    .attr_bits = 0,
    .cb_mem = NULL,
    .cb_size = 0,
};

/* 创建事件标志组 */
LED_EventHandle = osEventFlagsNew(&LED_Event_attributes);

/* LED控制任务 */
void LED_Control_Task(void *argument)
{
    uint32_t flags;
    
    for(;;)
    {
        // 等待任意一个LED事件
        flags = osEventFlagsWait(LED_EventHandle, 
                                LED1_EVENT_BIT | LED2_EVENT_BIT,  // 等待的事件
                                osFlagsWaitAny,                   // 任意一个事件即可
                                osWaitForever);                   // 永久等待
        
        if(flags & LED1_EVENT_BIT)
        {
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        }
        if(flags & LED2_EVENT_BIT)
        {
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
        }
    }
}

/* 按键控制任务 */
void KEY_Task(void *argument)
{
    uint8_t key_state, last_state = 1;
    static uint8_t led_select = 0;
    
    for(;;)
    {
        key_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
        if(key_state == 0 && last_state == 1)  // 按键按下
        {
            led_select = !led_select;  // 切换LED选择
            // 设置相应的事件标志
            osEventFlagsSet(LED_EventHandle, led_select ? LED1_EVENT_BIT : LED2_EVENT_BIT);
        }
        last_state = key_state;
        osDelay(20);
    }
}
```

### 2. 事件标志组API说明
- `osEventFlagsNew(attr)`
  - attr: 事件标志组属性
  - 返回值: 事件标志组句柄

- `osEventFlagsSet(ef_id, flags)`
  - ef_id: 事件标志组句柄
  - flags: 要设置的标志位
  - 返回值: 设置后的标志值

- `osEventFlagsWait(ef_id, flags, options, timeout)`
  - ef_id: 事件标志组句柄
  - flags: 等待的标志位
  - options: 等待选项（osFlagsWaitAll/osFlagsWaitAny）
  - timeout: 超时时间
  - 返回值: 触发的事件标志

## 六、通信机制的选择建议

### 1. 队列
- 适用场景：
  - 需要传输实际数据
  - 多个生产者/消费者
  - 需要数据缓冲
- 特点：
  - 支持多字节数据传输
  - 具有缓冲功能
  - 支持多个任务等待

### 2. 信号量
- 适用场景：
  - 任务同步
  - 资源访问控制
  - 简单的事件通知
- 特点：
  - 计数功能
  - 支持超时机制
  - 适合共享资源保护

### 3. 事件标志组
- 适用场景：
  - 多事件触发
  - 条件组合
  - 多任务协同
- 特点：
  - 支持多个事件位
  - 可等待多个事件组合
  - 无数据传输能力

### 4. 实际应用建议
1. 数据传输优先使用队列
2. 资源保护使用信号量
3. 多条件触发使用事件标志组
4. 简单通知可以使用任务通知
5. 注意避免优先级反转问题

代码示例：
```c
// 任务句柄定义
osThreadId_t LED1TaskHandle;
osThreadId_t LED2TaskHandle;
osThreadId_t KEY_TaskHandle;
osThreadId_t MonitorTaskHandle;

// 任务属性定义
const osThreadAttr_t LED1Task_attributes = {
    .name = "LED1Task",
    .stack_size = 128,
    .priority = (osPriority_t)osPriorityNormal,
};

const osThreadAttr_t LED2Task_attributes = {
    .name = "LED2Task",
    .stack_size = 128,
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

// 队列
osMessageQueueId_t KeyQueueHandle;
const osMessageQueueAttr_t KeyQueue_attributes = {
    .name = "KeyQueue"
};

// 信号量
osSemaphoreId_t LED_SemHandle;
const osSemaphoreAttr_t LED_Sem_attributes = {
    .name = "LEDSem"
};

// 事件标志组
osEventFlagsId_t LED_EventHandle;
const osEventFlagsAttr_t LED_Event_attributes = {
    .name = "LEDEvent"
};

// 事件标志位定义
#define LED1_EVENT_BIT    0x01
#define LED2_EVENT_BIT    0x02

// 任务函数声明
void LED1_Task(void *argument);
void LED2_Task(void *argument);
void KEY_Task(void *argument);
void Monitor_Task(void *argument);

//MX_FREERTOS_Init中写
/* USER CODE BEGIN RTOS_SEMAPHORES */
//信号量创建 
LED_SemHandle = osSemaphoreNew(1, 1, &LED_Sem_attributes);
/* USER CODE END RTOS_SEMAPHORES */

/* USER CODE BEGIN RTOS_QUEUES */
//队列创建
KeyQueueHandle = osMessageQueueNew(4, sizeof(uint8_t), &KeyQueue_attributes);
/* USER CODE END RTOS_QUEUES */

/* USER CODE BEGIN RTOS_THREADS */
LED1TaskHandle = osThreadNew(LED1_Task, NULL, &LED1Task_attributes);
LED2TaskHandle = osThreadNew(LED2_Task, NULL, &LED2Task_attributes);
KEY_TaskHandle = osThreadNew(KEY_Task, NULL, &KEY_Task_attributes);
MonitorTaskHandle = osThreadNew(Monitor_Task, NULL, &MonitorTask_attributes);
if(MonitorTaskHandle == NULL)
{
  DEBUG_Print("MonitorTask creation failed!\r\n");
  Error_Handler();
}
/* USER CODE END RTOS_THREADS */

/* USER CODE BEGIN RTOS_EVENTS */
//事件标志组创建
LED_EventHandle = osEventFlagsNew(&LED_Event_attributes);
/* USER CODE END RTOS_EVENTS */

/* USER CODE BEGIN Application */
void LED1_Task(void *argument)
{
  for(;;)
  {
    if(osSemaphoreAcquire(LED_SemHandle, osWaitForever) == osOK)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
  }
}

void LED2_Task(void *argument)
{
  uint32_t flags;
  
  for(;;)
  {
    // 等待LED2的事件标志
    flags = osEventFlagsWait(LED_EventHandle, 
                            LED2_EVENT_BIT,    // 只等待LED2事件
                            osFlagsWaitAny,    // 任意一个事件即可
                            osWaitForever);    // 永久等待
    
    if((flags & LED2_EVENT_BIT) == LED2_EVENT_BIT)
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
  
  for(;;)
  {
    key_state = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    
    // 检测到按键按下（下降沿）
    if(key_state == 0 && key_last_state == 1)
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
    osDelay(20);  // 按键消抖延时
  }
}

/* 监控任务实现 */
void Monitor_Task(void *argument)
{
  osDelay(1000);  // 等待其他任务初始化完成

  for(;;)
  {
    // 获取任务状态的文字描述
    const char *led1_state_str, *led2_state_str, *key_state_str;
    
    // LED1任务状态
    switch(osThreadGetState(LED1TaskHandle))
    {
      case osThreadReady: led1_state_str = "Ready"; break;
      case osThreadRunning: led1_state_str = "Running"; break;
      case osThreadBlocked: led1_state_str = "Blocked"; break;
      case osThreadTerminated: led1_state_str = "Terminated"; break;
      default: led1_state_str = "Unknown"; break;
    }
    
    // LED2任务状态
    switch(osThreadGetState(LED2TaskHandle))
    {
      case osThreadReady: led2_state_str = "Ready"; break;
      case osThreadRunning: led2_state_str = "Running"; break;
      case osThreadBlocked: led2_state_str = "Blocked"; break;
      case osThreadTerminated: led2_state_str = "Terminated"; break;
      default: led2_state_str = "Unknown"; break;
    }
    
    // 按键任务状态
    switch(osThreadGetState(KEY_TaskHandle))
    {
      case osThreadReady: key_state_str = "Ready"; break;
      case osThreadRunning: key_state_str = "Running"; break;
      case osThreadBlocked: key_state_str = "Blocked"; break;
      case osThreadTerminated: key_state_str = "Terminated"; break;
      default: key_state_str = "Unknown"; break;
    }

    DEBUG_Print("\r\nSystemStatus\r\n");
    DEBUG_Print("LED1 Task: "); DEBUG_Print(led1_state_str); DEBUG_Print("\r\n");
    DEBUG_Print("LED2 Task: "); DEBUG_Print(led2_state_str); DEBUG_Print("\r\n");
    DEBUG_Print("KEY Task: "); DEBUG_Print(key_state_str); DEBUG_Print("\r\n");
    DEBUG_Print("=\r\n");
           
    osDelay(2000);  // 增加状态打印的间隔时间
  }
}
/* USER CODE END Application */
```