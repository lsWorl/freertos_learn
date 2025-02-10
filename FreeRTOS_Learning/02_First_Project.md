# 第一个FreeRTOS项目：LED闪烁

## 硬件资源
- 开发板：STM32F103C8T6
- LED：PC13（板载LED）
- 系统时钟：72MHz
- 下载器：ST-LINK

## 项目目标
创建两个任务：
1. LED闪烁任务：控制PC13 LED每500ms翻转一次
2. 空闲任务：系统默认创建

## 详细步骤

### 1. STM32CubeMX配置
#### 系统配置
- RCC > HSE: Crystal/Ceramic Resonator
- SYS > Debug: Serial Wire
- 时钟配置：
  - HSE = 8MHz
  - HCLK = 72MHz
  - APB1 = 36MHz
  - APB2 = 72MHz

#### GPIO配置
- PC13: GPIO_Output
  - GPIO mode: Output Push Pull
  - GPIO Pull-up/Pull-down: No pull-up and no pull-down
  - Maximum output speed: Low

#### FreeRTOS配置
- Middleware > FREERTOS
  - Interface: CMSIS_V2
  - Heap size: 4096 bytes
  - 默认任务参数：
    - Stack size: 128 words
    - Priority: Normal

### 2. 代码实现
在生成代码后，需要修改以下文件：

#### freertos.c
```c
/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* 添加任务句柄 */
osThreadId_t LEDTaskHandle;

/* 任务函数声明 */
void LEDTask(void *argument);

/* 任务属性配置 */
const osThreadAttr_t LEDTask_attributes = {
  .name = "LEDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

void MX_FREERTOS_Init(void) {
  /* 创建LED任务 */
  LEDTaskHandle = osThreadNew(LEDTask, NULL, &LEDTask_attributes);
}

/* LED任务实现 */
void LEDTask(void *argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    osDelay(500);
  }
}
```

## 预期结果
- LED将以1Hz的频率闪烁（亮500ms，灭500ms）
- 系统通过FreeRTOS的任务调度实现LED控制

## 注意事项
1. 确保所有时钟配置正确
2. 注意堆栈大小的设置
3. 使用osDelay而不是HAL_Delay
4. 任务中必须包含死循环

## 学习要点
1. FreeRTOS任务的创建和基本配置
2. 任务函数的基本结构
3. FreeRTOS延时函数的使用
4. GPIO的基本控制

您要开始创建这个项目吗？我可以指导您一步步在STM32CubeMX中完成配置。 