# FreeRTOS电源管理

## 一、电源管理概述

FreeRTOS提供了灵活的电源管理机制，可以帮助系统在不同工作状态下实现最优的功耗控制。

### 1. 电源管理的重要性
- 延长电池供电设备的使用时间
- 降低系统发热
- 提高系统可靠性
- 满足低功耗应用需求

### 2. STM32低功耗模式
- Sleep模式：仅关闭CPU时钟
- Stop模式：关闭大部分时钟和电源
- Standby模式：几乎关闭所有电源

## 二、FreeRTOS低功耗支持

### 1. Tickless模式
```c
// 在FreeRTOSConfig.h中启用
#define configUSE_TICKLESS_IDLE         1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2  // 空闲阈值（单位：Tick）
```

### 2. 空闲任务钩子函数
```c
// 定义空闲任务钩子函数
void vApplicationIdleHook(void)
{
    // 在这里实现低功耗管理
    // 例如：检查系统状态，决定是否进入低功耗模式
}
```

## 三、STM32低功耗模式实现

### 1. Sleep模式
```c
void Enter_Sleep_Mode(void)
{
    // 配置为Sleep模式
    HAL_PWR_ConfigPVD(PWR_PVD_MODE_NORMAL);
    HAL_PWR_EnablePVD();
    
    // 进入Sleep模式
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}
```

### 2. Stop模式
```c
void Enter_Stop_Mode(void)
{
    // 配置为Stop模式
    HAL_PWR_ConfigPVD(PWR_PVD_MODE_NORMAL);
    HAL_PWR_EnablePVD();
    
    // 进入Stop模式
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // 退出Stop模式后重新配置系统时钟
    SystemClock_Config();
}
```

### 3. Standby模式
```c
void Enter_Standby_Mode(void)
{
    // 清除唤醒标志
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    
    // 配置唤醒引脚
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    
    // 进入Standby模式
    HAL_PWR_EnterSTANDBYMode();
}
```

## 四、实践示例：低功耗管理系统

### 1. 电源管理任务
```c
// 电源管理任务句柄
osThreadId_t PowerMgrTaskHandle;

// 电源管理任务属性
const osThreadAttr_t PowerMgr_attributes = {
    .name = "PowerMgrTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};

// 电源管理任务实现
void PowerMgr_Task(void *argument)
{
    uint32_t idle_time = 0;
    
    for(;;)
    {
        // 检查系统活动状态
        if(CheckSystemIdle())
        {
            idle_time++;
            
            // 根据空闲时间选择不同的低功耗模式
            if(idle_time > 1000)  // 空闲超过1秒
            {
                Enter_Stop_Mode();
            }
            else if(idle_time > 100)  // 空闲超过100ms
            {
                Enter_Sleep_Mode();
            }
        }
        else
        {
            idle_time = 0;
        }
        
        osDelay(10);
    }
}
```

### 2. 系统活动检测
```c
// 系统活动标志
static volatile uint32_t system_activity = 0;

// 检查系统是否空闲
bool CheckSystemIdle(void)
{
    if(system_activity > 0)
    {
        system_activity--;
        return false;
    }
    return true;
}

// 记录系统活动
void RecordSystemActivity(void)
{
    system_activity = 100;  // 设置活动计数
}
```

### 3. 唤醒源配置
```c
void ConfigureWakeupSources(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 配置按键作为唤醒源
    GPIO_InitStruct.Pin = KEY1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);
    
    // 配置外部中断
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
```

## 五、功耗优化建议

### 1. 硬件层面
1. 禁用不需要的外设时钟
2. 合理配置外设工作频率
3. 使用外设的低功耗模式
4. 控制IO口状态

### 2. 软件层面
1. 优化任务调度，减少任务切换
2. 合理使用任务延时
3. 避免忙等待
4. 使用事件驱动方式

### 3. 系统层面
1. 选择合适的低功耗模式
2. 正确配置唤醒源
3. 管理好系统时钟
4. 监控系统功耗

## 六、调试方法

### 1. 功耗测量
```c
void Power_Monitor_Task(void *argument)
{
    float current;
    char buf[64];
    
    for(;;)
    {
        // 获取系统当前功耗（需要硬件支持）
        current = GetSystemCurrent();
        
        // 打印功耗信息
        snprintf(buf, sizeof(buf), 
                "System Current: %.2f mA\r\n", 
                current);
        DEBUG_Print(buf);
        
        osDelay(1000);
    }
}
```

### 2. 低功耗模式调试
1. 使用LED指示系统状态
2. 通过串口输出调试信息
3. 使用逻辑分析仪观察系统行为
4. 监控唤醒源触发情况

### 3. 常见问题
1. 无法进入低功耗模式
   - 检查外设状态
   - 确认中断配置
   - 验证时钟设置

2. 无法正常唤醒
   - 检查唤醒源配置
   - 确认唤醒中断优先级
   - 验证时钟恢复流程

3. 功耗不达标
   - 检查外设使用情况
   - 优化软件架构
   - 调整低功耗策略

## 七、最佳实践

1. 系统设计
   - 采用模块化的电源管理架构
   - 实现灵活的功耗控制策略
   - 提供完善的调试手段

2. 代码实现
   - 使用状态机管理功耗模式
   - 实现可靠的唤醒机制
   - 保证数据一致性

3. 测试验证
   - 进行完整的功耗测试
   - 验证各种唤醒场景
   - 确保系统稳定性 