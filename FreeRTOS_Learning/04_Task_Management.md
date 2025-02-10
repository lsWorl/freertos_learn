# FreeRTOS任务管理 (CMSIS-RTOS V2版本)

## 一、任务的基本概念

### 1. 什么是任务
- 任务是一个独立的程序执行单元
- 每个任务都有自己的：
  - 栈空间
  - 优先级
  - 任务控制块（TCB）
  - 执行上下文

### 2. 任务状态
任务在运行时会有以下几种状态：
- 就绪态（Ready）：可以运行，等待调度器调度
- 运行态（Running）：正在运行
- 阻塞态（Blocked）：等待事件或延时
- 挂起态（Suspended）：被手动挂起
- 删除态（Deleted）：任务被删除

## 二、实践项目：多任务LED控制

我们将创建一个包含三个任务的项目：
1. LED1任务：控制LED快速闪烁（500ms）
2. LED2任务：控制LED慢速闪烁（1000ms）
3. 监控任务：打印任务状态信息

### 硬件准备
- LED1: PC13（板载LED）
- LED2: PB12（需要外接LED）
- 串口：PA9(TX)、PA10(RX) 用于输出调试信息

### STM32CubeMX配置步骤

1. GPIO配置
   - PC13: GPIO_Output（LED1）
   - PB12: GPIO_Output（LED2）
   - PA9: USART1_TX
   - PA10: USART1_RX

2. USART1配置
   - Mode: Asynchronous
   - Baud Rate: 115200
   - Word Length: 8 Bits
   - Parity: None
   - Stop Bits: 1

3. FreeRTOS配置
   - Tasks and Queues:
     - 配置三个任务的优先级和堆栈大小
     - LED1Task: Priority Normal, Stack 128 bytes
     - LED2Task: Priority Normal, Stack 128 bytes
     - MonitorTask: Priority Low, Stack 256 bytes

### 代码实现

```c
/* 任务句柄定义 */
osThreadId_t LED1TaskHandle;
osThreadId_t LED2TaskHandle;
osThreadId_t MonitorTaskHandle;

/* 任务属性定义 */
const osThreadAttr_t LED1Task_attributes = {
  .name = "LED1Task",
  .stack_size = 128,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t LED2Task_attributes = {
  .name = "LED2Task",
  .stack_size = 128,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t MonitorTask_attributes = {
  .name = "MonitorTask",
  .stack_size = 256,
  .priority = (osPriority_t) osPriorityLow,
};

/* 任务函数声明 */
void LED1_Task(void *argument);
void LED2_Task(void *argument);
void Monitor_Task(void *argument);

/* 任务创建 */
void MX_FREERTOS_Init(void) {
  /* 创建LED任务 */
  LED1TaskHandle = osThreadNew(LED1_Task, NULL, &LED1Task_attributes);
  LED2TaskHandle = osThreadNew(LED2_Task, NULL, &LED2Task_attributes);
  MonitorTaskHandle = osThreadNew(Monitor_Task, NULL, &MonitorTask_attributes);
}

/* LED1任务实现 */
void LED1_Task(void *argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    osDelay(500);
  }
}

/* LED2任务实现 */
void LED2_Task(void *argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    osDelay(1000);
  }
}

/* 监控任务实现 */
void Monitor_Task(void *argument)
{
  char buf[64];
  osDelay(1000);
  
  for(;;)
  {
    sprintf(buf, "Task Status:\r\nLED1 Task: %d\r\nLED2 Task: %d\r\n", 
            osThreadGetState(LED1TaskHandle),
            osThreadGetState(LED2TaskHandle));
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
    osDelay(3000);
  }
}
```

## 三、CMSIS-RTOS V2 任务管理API

### 1. 任务创建
- `osThreadNew()`: 创建新任务
  - 参数1：任务函数
  - 参数2：传递给任务的参数
  - 参数3：任务属性结构体指针
  - 返回值：任务句柄

### 2. 任务属性结构体
```c
typedef struct {
  const char *name;        // 任务名称
  uint32_t attr_bits;      // 任务属性位
  void *cb_mem;           // 控制块内存
  uint32_t cb_size;       // 控制块大小
  void *stack_mem;        // 堆栈内存
  uint32_t stack_size;    // 堆栈大小（字节）
  osPriority_t priority;  // 任务优先级
  // ... 其他属性
} osThreadAttr_t;
```

### 3. 任务控制
- `osThreadTerminate()`: 终止任务
- `osThreadGetState()`: 获取任务状态
- `osThreadGetName()`: 获取任务名称
- `osThreadGetId()`: 获取当前任务ID
- `osThreadYield()`: 任务让出CPU
- `osThreadSuspend()`: 挂起任务
- `osThreadResume()`: 恢复任务

### 4. 任务延时
- `osDelay()`: 相对延时（毫秒）
- `osDelayUntil()`: 绝对延时

### 5. 任务状态
- `osThreadState_t`枚举类型：
  - osThreadInactive: 未激活
  - osThreadReady: 就绪
  - osThreadRunning: 运行
  - osThreadBlocked: 阻塞
  - osThreadTerminated: 终止
  - osThreadError: 错误

### 6. 任务优先级
- `osPriority_t`枚举类型：
  - osPriorityNone: 无优先级
  - osPriorityIdle: 空闲优先级
  - osPriorityLow: 低优先级
  - osPriorityNormal: 普通优先级
  - osPriorityAboveNormal: 高于普通优先级
  - osPriorityHigh: 高优先级
  - osPriorityRealtime: 实时优先级

## 四、V2版本的主要改进
1. 更现代的API设计
2. 更灵活的任务属性配置
3. 更完善的错误处理
4. 更好的类型安全性
5. 更统一的命名规范

## 五、注意事项
1. V2版本的任务函数参数类型为`void *`
2. 任务属性结构体必须正确初始化
3. 堆栈大小单位为字节（而不是字）
4. 优先级使用新的枚举类型
5. 错误处理更加规范

## 六、重要配置说明（V1版本）

### FreeRTOSConfig.h 关键配置
在使用任务管理相关功能时，需要在`FreeRTOSConfig.h`中启用相关功能：

```c
/* 任务状态获取相关配置 */
#define INCLUDE_eTaskGetState        1    /* 启用eTaskGetState函数 */
#define configUSE_TRACE_FACILITY    1    /* 启用可跟踪功能 */
#define INCLUDE_xTaskAbortDelay     1    /* 启用任务延时中止功能 */
```

这些配置的作用：
1. `INCLUDE_eTaskGetState`：
   - 启用获取任务状态的功能
   - 如果不定义为1，eTaskGetState函数将无法使用
   - 编译时会出现"undefined symbol"错误

2. `configUSE_TRACE_FACILITY`：
   - 启用FreeRTOS的跟踪功能
   - 允许系统收集任务的运行时信息
   - 是使用任务状态获取功能的前提条件

3. `INCLUDE_xTaskAbortDelay`：
   - 允许中止任务的延时状态
   - 与任务状态管理相关
   - 增强任务控制的灵活性

### 为什么需要这些配置？
1. FreeRTOS采用模块化设计，很多功能默认是禁用的，以节省内存
2. 使用任务状态获取功能时，需要显式启用相关模块
3. 不启用这些配置会导致链接错误（如：undefined symbol）
4. 这些配置会略微增加系统开销，但对于STM32F103来说影响很小

### 配置建议
1. 在开发调试阶段建议启用这些功能
2. 在最终产品中，如果不需要任务状态监控，可以禁用以节省资源
3. 修改配置后需要重新编译整个项目

## 七、实际代码示例

以下是任务管理的完整示例代码：

```c
/* 任务句柄定义 */
osThreadId_t LED1TaskHandle;
osThreadId_t LED2TaskHandle;
osThreadId_t MonitorTaskHandle;

/* 任务属性定义 */
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

const osThreadAttr_t MonitorTask_attributes = {
    .name = "MonitorTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityLow,
};

/* 任务函数实现 */
void LED1_Task(void *argument)
{
  for(;;)
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

  for(;;)
  {
    flags = osEventFlagsWait(LED_EventHandle,
                             LED2_EVENT_BIT,
                             osFlagsWaitAny,
                             osWaitForever);
    if ((flags & LED2_EVENT_BIT) == LED2_EVENT_BIT)
    {
      DEBUG_Print("LED2 event received!\r\n");
      HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }
  }
}

void Monitor_Task(void *argument)
{
  osDelay(1000); // 等待其他任务初始化完成

  for(;;)
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