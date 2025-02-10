# FreeRTOS软件定时器

## 一、软件定时器概述

软件定时器是FreeRTOS提供的一种定时执行机制，它基于系统节拍（SysTick）实现，不占用硬件定时器资源。

### 1. 特点
- 基于系统节拍实现
- 在定时器守护任务中执行回调
- 支持单次和周期性触发
- 精度依赖于系统节拍率
- 不占用硬件定时器资源

### 2. 应用场景
- 周期性任务执行
- 延时操作
- 超时处理
- 状态监控
- LED控制

## 二、软件定时器API

### 1. 定时器创建
```c
osTimerId_t osTimerNew(
    osTimerFunc_t func,     // 定时器回调函数
    osTimerType_t type,     // 定时器类型（单次/周期）
    void *argument,         // 传递给回调函数的参数
    const osTimerAttr_t *attr  // 定时器属性
);
```

### 2. 定时器控制
```c
// 启动定时器
osStatus_t osTimerStart(
    osTimerId_t timer_id,   // 定时器句柄
    uint32_t ticks         // 定时周期（系统节拍数）
);

// 停止定时器
osStatus_t osTimerStop(osTimerId_t timer_id);

// 删除定时器
osStatus_t osTimerDelete(osTimerId_t timer_id);
```

### 3. 定时器类型
```c
typedef enum {
    osTimerOnce     = 0,    // 单次触发
    osTimerPeriodic = 1     // 周期性触发
} osTimerType_t;
```

## 三、实践示例

### 1. 定时器定义
```c
// 定时器句柄
osTimerId_t LED_TimerHandle;
osTimerId_t Print_TimerHandle;
osTimerId_t Delay_TimerHandle;

// 定时器属性
const osTimerAttr_t LED_Timer_attributes = {
    .name = "LEDTimer"
};

const osTimerAttr_t Print_Timer_attributes = {
    .name = "PrintTimer"
};

const osTimerAttr_t Delay_Timer_attributes = {
    .name = "DelayTimer"
};
```

### 2. 定时器创建和启动
```c
// 创建LED闪烁定时器（周期性，500ms）
LED_TimerHandle = osTimerNew(LED_Timer_Callback, osTimerPeriodic, NULL, &LED_Timer_attributes);
if(LED_TimerHandle == NULL)
{
    DEBUG_Print("LED Timer creation failed!\r\n");
    Error_Handler();
}

// 创建状态打印定时器（周期性，1000ms）
Print_TimerHandle = osTimerNew(Print_Timer_Callback, osTimerPeriodic, NULL, &Print_Timer_attributes);
if(Print_TimerHandle == NULL)
{
    DEBUG_Print("Print Timer creation failed!\r\n");
    Error_Handler();
}

// 创建延时操作定时器（单次，2000ms）
Delay_TimerHandle = osTimerNew(Delay_Timer_Callback, osTimerOnce, NULL, &Delay_Timer_attributes);
if(Delay_TimerHandle == NULL)
{
    DEBUG_Print("Delay Timer creation failed!\r\n");
    Error_Handler();
}

// 启动定时器
osTimerStart(LED_TimerHandle, 500);    // 500ms周期
osTimerStart(Print_TimerHandle, 1000); // 1000ms周期
osTimerStart(Delay_TimerHandle, 2000); // 2000ms后触发一次
```

### 3. 定时器回调函数
```c
// LED定时器回调函数
void LED_Timer_Callback(void *argument)
{
    static uint8_t led_state = 0;
    led_state = !led_state;
    
    if(led_state)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); // LED亮
        DEBUG_Print("LED ON\r\n");
    }
    else
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);   // LED灭
        DEBUG_Print("LED OFF\r\n");
    }
}

// 状态打印定时器回调函数
void Print_Timer_Callback(void *argument)
{
    // 获取系统运行时间
    uint32_t tick = osKernelGetTickCount();
    char buf[64];
    snprintf(buf, sizeof(buf), "System running time: %lu ms\r\n", tick);
    DEBUG_Print(buf);
}

// 延时操作定时器回调函数
void Delay_Timer_Callback(void *argument)
{
    DEBUG_Print("Delay timer triggered once!\r\n");
    // 可以在这里执行一些延时后的操作
}
```

## 四、注意事项

### 1. 回调函数限制
- 在定时器守护任务上下文中执行
- 不能调用会阻塞的API函数
- 应尽量简短，避免长时间运行
- 不能直接调用非线程安全的函数

### 2. 定时精度
- 依赖于系统节拍率（configTICK_RATE_HZ）
- 实际定时误差可能为0~1个节拍周期
- 不适合高精度定时要求的场合

### 3. 资源管理
- 定时器数量受内存限制
- 及时删除不再使用的定时器
- 合理设置定时器守护任务的栈大小

### 4. 使用建议
1. 对时间精度要求高的场合使用硬件定时器
2. 软件定时器适合非精确定时的应用
3. 避免在回调函数中执行耗时操作
4. 使用周期性定时器代替多个单次定时器
5. 合理规划定时器数量，避免占用过多系统资源

## 五、应用示例

### 1. LED呼吸灯效果
```c
// 使用软件定时器实现LED呼吸效果
void LED_Breath_Timer_Callback(void *argument)
{
    static uint8_t brightness = 0;
    static uint8_t increasing = 1;
    
    if(increasing)
    {
        brightness++;
        if(brightness >= 100) increasing = 0;
    }
    else
    {
        brightness--;
        if(brightness == 0) increasing = 1;
    }
    
    // 这里应该使用PWM控制LED亮度
    // 为了演示，我们用简单的开关模拟
    if(brightness > 50)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    }
}
```

### 2. 超时处理
```c
// 使用软件定时器实现操作超时处理
void Operation_Timeout_Callback(void *argument)
{
    // 操作超时处理
    DEBUG_Print("Operation timeout!\r\n");
    // 执行超时后的清理工作
    // ...
}

// 启动操作并设置超时
void StartOperation(void)
{
    // 启动超时定时器
    osTimerStart(timeout_timer, 5000);  // 5秒超时
    
    // 开始操作
    // ...
}

// 操作完成时
void OperationComplete(void)
{
    // 停止超时定时器
    osTimerStop(timeout_timer);
    
    // 处理操作结果
    // ...
}
```

### 3. 周期性状态监控
```c
// 使用软件定时器实现系统状态监控
void Status_Monitor_Callback(void *argument)
{
    // 检查系统状态
    uint32_t free_heap = xPortGetFreeHeapSize();
    uint32_t min_heap = xPortGetMinimumEverFreeHeapSize();
    
    // 检查任务状态
    char buf[128];
    snprintf(buf, sizeof(buf), 
             "System Status:\r\n"
             "Free Heap: %lu\r\n"
             "Min Heap: %lu\r\n",
             free_heap, min_heap);
    DEBUG_Print(buf);
}
```

## 六、调试技巧

1. 定时器状态监控
   - 检查定时器是否成功创建
   - 监控回调函数的执行情况
   - 观察定时器触发的时间间隔

2. 常见问题排查
   - 定时器未启动
   - 回调函数执行时间过长
   - 系统节拍配置不合适
   - 内存不足导致创建失败

3. 优化建议
   - 合理设置定时周期
   - 优化回调函数执行效率
   - 避免创建过多定时器
   - 及时清理不用的定时器资源

## 六、实际代码示例

以下是软件定时器的完整示例代码：

```c
/* 软件定时器句柄 */
osTimerId_t Print_TimerHandle;
osTimerId_t LED_Breath_TimerHandle;

/* 定时器属性定义 */
const osTimerAttr_t Print_Timer_attributes = {
    .name = "PrintTimer"
};

const osTimerAttr_t LED_Breath_Timer_attributes = {
    .name = "LEDBreathTimer"
};

/* 状态打印定时器回调函数 */
void Print_Timer_Callback(void *argument)
{
  // 获取系统运行时间
  uint32_t tick = osKernelGetTickCount();
  char buf[64];
  snprintf(buf, sizeof(buf), "System running time: %lu ms\r\n", tick);
  DEBUG_Print(buf);
}

/* LED呼吸灯定时器回调函数 */
void LED_Breath_Timer_Callback(void *argument)
{
  static uint8_t brightness = 0;
  static uint8_t increasing = 1;

  if (increasing)
  {
    brightness++;
    if (brightness >= 100)
      increasing = 0;
  }
  else
  {
    brightness--;
    if (brightness == 0)
      increasing = 1;
  }

  // 这里应该使用PWM控制LED亮度
  // 为了演示，我们用简单的开关模拟
  if (brightness > 50)
  {
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  }
  else
  {
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  }
}

/* 初始化函数中的定时器创建和启动部分 */
void MX_FREERTOS_Init(void)
{
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
} 