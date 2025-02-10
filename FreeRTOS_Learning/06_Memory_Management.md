# FreeRTOS内存管理

## 一、内存管理概述

FreeRTOS提供了多种内存分配方案，主要包括：
1. heap_1：最简单的内存分配方案，不支持内存释放
2. heap_2：支持内存释放，但可能产生内存碎片
3. heap_3：使用标准库的malloc/free
4. heap_4：支持内存合并，最常用的方案
5. heap_5：支持跨内存区域的内存管理

## 二、内存管理特点

### 1. heap_1
- 特点：
  - 最简单的实现方式
  - 只支持内存分配，不支持释放
  - 内存池是一个大数组
  - 没有内存碎片问题
- 适用场景：
  - 系统只在启动时分配内存
  - 运行时不需要动态分配/释放内存
  - 内存使用模式固定

### 2. heap_2
- 特点：
  - 支持内存分配和释放
  - 按照最佳匹配算法分配内存
  - 可能产生内存碎片
  - 不支持内存合并
- 适用场景：
  - 需要频繁分配/释放相同大小的内存块
  - 内存块大小比较固定

### 3. heap_3
- 特点：
  - 使用标准库的malloc/free
  - 线程安全（使用互斥量保护）
  - 性能取决于标准库实现
  - 占用更多代码空间
- 适用场景：
  - 需要与其他使用标准库的代码协同工作
  - 对内存管理性能要求不高

### 4. heap_4（推荐）
- 特点：
  - 支持内存分配、释放和合并
  - 使用最先匹配算法
  - 相邻的空闲块会自动合并
  - 效率较高
- 适用场景：
  - 需要频繁的内存分配/释放
  - 对内存使用效率要求较高
  - 通用场景的首选方案

### 5. heap_5
- 特点：
  - 支持多个内存区域
  - 可以管理不连续的内存空间
  - 基于heap_4的实现
  - 需要额外的初始化
- 适用场景：
  - 系统有多个内存区域
  - 需要管理不连续的内存空间

## 三、内存管理API

### 1. 内存分配函数
```c
// 分配内存
void *pvPortMalloc(size_t xSize);

// 释放内存
void vPortFree(void *pv);

// 获取空闲堆空间大小
size_t xPortGetFreeHeapSize(void);

// 获取最小剩余堆空间大小（历史最低值）
size_t xPortGetMinimumEverFreeHeapSize(void);
```

### 2. 使用示例
```c
// 分配内存示例
void MemoryAllocExample(void)
{
    // 分配一个整型数组
    int *array = (int *)pvPortMalloc(10 * sizeof(int));
    if (array != NULL)
    {
        // 使用内存
        for (int i = 0; i < 10; i++)
        {
            array[i] = i;
        }
        
        // 使用完后释放
        vPortFree(array);
    }
    
    // 获取当前空闲堆空间
    size_t freeHeap = xPortGetFreeHeapSize();
    
    // 获取历史最小空闲堆空间
    size_t minHeap = xPortGetMinimumEverFreeHeapSize();
}
```

## 四、内存管理配置

### 1. FreeRTOSConfig.h配置
```c
// 堆大小配置（字节）
#define configTOTAL_HEAP_SIZE    ((size_t)(10 * 1024))

// 使用的内存管理方案
#define configHEAP_ALLOCATION_SCHEME 4  // 使用heap_4

// 内存对齐配置
#define configMINIMAL_STACK_SIZE ((uint16_t)128)
#define configALIGNMENT 8
```

### 2. heap_4实现原理
1. 内存块结构：
```c
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;  // 指向下一个空闲块
    size_t xBlockSize;                     // 当前块大小
} BlockLink_t;
```

2. 分配策略：
- 使用最先匹配算法
- 从空闲链表中查找第一个足够大的块
- 如果块太大，会分割成两部分
- 分配的内存自动对齐

3. 释放策略：
- 将释放的块添加到空闲链表
- 检查并合并相邻的空闲块
- 维护空闲块链表的顺序

## 五、内存使用建议

### 1. 一般原则
1. 优先使用静态分配
2. 避免频繁的小内存分配
3. 注意内存对齐要求
4. 及时释放不用的内存
5. 监控系统内存使用情况

### 2. 调试技巧
1. 使用内存统计函数监控内存使用
2. 检查内存泄漏
3. 观察内存碎片情况
4. 合理设置堆大小

### 3. 性能优化
1. 使用内存池管理小块内存
2. 避免频繁分配/释放
3. 合理设置内存对齐
4. 选择合适的内存管理方案

## 六、实践示例

### 1. 内存监控任务
```c
void MemMonitorTask(void *argument)
{
    for(;;)
    {
        // 获取当前空闲堆空间
        size_t freeHeap = xPortGetFreeHeapSize();
        
        // 获取历史最小空闲堆空间
        size_t minHeap = xPortGetMinimumEverFreeHeapSize();
        
        printf("Current free heap: %d bytes\n", freeHeap);
        printf("Minimum ever free heap: %d bytes\n", minHeap);
        
        osDelay(1000);
    }
}
```

### 2. 内存池示例
```c
// 定义内存池
#define POOL_SIZE 10
#define BLOCK_SIZE 32

static uint8_t memPool[POOL_SIZE][BLOCK_SIZE];
static uint8_t memPoolUsed[POOL_SIZE] = {0};

// 从内存池分配内存
void *poolMalloc(void)
{
    for(int i = 0; i < POOL_SIZE; i++)
    {
        if(memPoolUsed[i] == 0)
        {
            memPoolUsed[i] = 1;
            return memPool[i];
        }
    }
    return NULL;
}

// 释放内存回内存池
void poolFree(void *ptr)
{
    for(int i = 0; i < POOL_SIZE; i++)
    {
        if(memPool[i] == ptr)
        {
            memPoolUsed[i] = 0;
            break;
        }
    }
}
```
### 3. 测试不同内存分配策略的性能差异
```c
// 内存测试配置
#define TEST_SMALL_SIZE   32    // 小块内存大小
#define TEST_MEDIUM_SIZE  128   // 中等内存大小
#define TEST_LARGE_SIZE   512   // 大块内存大小
#define TEST_COUNT        50    // 每种大小测试次数

// 添加内存测试任务句柄
osThreadId_t MemTestTaskHandle;

// 内存测试任务属性
const osThreadAttr_t MemTest_attributes = {
    .name = "MemTestTask",
    .stack_size = 1024,  // 增加堆栈大小以适应测试
    .priority = (osPriority_t)osPriorityNormal,
};

// 创建内存测试任务
  MemTestTaskHandle = osThreadNew(MemTest_Task, NULL, &MemTest_attributes);
  if(MemTestTaskHandle == NULL)
  {
    DEBUG_Print("MemTestTask creation failed!\r\n");
    Error_Handler();
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
```

## 七、实际代码示例

以下是内存管理的完整示例代码：

```c
/* 内存池配置 */
#define POOL_SIZE 10  // 内存池中的块数量
#define BLOCK_SIZE 32 // 每个内存块的大小（字节）

/* 内存测试配置 */
#define TEST_SMALL_SIZE 32   // 小块内存大小
#define TEST_MEDIUM_SIZE 128 // 中等内存大小
#define TEST_LARGE_SIZE 512  // 大块内存大小
#define TEST_COUNT 50        // 每种大小测试次数

/* 内存池相关变量 */
static uint8_t memPool[POOL_SIZE][BLOCK_SIZE]; // 内存池数组
static uint8_t memPoolUsed[POOL_SIZE] = {0};   // 内存块使用状态 0-未使用 1-已使用
static osMutexId_t memPoolMutexHandle;         // 内存池互斥量

/* 内存池互斥量属性 */
const osMutexAttr_t memPoolMutex_attributes = {
    .name = "MemPoolMutex"
};

/* 内存池测试任务句柄 */
osThreadId_t MemPoolTestTaskHandle;

/* 内存池测试任务属性 */
const osThreadAttr_t MemPoolTest_attributes = {
    .name = "MemPoolTestTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityNormal,
};

/* 内存测试任务句柄 */
osThreadId_t MemTestTaskHandle;

/* 内存测试任务属性 */
const osThreadAttr_t MemTest_attributes = {
    .name = "MemTestTask",
    .stack_size = 1024, // 增加堆栈大小以适应测试
    .priority = (osPriority_t)osPriorityNormal,
};

/* 内存监控任务句柄 */
osThreadId_t MemMonitorTaskHandle;

/* 内存监控任务属性 */
const osThreadAttr_t MemMonitor_attributes = {
    .name = "MemMonitorTask",
    .stack_size = 512,
    .priority = (osPriority_t)osPriorityLow,
};

/* 内存池分配函数 */
void *poolMalloc(void)
{
  void *ptr = NULL;

  // 获取互斥量
  if (osMutexAcquire(memPoolMutexHandle, osWaitForever) == osOK)
  {
    // 查找空闲内存块
    for (int i = 0; i < POOL_SIZE; i++)
    {
      if (memPoolUsed[i] == 0)
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
void poolFree(void *ptr)
{
  // 获取互斥量
  if (osMutexAcquire(memPoolMutexHandle, osWaitForever) == osOK)
  {
    // 查找要释放的内存块
    for (int i = 0; i < POOL_SIZE; i++)
    {
      if (memPool[i] == ptr)
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
  void *ptr_array[5] = {NULL}; // 用于存储分配的内存指针

  // 等待其他任务初始化完成
  osDelay(1000);

  for (;;)
  {
    DEBUG_Print("\r\n=== Memory Pool Test ===\r\n");

    // 分配测试
    DEBUG_Print("Allocating memory blocks...\r\n");
    for (int i = 0; i < 5; i++)
    {
      ptr_array[i] = poolMalloc();
      if (ptr_array[i] != NULL)
      {
        // 使用内存块，写入测试数据
        uint8_t *data = (uint8_t *)ptr_array[i];
        for (int j = 0; j < BLOCK_SIZE; j++)
        {
          data[j] = i + 1; // 填充测试数据
        }
      }
      osDelay(200); // 延时以便观察
    }

    // 释放测试
    DEBUG_Print("\r\nFreeing memory blocks...\r\n");
    for (int i = 0; i < 5; i++)
    {
      if (ptr_array[i] != NULL)
      {
        poolFree(ptr_array[i]);
        ptr_array[i] = NULL;
      }
      osDelay(200); // 延时以便观察
    }

    DEBUG_Print("\r\n=== Test Complete ===\r\n");
    osDelay(5000); // 等待5秒后重新开始测试
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

  for (;;)
  {
    DEBUG_Print("\r\n=== Memory Allocation Performance Test ===\r\n");

    // 1. 测试小块内存分配/释放性能
    DEBUG_Print("\r\nTesting small block allocation...\r\n");
    start_time = osKernelGetTickCount();

    // 分配测试
    for (int i = 0; i < TEST_COUNT; i++)
    {
      ptr_array[i] = pvPortMalloc(TEST_SMALL_SIZE);
      if (ptr_array[i] != NULL)
      {
        // 写入一些测试数据
        memset(ptr_array[i], 0xAA, TEST_SMALL_SIZE);
      }
    }

    // 释放测试
    for (int i = 0; i < TEST_COUNT; i++)
    {
      if (ptr_array[i] != NULL)
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

    for (int i = 0; i < TEST_COUNT; i++)
    {
      ptr_array[i] = pvPortMalloc(TEST_MEDIUM_SIZE);
      if (ptr_array[i] != NULL)
      {
        memset(ptr_array[i], 0xBB, TEST_MEDIUM_SIZE);
      }
    }

    for (int i = 0; i < TEST_COUNT; i++)
    {
      if (ptr_array[i] != NULL)
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

    for (int i = 0; i < TEST_COUNT; i++)
    {
      ptr_array[i] = pvPortMalloc(TEST_LARGE_SIZE);
      if (ptr_array[i] != NULL)
      {
        memset(ptr_array[i], 0xCC, TEST_LARGE_SIZE);
      }
    }

    for (int i = 0; i < TEST_COUNT; i++)
    {
      if (ptr_array[i] != NULL)
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
```
