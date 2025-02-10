# FreeRTOS中断管理

## 一、中断基础概念

### 1. 中断类型
- 硬件中断：由外部硬件触发
  - 外部中断（EXTI）
  - 定时器中断
  - 串口中断
  - DMA中断
- 软件中断：由软件触发
  - SVC（系统服务调用）
  - PendSV（可挂起的系统调用）
  - SysTick（系统滴答定时器）

### 2. 中断优先级
- 优先级分组：0-4组
- 抢占优先级：决定中断是否可以打断其他中断
- 子优先级：同级中断的响应顺序
- FreeRTOS配置：
  ```c
  // 配置最低中断优先级
  #define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15
  // 配置内核中断优先级
  #define configKERNEL_INTERRUPT_PRIORITY          (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << 4)
  // 配置最高中断优先级
  #define configMAX_SYSCALL_INTERRUPT_PRIORITY     3
  ```

## 二、FreeRTOS中断处理规则

### 1. 中断服务函数（ISR）规则
1. ISR应尽可能短
2. 避免在ISR中执行耗时操作
3. 使用中断安全的API函数
4. 正确处理中断优先级

### 2. 中断安全API
- `...FromISR()`后缀的函数：
  ```c
  xQueueSendFromISR()      // 从中断发送队列数据
  xSemaphoreGiveFromISR()  // 从中断释放信号量
  xTaskResumeFromISR()     // 从中断恢复任务
  ```

### 3. 临界区保护
```c
// 方法1：关闭所有中断
taskENTER_CRITICAL();
// 临界区代码
taskEXIT_CRITICAL();

// 方法2：关闭部分中断
UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
// 临界区代码
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
```

## 三、实践示例：外部中断控制LED

### 1. 硬件准备
- KEY1：PA0（外部中断输入）
- LED1：PC13（LED输出）
- LED2：PB12（LED输出）

### 2. 代码实现
```c
/* 外部中断配置 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == KEY1_Pin)
    {
        // 从中断发送事件标志
        xEventGroupSetBitsFromISR(
            LED_EventHandle,
            LED1_EVENT_BIT | LED2_EVENT_BIT,
            &xHigherPriorityTaskWoken
        );
        
        // 如果需要任务切换，触发PendSV中断
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* LED控制任务 */
void LED_Control_Task(void *argument)
{
    EventBits_t uxBits;
    
    for(;;)
    {
        // 等待中断触发的事件
        uxBits = xEventGroupWaitBits(
            LED_EventHandle,
            LED1_EVENT_BIT | LED2_EVENT_BIT,
            pdTRUE,    // 清除事件标志
            pdFALSE,   // 任一事件满足即可
            portMAX_DELAY
        );
        
        // 处理事件
        if((uxBits & LED1_EVENT_BIT) != 0)
        {
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        }
        if((uxBits & LED2_EVENT_BIT) != 0)
        {
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
        }
    }
}
```

## 四、中断优先级配置建议

### 1. 优先级分配原则
1. 最高优先级：紧急硬件中断
2. 中等优先级：普通硬件中断
3. 低优先级：软件中断和系统调用
4. 最低优先级：空闲任务

### 2. 示例配置
```c
// 中断优先级配置示例
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 配置EXTI中断优先级
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    // KEY1引脚配置
    GPIO_InitStruct.Pin = KEY1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);
}
```

## 五、调试技巧

### 1. 常见问题
1. 中断嵌套导致的死锁
2. 优先级配置不当导致的响应延迟
3. 中断服务函数过长影响系统实时性
4. 临界区保护不当导致的数据竞争

### 2. 调试方法
1. 使用逻辑分析仪观察中断响应时间
2. 添加调试输出跟踪中断执行
3. 使用断点调试关键代码
4. 检查中断优先级配置

### 3. 优化建议
1. 最小化中断服务函数
2. 合理使用中断优先级
3. 避免在中断中执行耗时操作
4. 正确使用中断安全API

## 六、注意事项

1. 中断安全
   - 使用`...FromISR()`函数
   - 正确处理任务切换
   - 保护共享资源
   -不能在ISR中调用可能阻塞的函数，如`vTaskDelay()`
   -不能在ISR里调用`pvPortMalloc()`/`vPortFree()`（避免`heap`碎片化）
   -不能在ISR里调用`vTaskDelete()`（避免任务调度问题）
   -如果ISR需要通知任务，必须用`xQueueSendFromISR()`、`xSemaphoreGiveFromISR()`、`xTaskResumeFromISR()`等中断安全函数

2. 优先级管理
   - 合理分配中断优先级
   - 避免优先级反转
   - 注意中断嵌套

3. 实时性保证
   - 控制中断服务时间
   - 避免关中断时间过长
   - 合理使用临界区保护

## 七、实际代码示例

以下是中断管理的完整示例代码：

```c
/* 中断事件标志位定义 */
#define INT_LED1_EVENT_BIT 0x04
#define INT_LED2_EVENT_BIT 0x08

/* 中断事件标志位定义 */
#define EVT_TIMER_UPDATE 0x01 // 定时器更新事件
#define EVT_KEY_PRESS 0x02    // 按键按下事件
#define EVT_UART_CMD 0x04     // 串口命令事件

/* 中断控制任务句柄 */
osThreadId_t IntLedTaskHandle;

/* 中断控制任务属性 */
const osThreadAttr_t IntLed_attributes = {
    .name = "IntLedTask",
    .stack_size = 256,
    .priority = (osPriority_t)osPriorityNormal,
};

/* 中断控制LED任务 */
void IntLed_Task(void *argument)
{
  EventBits_t uxBits;

  for (;;)
  {
    // 等待中断触发的事件
    uxBits = xEventGroupWaitBits(
        LED_EventHandle,
        INT_LED1_EVENT_BIT | INT_LED2_EVENT_BIT,
        pdTRUE,  // 清除事件标志
        pdFALSE, // 任一事件满足即可
        portMAX_DELAY);

    // 处理事件
    if ((uxBits & INT_LED1_EVENT_BIT) != 0)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
      DEBUG_Print("LED1 Toggled by Interrupt\r\n");
    }
    if ((uxBits & INT_LED2_EVENT_BIT) != 0)
    {
      HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
      DEBUG_Print("LED2 Toggled by Interrupt\r\n");
    }
  }
}

/* 定时器中断回调函数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (htim->Instance == TIM3) // 假设使用TIM3
  {
    // 从中断发送事件标志
    xEventGroupSetBitsFromISR(
        LED_EventHandle,
        EVT_TIMER_UPDATE,
        &xHigherPriorityTaskWoken);

    // 如果需要任务切换，触发PendSV中断
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* 按键中断回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (GPIO_Pin == KEY1_Pin)
  {
    // 从中断发送事件标志
    xEventGroupSetBitsFromISR(
        LED_EventHandle,
        EVT_KEY_PRESS,
        &xHigherPriorityTaskWoken);

    DEBUG_Print("Key Interrupt Triggered!\r\n");

    // 如果需要任务切换，触发PendSV中断
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* 串口接收中断回调函数 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (huart->Instance == USART1)
  {
    // 从中断发送事件标志
    xEventGroupSetBitsFromISR(
        LED_EventHandle,
        EVT_UART_CMD,
        &xHigherPriorityTaskWoken);

    // 如果需要任务切换，触发PendSV中断
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}