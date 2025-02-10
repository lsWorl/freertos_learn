# STM32F103 FreeRTOS LED闪烁项目 - 详细操作指南

## 第一步：STM32CubeMX配置

### 1. 时钟配置
1. 打开learn_project.ioc文件
2. RCC配置
   - 在Pinout&Configuration选项卡中
   - RCC > High Speed Clock (HSE) > Crystal/Ceramic Resonator
   - RCC > Low Speed Clock (LSE) > 保持Disable

3. 时钟树配置
   - 点击Clock Configuration选项卡
   - 设置HSE = 8MHz
   - 设置PLL Source Mux = HSE
   - 设置HCLK = 72MHz（系统主频）
   - 确保APB1 = 36MHz（最大）
   - 确保APB2 = 72MHz

### 2. GPIO配置
1. LED引脚配置
   - 在Pinout视图中找到PC13
   - 设置为GPIO_Output
   - 在左侧Configuration中找到GPIO
   - 配置PC13参数：
     - GPIO Mode: Output Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: Low（因为是LED，不需要高速）

### 3. FreeRTOS配置
1. 启用FreeRTOS
   - 在左侧Middleware中选择FREERTOS
   - Interface设置为CMSIS_V1
   
2. 配置FreeRTOS参数
   - Tasks and Queues选项卡：
     - 默认任务可以保留
     - Stack Size设置为128 words
     - Priority保持Normal
   - Advanced Settings:
     - 将Heap Size设置为4096 bytes
     - 选择Heap_4作为内存管理方式

### 4. 调试配置
1. 配置调试接口
   - 在System Core中选择SYS
   - Debug设置为Serial Wire

### 5. 项目设置
1. Project Manager选项卡
   - Project > Toolchain/IDE > MDK-ARM V5
   - Code Generator > 
     - 勾选"Generate peripheral initialization as a pair of '.c/.h' files per peripheral"
     - 勾选"Keep User Code when re-generating"

2. 生成代码
   - 点击Generate Code按钮
   - 等待代码生成完成

## 第二步：代码编写

### 1. 修改freertos.c文件
在`freertos.c`中的USER CODE BEGIN区域添加以下代码：

```c
/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* USER CODE BEGIN Variables */
/* LED任务句柄 */
osThreadId LEDTaskHandle;
/* USER CODE END Variables */

/* USER CODE BEGIN FunctionPrototypes */
/* LED任务函数声明 */
void LED_Task(void const * argument);
/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN RTOS_THREADS */
  /* 创建LED任务 */
  osThreadDef(LEDTask, LED_Task, osPriorityNormal, 0, 128);
  LEDTaskHandle = osThreadCreate(osThread(LEDTask), NULL);
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Application */
/* LED任务实现 */
void LED_Task(void const * argument)
{
  /* 无限循环 */
  for(;;)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);  /* LED翻转 */
    osDelay(500);                                /* 延时500ms */
  }
}
/* USER CODE END Application */
```

## 第三步：编译和下载

1. 打开MDK-ARM工程
   - 双击MDK-ARM文件夹中的.uvprojx文件
   
2. 编译项目
   - 点击"Build"按钮（F7）
   - 确保没有错误和警告

3. 下载程序
   - 连接ST-LINK到开发板
   - 点击"Download"按钮（F8）
   - 等待下载完成

## 预期结果
- LED将以1Hz的频率闪烁
- 通过观察可以看到LED每500ms改变一次状态

## 故障排除
如果LED不闪烁，请检查：
1. 时钟配置是否正确
2. PC13引脚配置是否正确
3. FreeRTOS任务是否正确创建
4. 下载是否成功

## 代码解释
1. `osThreadDef()`: 定义任务的宏，包含任务名称、函数、优先级、实例数和堆栈大小
2. `osThreadCreate()`: CMSIS-RTOS v1的任务创建函数
3. `osDelay()`: FreeRTOS的延时函数，单位为毫秒
4. `HAL_GPIO_TogglePin()`: HAL库函数，用于翻转GPIO引脚状态
5. 任务函数中的无限循环确保任务持续运行

### CMSIS-RTOS v1 API说明
- `osThreadDef(name, function, priority, instances, stacksz)`
  - name: 任务名称
  - function: 任务函数
  - priority: 任务优先级（osPriorityNormal等）
  - instances: 任务实例数（通常为0）
  - stacksz: 堆栈大小（单位为字）
