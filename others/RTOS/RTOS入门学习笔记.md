---
Data: 2025-08-18
---
# 第一节：RTOS基础知识
## 一、任务调度
- 使用相关的调度算法来决定当前需要执行哪个任务
- FreeRTOS一共支持三种任务调度方式：
1. **抢占式调度**
	- 主要是==针对优先级**不同**的任务==，每个任务都有一个优先级，优先级高的任务可以抢占优先级低的任务（数值越大，优先级越高）![[抢占式调度.png]]
2. **时间片调度**
	- 主要==针对优先级**相同**的任务==，当多个任务的优先级相同时，任务调度器会在**每一次系统时钟节拍**到的时候切换任务
	- 同等优先级任务轮流地享有相同的CPU时间（可设置），叫时间片，在FreeRTOS中，一个时间片就等于SysTick中断周期![[时间片调度.png]]

## 二、任务状态
1. **FreeRTOS一共有四种任务状态**
	- **运行态**：同一时刻**仅一个**任务处于运行状态
	- **就绪态**
	- **阻塞态**：如果一个任务因延时或等待外部事件发生，那么这个任务就处于阻塞态
	- **挂起态**：类似暂停，调用函数vTaskSuspend()进入挂起态，调用函数vTaskResume()才可以进入就绪态 
	![[状态转移.png]]
2. **任务状态列表**
	- **就绪列表**：pxReadyTaskList[x]，共有0~31位
	- **阻塞列表**：pxDelayTaskList
	- **挂起列表**：xSuspendTaskList
	- 注
		- 新创建的任务会被挂载到就绪列表下，角标为其优先级
		- **调度器总是在所有处于就绪列表的任务中，选择具有最高优先级的任务来执行**
		- 相同优先级的任务会连接在同一个就绪列表上
# 第二节：算了不学了，会用就行
## 一、任务创建和删除
-  **`BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, const configSTACK_DEPTH_TYPE uxStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask)`**
	- 例
		``` C
		void vTask(void *pvParameters) {
			//任务初始化
			for(;;){
				//任务的内容
			}
		}
		
		void main(void){
			xTaskCreate(vTask, "Task", 128, NULL, 1, NULL);
			vTaskStartScheduler();
		}
		```
## 二、任务级函数进入阻塞态
- **`void vTaskDelay(const TickType_t xTicksToDelay)`：在任务内执行，将任务阻塞一段时间**
	- 例
		``` C
		vTaskDelay(500 / portTICK_PERIOD_MS);//阻塞延迟500ms
		```
## 三、列表
1. **创建一个列表
	`BaseType_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize)`
	- 例
		``` C
		QueueHandle_t xQueque;//在main.c中定义一个全局变量
		
		void main(void){
			xQueque = xQueueCreate(20, sizeof(uint8_t));
		}
		```
2. **将数据放入列表
	- **任务级函数（可阻塞）**
		- **`BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait)`：最常用的发送函数，实现`FIFO**
			- 参数
				- `xTicksToWait`
					- `0`：**非阻塞模式**
					- `portMAX_DELAY`：无限期阻塞
					- `N`：等待最多 `N` 个时钟节拍
			- 返回值
				- 数据成功发送到队列：返回`pdPASS`
				- 队列处于满的状态：进入阻塞态
				- 阻塞状态超时：返回`errQUEUE_FULL
		- **`BaseType_t xQueueOverwrite(QueueHandle_t xQueue, const void * const pvItemToQueue)`：仅用于长度为1的队列。如果队列已满，它会自动覆盖最旧的数据，==不会阻塞==**
			- 返回值
				- 永远为`pdPASS`
		- 例
			``` C
			void vTask_transmitter(void *pvParameters){
					BaseType_t xStatus =xQueueSendToBack(xQueque, 'a', portMAX_DELAY);
					if(xStatus != pdPASS){
					//数据发送失败
					}
				}
			```

	- **中断级函数（绝不阻塞）**
		- **`BaseType_t xQueueSendToBackFromISR(QueueHandle_t xQueue, const void * const pvItemToQueue, BaseType_t * pxHigherPriorityTaskWoken)`**
			- 参数
				`pxHigherPriorityTaskWoken`：**输出**一个指向任务的上下文切换标识的指针，在调用函数前，将其==初始化为 `pdFALSE`==；如果本次 `Send` 操作**解除阻塞**了一个==优先级高于==被中断的任务（任务能被阻塞，说明队列是空的，那么一定返回`pdPASS`），那么函数就会将 `*pxHigherPriorityTaskWoken`置为 **`pdTRUE`**；如果解除阻塞的是一个==优先级小于或等于==被中断的任务，那么函数就会将`*pxHigherPriorityTaskWoken`置为 **`pdFALSE`**
			- 返回值
				-  数据成功发送到队列：返回`pdPASS`
				- 队列处于满的状态：返回`errQUEUE_FULL
		- 例
			``` C
			void USART1_IRQHandler(void) {
				if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET){
					BaseType_t xHigherPriorityTaskWoken = pdFALSE; // 初始化标识
					uint8_t data =  = USART_ReceiveData(USART1);
					//使用中断安全函数发送数据到队列
				    xQueueSendToBackFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
				        
				    // 我们根据这个标识来决定是否请求一次上下文切换
				    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
			    }
			}
			```
		- 例
			``` C
			void My_ISR(void) {
			    BaseType_t xYieldNeeded = pdFALSE; // 只用一个标识
			    
			    xQueueSendToBackFromISR( xQueue1, &data1, &xYieldNeeded );
			    xQueueSendToBackFromISR( xQueue2, &data2, &xYieldNeeded );
			    // ... 可能还有其他操作
			    
			    portYIELD_FROM_ISR(xYieldNeeded); // 最后统一判断一次即可
			}
			```
		- `portYIELD_FROM_ISR()`
			- 该宏会根据传入的参数决定在执行完本中断函数后是继续执行被中断打断的任务，还是将该任务阻塞，执行更高优先级的任务。
			- ==我向`deepseek`提出的一个问题==：任务A因为被中断而变成了就绪态，任务B因为被唤醒也变成了就绪态，所以现在有两个同时处于就绪态的任务，那么系统在执行完中断以后应该会直接执行优先级更高的任务B，不会执行任务A，所以`portYIELD_FROM_ISR`没有用。
				- 答：您的推论基于一个前提：**调度器总是在任何时刻自动运行，比较就绪任务优先级**。这个前提在**任务上下文**中是正确的，但在**中断上下文**中却有一个例外。关键区别：自动调度 vs. 手动触发调度
					1. **在任务中（自动调度）**：如果一个正在运行的任务调用了`vTaskDelay()`、`xQueueSend()`等函数，这些函数内部会**主动调用调度器**。调度器会检查就绪列表，如果发现有更高优先级的任务就绪，就会立即进行切换。您想象中的行为是在这个场景下发生的。
					2. **在中断中（手动触发调度）**：**中断服务程序 (ISR) 具有最高的优先级，它会抢占任何任务。当 ISR 在执行时，FreeRTOS 的调度器是被“冻结”的，它不会自动运行**。在 ISR 中，`xQueueSendFromISR` 等函数**只会修改内核的数据结构**（例如将任务B添加到就绪列表，并设置 `xHigherPriorityTaskWoken = pdTRUE`）。它们不会主动调用调度器。**CPU 硬件的中断退出机制是“返回被中断的代码处继续执行”**。所以如果没有额外干预，CPU 会忠实地遵循这个机制，回到任务A。

| 特性           | 调用 `portYIELD_FROM_ISR(pdTRUE)` | 不调用 `portYIELD_FROM_ISR(pdTRUE)` |
| ------------ | ------------------------------- | -------------------------------- |
| **任务B响应速度**  | **极快**（微秒级）                     | **较慢**（可能长达一个时间片，如1ms）           |
| **任务A是否会运行** | **不会**                          | **会**，它会继续运行直到时间片用完或被其他中断抢占      |
| **实时性**      | **强实时**，确定性高                    | **弱实时**，响应时间有抖动                  |

2. **将数据读出队列**
	- **任务级函数（可阻塞）**
		- **`BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait)`：最常用的接收函数。它会从队列中==移除==数据项。如果队列为空，任务可以阻塞等待**
			- 参数
				- `xBlockTime`
					- `0`：**非阻塞模式**
					- `portMAX_DELAY`：无限期阻塞
					- `N`：等待最多 `N` 个时钟节拍
			- 返回值
				- 队列中有数据：移除最先进来的数据，返回`pdPASS`
				- 队列中无数据：进入阻塞态，等待数据
				- 阻塞状态超时：返回`pdFALT`
		- **`BaseType_t xQueuePeek(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait)`：查看但==不移除==数据项。数据仍然保留在队列中**
			- 参数
				- `xBlockTime`
					- `0`：**非阻塞模式**
					- `portMAX_DELAY`：无限期阻塞
					- `N`：等待最多 `N` 个时钟节拍
			- 返回值
				- 队列中有数据：返回`pdPASS`
				- 队列中无数据：进入阻塞态，等待数据
				- 阻塞状态超时：返回`pdFALT`
		- 例
			``` C
			void vTask_reciver(void *pvParameters) {
				uint8_t buf = 0;
				BaseType_t xStatus = xQueueReceive(xQueque, &buf, portMAX_DELAY);//如果没有数据就阻塞
				if(xStatus == pdPASS) {
				//成功接收到数据
			    }
			}
			```
	- **中断级函数（绝不阻塞）**
		- **`BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void * const pvBuffer, BaseType_t * const pxHigherPriorityTaskWoken)`**
## 信号量
1. **创建信号量**
	- **二进制信号量**
		**`SemaphoreHandle_t xSemaphoreCreateBinary(void)`：创建后状态为0，==需要先释放一次==**
	- **计数信号量**
		**`SemaphoreHandle_t xSemaphoreCreateCounting(const UBaseType_t uxMaxCount, const UBaseType_t uxInitialCount)`**
	- **互斥锁**
		**`SemaphoreHandle_t xSemaphoreCreateMutex(void)`：创建后状态为1**
	- 例
		``` C
		#include "FreeRTOS.h"
		#include "semphr.h" // 必须包含信号量头文件
	
		SemaphoreHandle_t xBinarySemaphore;
		SemaphoreHandle_t xCountingSemaphore;
		SemaphoreHandle_t xMutex;
		
		void main(void){
		// 创建二进制信号量
	    xBinarySemaphore = xSemaphoreCreateBinary();
	    // 创建计数信号量：最多5把钥匙，初始有3把
	    xCountingSemaphore = xSemaphoreCreateCounting(5, 3);
	    // 创建互斥锁
	    xMutex = xSemaphoreCreateMutex();
		}
		
		// 务必检查创建是否成功！
	    if (xBinarySemaphore == NULL || ...) {
	        // 错误处理
		```
2. **获取（Take）信号量**
	**`BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime)`：获取后信号量-1**
	- 参数
		- `xBlockTime`
			- `0`：**非阻塞模式**
			- `portMAX_DELAY`：无限期阻塞
			- `N`：等待最多 `N` 个时钟节拍
	- 返回值
		- 信号量大于0：信号量-1，返回`pdPASS`
		- 信号量等于0：进入阻塞态，等待信号量
		- 阻塞状态超时：返回`pdFALT`
		
3. **释放（Give）信号量：获取后信号量+1**
	- **任务级函数**
		**`BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)`**
		- 返回值
			- ==有任务等待信号量（此时信号量为0，不可能等于最大值）：返回`pdPASS`，唤醒等待的任务==
			- 信号量小于最大值：信号量+1，返回`pdPASS`
			- 信号量等于最大值：返回`pdFAIL`
	- **中断级函数**
		**`xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t * const pxHigherPriorityTaskWoken)`**
- 例
	``` C
	// 全局句柄
	SemaphoreHandle_t xButtonSemaphore;
	
	// 中断服务程序
	void EXTI0_IRQHandler(void){
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	    // 清除中断标志...
	    // 释放信号量（通知任务）
	    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
	    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	
	// 处理任务
	void vButtonTask(void *pvParameters) {
	    for (;;) {
	        // 无限等待信号量（阻塞在此处，不消耗CPU）
	        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdPASS){
	            // 收到信号量，执行处理逻辑
	            printf("Button was pressed!\n");
	            // 这里可以执行复杂的处理，中断ISR早已退出
		    }
	    }
	}
	
	// 主函数中
	int main(void){
	    xButtonSemaphore = xSemaphoreCreateBinary(); // 创建
	    xTaskCreate(vButtonTask, "ButtonTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	    vTaskStartScheduler();
	    for (;;);
	}
	```
1. **普通二进制信号量与互斥量的区别**




-