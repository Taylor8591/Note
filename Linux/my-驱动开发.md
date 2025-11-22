
# 第一章：环境搭建
## 一、搭建编译环境
``` bash
sudo apt update
sudo apt install gcc make git bc libssl-dev liblz4-tool device-tree-compiler bison flex u-boot-tools gcc-aarch64-linux-gnu
```
## 二、获取内核源码
``` bash
git clone -b lbc-develop-6.1 https://github.com/LubanCat/kernel.git
```
## 三、编译内核
``` bash
# 清除之前生成的所有文件和配置
make mrproper
# 加载 lubancat_linux_rk3576_defconfig 配置文件，rk3576 系列均是该配置文件
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- lubancat_linux_rk3576_defconfig
# 编译内核，指定交叉编译工具，使用 8 线程进行编译，线程可根据电脑性能自行确定
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8
```
## 四、编译和加载内核驱动模块
- **编译内核驱动模块**
	内核（驱动）模块加载到内核，可以将内核模块编译成单独的模块，在内核启动后由用户手动动态加载， 也可以将模块直接编译进内核，在内核启动时就自动加载。**测试一般是单独编译成内核模块**，然后手动加载，方便调试，同时也节省时间
	``` bash
	git clone https://gitee.com/LubanCat/lubancat_rk_code_storage
	```
	获取到源码后，源码目录下的 `linux_driver`文件夹就是存放驱动教程的例程文件，将其配套驱动程序代码放置到**内核代码同级目录**，原因是编译内核模块时，驱动程序需要依赖编译好的 Linux 内核，驱动模块中的`Makefile`中指定了内核的路径，为方便使用例程，请放至同一目录结构下![[Pasted image 20250917204446.png]]
	``` bash
	cd linux_driver/module/hellomodule/ 
	make
	```
- **加载内核驱动模块**
	``` bash
	# 进入放编译好的内核驱动模块的目录
	cd /home/cat/ 
	# 加载内核模块 
	sudo insmod hellomodule.ko 
	# 查看当前加载的内核模块 
	lsmod 
	# 卸载内核模块 
	sudo rmmod hellomodule.ko
		```
## 五、编译和加载设备树
- **使用内核工具编译设备树**
	在编译 Linux 内核时，会生成名为`dtc`(Device Tree Compiler)的工具，该工具用于自动编译设备树源文件(`.dts`或`.dtsi`文件)为二进制的设备树文件(`.dtb`文件)。
	``` bash
	内核目录/scripts/dtc/dtc -I dts -O dtb -o xxx.dtb xxx.dts
	```
- ==**使用编译内核的方式编译设备树**==
	- 在`kernel`中
		``` bash
		make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- lubancat_linux_rk3576_defconfig
		make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 dtbs
		```
	- 在`SDK/kernel`中
		``` bash
		make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- lubancat_linux_rk3576_defconfig
		make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j8 dtbs
		```
- **加载设备树**
	加载设备树，将编译好的新设备树文件，替换对应板卡的设备树，替换`/boot/dtb/`目录下的设备树文件即可。
	1. **确定板卡使用的设备树文件**
		``` bash
		ls -l /boot/
		```
		![[Pasted image 20250917210002.png]]
		可以从上图中看到`rk-kernel.dtb`链接到了`dtb/rk3566-lubancat-1n.dtb`，在系统启动过程中会读取 `rk-kernel.dtb`作为系统设备树，实际读取的设备树为`/boot/dtb/rk3566-lubancat-1n.dtb`。因此，如果需要修改和替换系统加载的设 备树，那么就要修改`rk-kernel.dtb`软链接的设备树。
	2. **替换设备树**
# 第二章：Linux 内核模块
## 一、内核模块的概念
- **内核驱动模块**
	我们编写的内核模块，经过编译，最终形成==`.ko`==为后缀的`ELF`文件。这是一种普通的可重定位目标文件。这类文件包含了代码和数据，可以被用来链接成可执行文件或共享目标文件，静态链接库也可以归为这一类。
	- 我们可以使用`readelf`工具查看`elf`文件的头部详细信息
		``` bash
		readelf -h hellomodule.ko
		```
## 二、导出符号
==用于模块间的代码共享==
- 使用以下方法以导出符号
	``` C
	EXPORT_SYMBOL(name)
	EXPORT_SYMBOL_GPL(name)
	```
- 注
	`GPL`使得导出的模块只能被`GPL`许可的模块使用
- 例
	``` C
	#include <linux/module.h>
	#include <linux/kernel.h>
	#include <linux/init.h>
	
	// 定义一个导出的变量
	int shared_value = 100;
	EXPORT_SYMBOL(shared_value);  // 导出变量（所有模块可使用）
	
	// 定义一个导出的函数
	void shared_function(const char *msg) {
		printk(KERN_INFO "Shared function called: %s\n", msg);
	}
	EXPORT_SYMBOL_GPL(shared_function);  // 导出函数（仅GPL许可证模块可使用）
	
	// 模块初始化函数
	static int __init provider_init(void) {
		printk(KERN_INFO "Symbol provider module loaded\n");
		return 0;
	}
	
	// 模块退出函数
	static void __exit provider_exit(void) {
		printk(KERN_INFO "Symbol provider module unloaded\n");
	}
	
	module_init(provider_init);
	module_exit(provider_exit);
	
	MODULE_LICENSE("GPL");  // 必须声明GPL许可证，否则EXPORT_SYMBOL_GPL无效
	MODULE_AUTHOR("Your Name");
	MODULE_DESCRIPTION("A module that exports symbols");

	```
	
	``` C
	#include <linux/module.h>
	#include <linux/kernel.h>
	#include <linux/init.h>
	
	// 声明要使用的外部符号（来自provider模块）
	extern int shared_value;                  // 外部变量
	extern void shared_function(const char *); // 外部函数
	
	// 模块初始化函数
	static int __init consumer_init(void) {
		printk(KERN_INFO "Symbol consumer module loaded\n");
		
		// 使用导出的变量
		printk(KERN_INFO "Read shared_value: %d\n", shared_value);
		shared_value = 200;  // 修改导出的变量（其他模块也会看到修改后的值）
		printk(KERN_INFO "Modified shared_value to: %d\n", shared_value);
		
		// 使用导出的函数
		shared_function("Hello from consumer module");
		return 0;
	}
	
	// 模块退出函数
	static void __exit consumer_exit(void) {
		printk(KERN_INFO "Symbol consumer module unloaded\n");
	}
	
	module_init(consumer_init);
	module_exit(consumer_exit);
	
	MODULE_LICENSE("GPL");  // 必须与provider模块的许可证兼容（若使用EXPORT_SYMBOL_GPL）
	MODULE_AUTHOR("Your Name");
	MODULE_DESCRIPTION("A module that uses exported symbols");
	```
## 三、传递参数与符号共享实验
==加载模块时传递参数==
- `module_param(name, type, perm)`：向内核中传递参数
	- `name`：我们定义的变量名
	- `type：参数的类型
	- `perm`：该文件的权限
- 例
	``` C
	#include <linux/init.h>
	#include <linux/module.h>
	#include <linux/moduleparam.h>
	#include <linux/kernel.h>
	
	// 1. 整数类型参数
	static int int_param = 10;               // 默认值10
	module_param(int_param, int, 0644);
	MODULE_PARM_DESC(int_param, "An integer parameter (default: 10)");
	
	// 模块初始化函数
	static int __init param_demo_init(void) {
		// 打印整数类型参数
		printk(KERN_INFO "int_param: %d\n", int_param);
		
		return 0;
	}
	
	// 模块退出函数
	static void __exit param_demo_exit(void) {
		printk(KERN_INFO "Parameter demo module unloaded\n");
	}
	
	// 注册模块初始化和退出函数
	module_init(param_demo_init);
	module_exit(param_demo_exit);
	
	// 模块元信息
	MODULE_LICENSE("GPL");
	MODULE_AUTHOR("Your Name");
	MODULE_DESCRIPTION("A module demonstrating module_param usage");
	MODULE_VERSION("1.0");
	```
	- 编译模块后，可通过以下方式加载并传递参数
		``` bash
		insmod param_demo.ko int_param=20
		```
## ==四、模块命令==
``` bash
# 加载内核模块 
insmod xxx.ko 
# 卸载内核模块 
rmmod xxx.ko
#查看当前模块列表
lsmod
```
## 五、内核模块模板
- **驱动部分**
	``` C
	#include <linux/module.h>
	#include <linux/init.h>
	#include <linux/kernel.h>
	
	static int __init hello_init(void) {
		printk(KERN_EMERG "[ KERN_EMERG ] Hello Module Init\n");
		printk(KERN_EMERG "[ default ] Hello Module Init\n");
		return 0;
	}
	
	static void __exit hello_exit(void) {
		printk("[ default ] Hello Module Exit\n");
	}
	
	module_init(hello_init);//（必须）
	module_exit(hello_exit);//（必须）
	
	MODULE_LICENSE("GPL");//（必须）
	```
	- **Linux 内核模块的代码框架**
		1. 模块加载函数（必须）
		2. 模块卸载函数（必须）
		3. 模块许可证声明（必须）
		4. 模块参数
		5. 模块导出符号
		6. 模块的其他相关信息
	- **头文件**
		在Linux 的应用编程，Linux 的头文件都存放在`/usr/include`中，编写内核模块所需要的头文件，并不在上述说到的目录，而是在`内核源码/include`中
		- `#include <linux/module.h>`：包含内核模块信息声明的相关函数
		- `#include <linux/init.h>`：包含了`module_init()`和`module_exit()`函数的声明
		- `#include <linux/kernel.h>`：包含内核提供的各种函数，如`printk`
	-  **`static`关键字的作用**
		- `static`修饰的**静态局部变量**直到程序运行结束以后才释放，延长了局部变量的生命周期
		- `static`的修饰**全局变量**只能在本文件中访问，不能在其它文件中访问
		- `static`修饰的**函数**只能在本文件中调用，不能被其他文件调用
	- **`printk`打印等级**
		- `KERN_EMERG`通常是系统崩溃前的信息
		- `KERN_ALERT`需要立即处理的消息
		- `KERN_CRIT`严重情况
		- `KERN_ERR` 错误情况 
		- `KERN_WARNING`有问题的情况
		- `KERN_NOTICE`
		- `KERN_INFO`普通消息
		- `KERN_DEBUG`调试信息
- **`makefile`内容**
	``` makefile
	KERNEL_DIR = ../../../kernel/
	ARCH = arm64
	CROSS_COMPILE = aarch64-linux-gnu-
	export ARCH CROSS_COMPILE
	
	obj-m := xxx.o
	
	all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules
	.PHONE:clean
	
	clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean
	```
	- **`KERNEL_DIR=../../../kernel/`**
		这个路径指向你的Linux**内核源代码树的根目录**，必须是一个已经配置好（即执行过`make menuconfig`或拥有`.config`文件）的完整内核源码目录
	- **`export`**
		意味着这些变量不仅在本`Makefile`中有效，还会传递给任何由本`Makefile`启动的子进程（sub-make）。这是关键的一步，因为接下来我们要调用内核目录下的`Makefile`，那个`Makefile`需要读取这两个环境变量来知道如何配置自己
	- **`obj-m`**
		表示要编译成**可加载内核模块**（loadable kernel module）的对象列表。==编译产物是`.ko`文件==
		- **`obj-y`**
			表示要编译并直接链接进内核镜像(`vmlinux`)的对象列表
	- **`MAKE`**
		是一个内置变量，其值就是`make`程序本身的路径。使用`$(MAKE)`而不是直接写`make`是更好的实践，因为它保证了传递所有必要的选项和环境变量
	- **`-C`**
		是`make`命令的一个选项，意思是 “Change directory”。它指示`make`进程先切换到`$(KERNEL_DIR)`目录（即我们的内核源码目录），然后再去读取和执行该目录下的`Makefile`
	- **`M`**
		是一个传递给内核顶层`Makefile`的变量。这是整个过程的精髓所在。它的意思是：“亲爱的内核构建系统，虽然我现在是在你的地盘`$(KERNEL_DIR)`上运行，但我真正要构建的东西的源代码其实在`M`所指的目录（`$(CURDIR)`，即当前目录）里。” 内核的`Makefile`被设计为可以识别`M`变量，当看到这个变量时，它就知道要离开内核源码树，去指定的外部目录编译一个外部模块
	- **`modules`**
		这是传递给内核`Makefile`的目标（target）。它指示内核构建系统执行编译外部模块所需的规则
# 第三章：`pinctrl`子系统
用于管理系统中所有的`pin`，也可以设置管脚的==复用关系==，电器属性等。`pinctrl`定义在芯片的设备树文件中，如`rk3506.dtsi`，我们需要在我们的板级设备树文件中在该节点中引用`pinctrl`
## 一、构成
```
//客户端
led {
	pinctrl-names = "default";
	pinctrl-0 = <&led1>;
};

//服务端
&pinctrl {
	...
	led {
		led1:led1 {
			rockchip,pins = <0 RK_PC5 RK_FUNC_GPIO &pcfg_pull_up>;
		};
	};
	...
}
```
## 二、服务端
- `Pin groups`：表示一组 pins，这一组pins统一表示了一种功能
- `function`：一组引脚功能
- `pin state`
``` dts
&pinctrl {
	...
	spi { //function
		spi3_cs0: spi3-cs0 { //groups
			rockchip,pins = <4 RK_PC6 RK_FUNC_GPIO &pcfg_pull_up_drv_level_1>;
		};
			
		spi3_cs1: spi3-cs1 {
			rockchip,pins = <4 RK_PC4 RK_FUNC_GPIO &pcfg_pull_up_drv_level_1>;
		};
	};
	...
}
```
# 第四章：`GPIO`子系统
## 一、GPIO控制器
- 我们可以在芯片的设备树文件中看到`gpio`节点下有`gpio-controller`
	![[Pasted image 20251004200444.png]]
- **在板级设备树文件中描述`GPIO`**
	``` dts
	/ {
		...
		adc:adc {
			status = "okay";
			compatible = "led_test";
			cs-gpios = <&gpio0 RK_PC7 GPIO_ACTIVATE_HIGH>;
			interrupt-gpios = <&gpio0 RK_PC7 GPIO_ACTIVATE_HIGH>;
			reset-gpios = cs-gpios = <&gpio0 RK_PC9 GPIO_ACTIVATE_HIGH>;
		};
		...
	};
	```
## 二、`GPIO`相关`api`
- **获取`GPIO`的`api`**
	定义在了`include/linux/gpio/consumer.h`
	``` C
	struct gpio_desc *__must_check devm_gpiod_get(
		struct device *dev,
		const char *con_id,
		enum gpiod_flags flags
	)
	
	struct gpio_desc *__must_check devm_gpiod_get_index(
		struct device *dev,
		const char *con_id,
		unsigned int idx,
		enum gpiod_flags flags
	)
	
	struct gpio_desc *__must_check devm_gpiod_get_optional(
		struct device *dev,
		const char *con_id,
		enum gpiod_flags flags
	)
	
	struct gpio_desc *__must_check devm_gpiod_get_index_optional(
		struct device *dev,
		const char *con_id,
		unsigned int index,
		enum gpiod_flags flags
	)
	
	struct gpio_descs *__must_check devm_gpiod_get_array(
		struct device *dev,
		const char *con_id,
		enum gpiod_flags flags
	)
	
	struct gpio_descs *__must_check devm_gpiod_get_array_optional(
		struct device *dev,
		const char *con_id,
		enum gpiod_flags flags
	)
	```
	- ==`con_id`==
		`gpios`属性名的前缀，如设备树中`xxx-gpios = <xxx>`，则`con_id`为`xxx`
		如果`gpios`属性名无前缀，仅为`gpios`，则`con_id`为`NULL`
	- `flags`
		`GPIOD_ASIS`
		`GPIOD_IN`
		`GPIOD_OUT_LOW`
		`GPIOD_OUT_HIGH`
		`GPIOD_OUT_LOW_OPEN_DRAIN`
		`GPIOD_OUT_HIGH_OPEN_DRAIN`
	- `index`：索引值，从0开始，针对`led-gpios = <xxx> <xxx> <xxx>`的情况
- **方向控制**
	``` C
	int gpiod_direction_input(struct gpio_desc *desc)
	int gpiod_direction_output(struct gpio_desc *desc, int value)
	```
- **读写操作**
	``` C
	int gpiod_get_value(const struct gpio_desc *desc)
	void gpiod_set_value(struct gpio_desc *desc, int value)
	```
# 第五章：中断
## 一、基本知识
1. **中断号**
	- ==`IRQ number`：软中断号，在Linux中是唯一的，也是我们在编程中使用的中断号==
	- `HW interrupt ID`：硬中断号
	- `IRQ domain`：将硬中断号与软中断号进行映射
2. **中断源**
	- `SGI(Software Generated Inperrupt)`：中断号在0~15之间
	- `PPI(Private Perpheral Interrupt)`：中断号在16~31之间
	- ==`SPI(Shared Perpheral Interrupt)`：中断号在32~1020之间==
## 二、中断控制器
- **中断控制器分为：**
	- `GIC`通用中断控制器
		对于 ARM GIC，通常 `#interrupt-cells = <3>`，三个参数的意义：
		- 中断类型
			`GIC_SPI`：共享外设中断
			`GIC_PPI`：私有外设中断
		- 中断号
		- 触发类型
			`IRQ_TYPE_EDGE_RISING`
			`IRQ_TYPE_EDGE_FALLING`
			`IRQ_TYPE_EDGE_BOTH`
			`ITYPE_LEVEL_HIGH`
			`IRQ_TYPE_LEVEL_LOW`
	- `GPIO`中断控制器
		对于 GPIO 控制器，通常 `#interrupt-cells = <2>`，两个参数的意义：
		- 中断号
		- 触发类型
	- ![[Pasted image 20251004200456.png]]
- **在设备树文件中描述中断**
	``` dts
	/ {
		...
		ft5x06:ft5x06 {
			status = "okay";
			compatible = "ft5x06";
			
			//描述一个中断
			interrupt-parent = <&gpio0>;//表示中断信号连接的是哪个中断控制器
			interrupts = <RK_PC7 IRQ_TYPE_EDGE_RISING>;//表示中断引脚如触发方式
			
			//描述多个中断
			interrupt-parent = <&gpio0>;
			interrupts = <RK_PC7 IRQ_TYPE_EDGE_RISING>, //索引号为0
				<RK_PC8 IRQ_TYPE_EDGE_RISING>;//索引号为1
			interrupt-names = "interrupt1", "interrupt2";//可选：为中断命名
		};
		...
	};
	```
## 三、中断相关`api`
- **获取中断的`api`**
	``` C
	//使用索引号获取
	int platform_get_irq(struct platform_device *pdev, unsigned int index)
	int platform_get_irq_optional(
		struct platform_device *pdev, 
		unsigned int index
	)
	
	//使用中断名获取
	int platform_get_irq_byname(struct platform_device *dev, const char *name)
	```
	- `index`：索引号，从0开始
- **注册中断**
	``` C
	int devm_request_irq(
		struct device *dev, unsigned int irq, 
		irq_handler_t handler, unsigned long flags, 
		const char *name, void *dev_id
	)
	```
	- `unsigned long flags`：中断标志位
		`IRQF_TRIGGER_RISING`     // 上升沿触发
		`IRQF_TRIGGER_FALLING`    // 下降沿触发
		`IRQF_TRIGGER_HIGH`       // 高电平触发  
		`IRQF_TRIGGER_LOW`        // 低电平触发
		`IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING`  // 双边沿触发
		
		`IRQF_SHARED`             // 共享中断
		`IRQF_ONESHOT`            // 线程化中断使用
	- `const char *name`：中断名称（用于标识）
		在`/proc/interrupts`中显示的名称
## 四、`gpio`中断
- **方法一**
1. 设备节点
	``` dts
	led_test:led_test {
		status = "okay";
		compatible = "led_test";
		interrupt-gpios = <&gpio0 RK_PC7 GPIO_ACTIVATE_HIGH>;
		pinctrl-names = "default";
		pinctrl-0 = <&led1>;
	}
	&pinctrl {
		...
		led1:led1 {
			rockchip,pins = <0 RK_PC7 RK_FUNC_GPIO &pcfg_pull_up>;;
		};
		...
	}
	```
2. 获取GPIO
	``` C
	struct gpio_desc *__must_check devm_gpiod_get(
		struct device *dev,
		const char *con_id,
		enum gpiod_flags flags
	)
	```
3. 获取中断号
	``` C
	//远古时期版本，不建议使用
	int gpio_to_irq(unsigned gpio);
	
	//新版，建议使用
	int gpiod_to_irq(struct gpio_desc *desc)
	```
4. 注册中断
	``` C
	int devm_request_irq(
		struct device *dev, unsigned int irq,
		irq_handler_t handler, unsigned long flags, 
		const char *name, void *dev_id
	)
	```
- **方法二**
1. 设备节点
	``` dts
	led_test:led_test {
		status = "okay";
		compatible = "led_test";
		interrupt-parent = <&gpio0>;
		interrupts = <RK_PC7 IRQ_TYPE_EDGE_RISING>;
		pinctrl-names = "default";
		pinctrl-0 = <&led1>;
	}
	&pinctrl {
		...
		led1:led1 {
			rockchip,pins = <0 RK_PC7 RK_FUNC_GPIO &pcfg_pull_up>
		}
		...
	}
	```
2. 获取中断
	``` C
	int platform_get_irq(struct platform_device *pdev, unsigned int index)
	```
3. 注册中断
	``` C
	int devm_request_irq(
		struct device *dev, unsigned int irq,
		irq_handler_t handler, unsigned long flags, 
		const char *name, void *dev_id
	)
	```
## 五、中断下文
- `tasklet`是一种特殊的软中断，在Linux内核中，一般使用`tasklet`机制来实现中断下文
- tasklet的api
	- 静态方法
		``` C
		#define DECLARE_TASKLET(name, func, data)
		#define DECLARE_TASKLET_DISABLE(name, func, data)
		```
	- 动态方法
		``` C
		void tasklet_init(
			struct tasklet_struct *t, 
			void (*func)(unsigned long), 
			unsigned long data
		)
		```
- tasklet使能与失能
	``` C
	void tasklet_enable(struct tasklet_struct *t)
	void tasklet_disable(struct tasklet_struct *t)
	```
- tasklet调度与取消调度
	``` C
	void tasklet_schedule(struct tasklet_struct *t);
	void tasklet_kill(struct tasklet_struct *t);
	```
- 例
	``` C
	#include <linux/module.h>
	#include <linux/init.h>
	#include <linux/interrupt.h>
	#include <linux/gpio.h>
	
	int irq;
	struct tasklet_struct t;
	
	irqreturn_t test_interrupt(int irq, void *args) {
		return IRQ_RETVAL(IRQ_HANDLED);
	}
	
	void test_tasklet(unsigned long data) {
		...
		//不能有sleep函数
	}
	
	static int interrupt_irq_init(void) {
		
		int ret;
		irq = gpio_to_irq(13);
		ret = request_irq(irq, test_interrupt, IRQ_TRIGGER_FALLING. "test", NULL);
		if(ret < 0) {
			return -1;
		}
		tasklet_init(&t, test_tasklet, 1)
		return 0;
	}
	
	static void interrupt_irq_exit(void) {
		
		free_irq(irq, NULL);
		tasklet_enable(&t);
		tasklet_kill(&t);
	}
	
	module_init(interrupt_irq_init);
	module_exit(interrupt_irq_exit);
	
	MODULE_LIENCE("GPL");
	```
# 第六章：字符设备
## 一、设备的基本概念
- **设备号**
	- **主设备号**用于标识设备对应的驱动程序，告诉 Linux 内核使用哪一个**驱动程序**为该设备服务
	- **次设备号**可以用一个**驱动程序**去控制各种设备
	- ==查看Linux系统中所有**设备文件**==
		``` bash
		ls -l /dev/
		```
		每一行的第一个字符
		- c：标识字符设备
		- b：用来标识块设备
	- ==查看**设备驱动**与设备号的映射关系==
		``` bash
		cat /proc/devices
		```
		记录"主设备号 + 驱动名称"
- **设备节点**
	**一个设备节点其实就是一个文件**，Linux 中称为设备文件。有一点必要说明的是，在 Linux 中，所有的设备访问都是通 过文件的方式，一般的数据文件程序普通文件，设备节点称为设备文件。
	设备节点被创建在`/dev` 下，是连接内核与用户层的枢纽，就是设备是接到对应哪种接口的哪个 ID 上。
	- 使用命令创建
		``` bash
		mknod [选项] 设备节点路径 设备类型 主设备号 次设备号
		```
	- 使用命令删除
		``` bash
		rm /dev/chrdev
		```
	- ==使用类创建==
		``` C
		struct class *my_class
		
		//my_device_class会作为目录名出现在/sys/class/下
		my_class = class_create(THIS_MODULE, "my_device_class");
		if (IS_ERR(my_class)) {
			...
		}
		if (IS_ERR(device_create(my_class, NULL, dev_num, NULL, "my_device"))) {
			...
		}
		```
	- ==使用类删除==
		``` C
		device_destroy(my_class, dev_num);
		class_destroy(my_class);
		```
- **数据结构**
	- `struct file_operations`
		定义了对文件的所有操作方法。把**系统调用**和**驱动程序**关联起来的关键数据结构。
		``` C
		struct file_operations {
			struct module *owner;
			loff_t (*llseek) (struct file *, loff_t, int);
			ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
			ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
			long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
			int (*open) (struct inode *, struct file *)
			int (*release) (struct inode *, struct file *);
		};
		```
		- **在系统调用和驱动程序之间拷贝数据的api**
			1. 将数据从**用户空间**安全地复制到**内核空间**
				``` C
				 unsigned long copy_from_user(
				  void *to, const void __user *from, unsigned long n
			  )
				```
				- 参数
					`void *to`：内核空间的目标缓冲区指针（数据复制的目的地）
					`const void __user *from`：用户空间的源数据指针
					`unsigned long n`：字节数
			2. 将数据从**内核空间**安全地复制到**用户空间**
				``` C
				unsigned long copy_to_user(
					void __user *to, const void *from, unsigned long n
				)
				```
				- 参数
					`void __user *to`：用户空间的目标缓冲区指针
					`const void *from`：内核空间的源数据指针
					`unsigned long n`：字节数
	- `struct cdev`
		``` C
		struct cdev {
			...
		    struct module *owner;// 指向拥有该设备的模块（通常为 THIS_MODULE）
		    const struct file_operations *ops;// 指向设备的操作函数集（核心成员）
		    dev_t dev;// 设备号（包含主设备号和次设备号）
		    ...
		}
		```
## 二、驱动程序流程
1. **字符设备的定义与注销**
	- 定义字符设备结构体
		``` C
		//第一种方式
		static struct cdev chrdev;
		//第二种方式
		struct cdev *cdev_alloc(void);
		```
	- 移除字符设备
		``` C
		void cdev_del(struct cdev *p);
		```
2. **设备编号的申请与注销**
	- 申请字符设备设备编号
		-  静态地为一个字符设备申请一个或多个设备编号
			``` C
			int register_chrdev_region(
				dev_t from, unsigned count, const char *name
			)
			```
		- ==动态地为一个字符设备申请一个或多个设备编号==
			``` C
			int alloc_chrdev_region(
				dev_t *dev, unsigned baseminor, unsigned count, const char *name
			)
			```
			- 参数
				`unsigned baseminor`：次设备号，可写0
	- 注销字符设备设备编号
		``` C
		void unregister_chrdev_region(dev_t from, unsigned count)
		```
3. **初始化`cdev`**
	``` C
	void cdev_init(struct cdev *cdev, const struct file_operations *fops)
	```
4. **设备注册和注销**
	- 注册设备
		``` C
		int cdev_add(struct cdev *cdev, dev_t dev, unsigned count)
		```
	- 注销设备
		``` C
		void cdev_del(struct cdev *cdev)
		```
# §3.7：devm_系列函数
## 一、申请内存
``` C
void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
```
## 二、中断相关
``` C
	int devm_request_irq(
		struct device *dev, unsigned int irq,
		irq_handler_t handler, unsigned long flags, 
		const char *name, void *dev_id
	)
```
## 三、GPIO相关
``` C
struct gpio_desc *devm_gpiod_get(
	struct device *dev, 
	const char *con_id, 
	enum gpiod_flags flags
)
```
- `enum gpiod_flags flags`
	`GPIOD_IN` → 输入模式
	`GPIOD_OUT_LOW` → 输出模式，初始值拉低
	`GPIOD_OUT_HIGH` → 输出模式，初始值拉高
	`GPIOD_ASIS` → 保持硬件原有状态，不修改方向
## 四、IO相关
``` C
void __iomem *devm_ioremap(struct device *dev, resource_size_t offset,
                          resource_size_t size);
void __iomem *devm_ioremap_resource(struct device *dev, struct resource *res);
void __iomem *devm_platform_ioremap_resource(struct platform_device *pdev,
                                           unsigned int index);
```
#  §3.8：设备树与平台总线
`platform`总线模型是Linux虚拟出来的，思想是**将以前1个.c文件分成设备device.c与driver.c两部分**
## 一、设备树语法
1. **第一行要写的东西**
	`/dts-v1/;`
2. **设备树根节点**
	``` 
	/ {
	...
	};
	```
3. **子节点**
	```
	节点标签：node_name@单元地址 {
		
	};
	```
	- 单元地址的值要和节点`reg`属性的第一个地址一致（建议），如果节点没有“reg”属性值，可以直接省略
	- 同一级的节点名字不能相同，不同级的节点名字可以相同
4. **属性**
	1. ==`reg`属性==
		`reg = <address1 length1 address2 length2 ...>;`
		`#address-cells`与`#size-cells`确定了子节点中的`address`与`lengrh`的个数
		- 如
			``` C
			node1 {
				#address-cells = <n>;
				#size-cells = <n>;
				node1-child {
					reg = <0x10>;
				};
			};
			```
	2. `model`属性
		model属性的值是一个字符串，一般用来描述一些信息，比如设备的名字
		`model = "..."`
	3. `status`属性
		`okay`：设备是可用状态
		`disable`：设备是不可用状态
		`fail`：设备是不可用状态并且设备检测到了错误
		`fail-sss`：设备是不可用状态并且设备检测到了错误，sss是错误内容
	4. `compatible`属性
		`compatible`是一个非常重要的属性，用来和驱动进行匹配，匹配成功后会执行驱动中的`probe`函数
		`compatible = "aaa", "bbb";`
		- 在匹配的时候会先使用第一个值`aaa`进行匹配，如果没有就会使用第二个值`bbb`进行匹配
	5. `range`属性
	6. 追加/修改节点内容
		```
		&节点标签 {
			cpu-supply = <&vdd_cpu>;
		}
		```
## 二、获取节点的属性
1. **获取字符串属性**
	``` C
	int of_property_read_string(
		struct device_node *np, 
		const char *propname, 
		const char **out_string
	)
	```
2. **获取整数属性**
	``` C
	int of_property_read_u32(
		struct device_node *np, 
		const char *propname, 
		u32 *out_value
	)
	```
3. **获取布尔属性**
	``` C
	bool of_property_read_bool(
		struct device_node *np, 
		const char *propname
	)
	```
4. **获取`reg`属性**
	``` C
	struct resource *platform_get_resource(
		struct platform_device *dev,
		unsigned int type, 
		unsigned int num
	)
	```
	`unsigned int type`： 资源类型，写`IORESOURCE_MEM`
	`num`: 资源索引（从0开始）
5. **获取中断**
6. **获取GPIO**
- 例
	``` dts
	my_device {
	    compatible = "vendor,my-device";
	    device-name = "my_uart";
	    clock-frequency = <115200>;
	    dma-enabled;
	    reg = <0x0 0xff180000 0x0 0x1000>;
	    gpios = <&gpio0 RK_PC7 GPIO_ACTIVATE_HIGH>
	    interrupt-parent = <&gpio0>
	    interrupts = <RK_PC7 IRQ_TYPE_EDGE_RISING>;
	};
	```
	
	``` C
	static int my_probe(struct platform_device *pdev) {
		//非常重要，记下
	    struct device_node *np = pdev->dev.of_node;
	    const char *string_prop;
	    u32 int_prop;
	    u32 array_prop[3];
	    int ret;
		
	    // 1. 读取字符串属性
	    ret = of_property_read_string(np, "device-name", &string_prop);
	    if (!ret) {
	        printk("Device name: %s\n", string_prop);
	    }
		
	    // 2. 读取整数属性
	    ret = of_property_read_u32(np, "clock-frequency", &int_prop);
	    if (!ret) {
	        printk("Clock frequency: %d\n", int_prop);
	    }
		
	    // 3. 读取数组属性
	    ret = of_property_read_u32_array(np, "reg", array_prop, 3);
	    if (!ret) {
	        printk("Reg: 0x%x, 0x%x, 0x%x\n", 
	               array_prop[0], array_prop[1], array_prop[2]);
	    }
		
	    // 4. 检查布尔属性
	    if (of_property_read_bool(np, "dma-enabled")) {
	        printk("DMA is enabled\n");
	    }
		
	    return 0;
	}
	```
## 三、注册`platform`驱动
- **`struct platform_driver`**
	![[platform_driver结构体.png]]
	- `.driver`
		- `.name`：用于匹配（未使用设备树时使用）
		- `.owner`
		- `.of_match_table`：==用于匹配（使用设备树时使用）==
	- `.id_table`：用于匹配
	- 匹配的优先级
		`name`<`id_table`<`of_match_table`
- **加载与卸载函数**
	``` C
	//不建议使用
	int platform_driver_register(struct platform_driver *driver)
	void platform_driver_unregister(struct platform_driver *driver)
	
	//非常建议使用
	module_platform_driver(struct platform_driver *driver)
	```
- **获取资源函数**
	- 通用函数
		``` C
		struct resource *platform_get_resource(
			struct platform_device *dev, unsigned int type, unsigned int num
		)
		```
		- 参数
			- `type`：资源类型
				`IORESOURCE_MEM`
				`IORESOURCE_IRQ`
			- `num`：资源索引
	- ==专门获取中断号的便捷函数==
		``` C
		int platform_get_irq(struct platform_device *dev, unsigned int num)
		```
		- 返回值
			正值（中断号）
			负值（错误码）
- **私有数据**
	- 设置驱动私有数据
		``` C
		void platform_set_drvdata(struct platform_device *pdev, void *data)
		```
	- 获取驱动私有数据
		``` C
		void *platform_get_drvdata(struct platform_device *pdev)
		```
	- 例
		``` C
		struct my_private_data {
		    struct cdev cdev;
		    dev_t devno;
		    struct device *dev;
		    // 其他设备特定数据...
		};
		
		static int my_open(struct inode *inode, struct file *filp) {
		    struct my_driver_data *priv = container_of(inode->i_cdev, 
		                                             struct my_driver_data, cdev);
		    filp->private_data = priv;
		    return 0;
		}
		
		static ssize_t my_read(struct file *filp, char __user *buf, 
								size_t count, loff_t *f_pos) {
		    struct my_driver_data *priv = filp->private_data;
		    ...
		}
		
		static int my_driver_probe(struct platform_device *pdev) {
		    struct my_private_data *priv;
		    int ret;
		    
		    // 1. 分配私有数据结构
		    priv = devm_kzalloc(&pdev->dev, sizeof(struct my_private_data), GFP_KERNEL);
		    if (!priv) {
		        return -ENOMEM;
		    }
		    
		    // 2. 初始化私有数据
		    priv->dev = &pdev->dev;
		    
		    // 4. 设置驱动数据（关键步骤）
		    platform_set_drvdata(pdev, priv);
		    
		    
		    dev_info(&pdev->dev, "Device probed successfully\n");
		    return 0;
		}
		```
- **例**
	``` C
	#include <linux/module.h>
	#include <linux/platform_device.h>
	#include <linux/io.h>
	
	static int my_probe(struct platform_device *pdev) {
		
	    return 0;
	}
	
	static void my_remove(struct platform_device *pdev) {
		  
	}
	
	static const struct platform_device_id my_platform_ids[] = {
	    {"my_platform_device"},
	    { }
	};
	MODULE_DEVICE_TABLE(platform, my_platform_ids);
	
	static const struct of_device_id my_of_match[] = {
	    { .compatible = "my-platform-device" },
	    { }
	};
	MODULE_DEVICE_TABLE(of, my_of_match);//可写可不写
	
	static struct platform_driver my_driver = {
	    .probe = my_probe,
	    .remove = my_remove,
	    .driver = {
	        .name = "my-platform-device",
	        .of_match_table = my_of_match,
	        .owner = THIS_MODULE,
	    },
	    .id_table = my_platform_ids,
	};
	
	module_platform_driver(my_driver);//替换了那两行
	
	MODULE_LICENSE("GPL");
	```
# §3.9：`SPI`子系统框架
## 一、device部分
1. 在**芯片**的设备树文件中寻找`spi`控制器的节点![[Pasted image 20250921114620.png]]
2. 如果`spi`控制器的引脚复用等需要修改，那么可以在**开发板**的设备树文件中以追加的形式修改
3. 在**开发板**的设备树文件中，以追加的形式添加`spi`设备
	``` dts
	&spi0 {
		status = "okay";
		
		spi-adc:spi-adc@0 {
			compatible = "spi,adc";
			reg = <0>;//表示片选
			spi-max-frequency = <24000000>//非常重要
			spi-cpha;//设置工作模式
			spi-cpol;//设置工作模式
			spi-lsb-first;//低位优先
			spi-cs-high;//高电平片选
			
			status = "okay";
		};
	};
	```
## 二、driver部分
1. **spi_driver结构体：**
	``` C
	struct spi_driver {
		const struct spi_device_id *id_table;
		int (*probe)(struct spi_device *spi);
		void (*remove)(struct spi_device *spi);
		void (*shutdown)(struct spi_device *spi);
		struct device_driver driver;
	};
	```
2. **spi_device结构体：**
	``` C
	struct spi_device {
		...
		u32 max_speed_hz;
		u32 mode;
		struct device dev;
		int irq;
		...
	};
	```
- **例**
	``` C
	#include <linux/init.h>
	#include <linux/module.h>
	#include <linux/spi/spi.h>
	#include <linux/fs.h>
	#include <linux/kdev_t.h>
	
	static int my_probe(struct spi_device *spi) {
		
		return 0;
	}
	static void my_spi_remove(struct spi_device *spi) {
		
	}
	
	static const struct spi_device_id my_id_table[] = {
		{"my_device"},
		{ }
	};
	MODULE_DEVICE_TABLE(spi, my_id_table);
	
	static const struct of_device_id my_of_match_table[] = {
		{.compatible = "my_device"},
		{ }
	};
	MODULE_DEVICE_TABLE(of, my_of_match_table);
	
	static struct spi_driver spi_adc = {
		.probe = my_probe;
		.remove = my_remove;
		.driver = {
			.name = "my_device";
			.owner = THIS_MODULE;
			.of_match_table = my_of_match_table;			
		};
		.id_table = my_id_table;
	};
	module_spi_driver(spi_adc);
	
	MOUDLE_LICENCE("GPL");
	```
## 三、信息收发
- **封装好的版本**
	``` C
	static inline int spi_write(
		struct spi_device *spi, const void *buf, size_t len
	)
	static inline int spi_read(
		struct spi_device *spi, void *buf, size_t len
	)
	int spi_write_then_read(
		struct spi_device *spi, const void *txbuf, 
		unsigned n_tx, void *rxbuf, unsigned n_rx
	)
	```
- **底层版本**
	``` C
	struct spi_transfer {
		...
		const void *tx_buf;
		void *rx_buf;
		unsigned len;
		unsigned cs_change;
		...
	};
	static inline int spi_sync_transfer(
		struct spi_device *spi, struct spi_transfer *xfers,
		unsigned int num_xfers
	)
	```

# `I2C`子系统框架
## 一、device部分
1. 在**芯片**的设备树文件中寻找`i2c`控制器的节点![[Pasted image 20251003230930.png]]
2. 如果`i2c`控制器的引脚复用等需要修改，那么可以在**开发板**的设备树文件中以追加的形式修改
3. 在**开发板**的设备树文件中，以追加的形式添加`i2c`设备
	``` dts
		&i2c0 {
			status = "okay";
			
			i2c-adc:i2c-adc@0 {
				compatible = "i2c,adc";
				status = "okay";
			};
		};
	```
## 二、driver部分
1. **`i2c_driver`结构体：**
	``` C
	struct i2c_driver {
		unsigned int class;
		
		/* Standard driver model interfaces */
		int (*probe)(struct i2c_client *client, const struct i2c_device_id *id);
		void (*remove)(struct i2c_client *client);
		  
		/* New driver model interface to aid the seamless removal of the
		* current probe()'s, more commonly unused than used second parameter.
		*/
		int (*probe_new)(struct i2c_client *client);
		
		/* driver model interfaces that don't relate to enumeration */
		void (*shutdown)(struct i2c_client *client);
		
		/* Alert callback, for example for the SMBus alert protocol.
		* The format and meaning of the data value depends on the protocol.
		* For the SMBus alert protocol, there is a single bit of data passed
		* as the alert response's low bit ("event flag").
		* For the SMBus Host Notify protocol, the data corresponds to the
		* 16-bit payload data reported by the slave device acting as master.
		*/
		void (*alert)(struct i2c_client *client, enum i2c_alert_protocol protocol,
		unsigned int data);
		  
		/* a ioctl like command that can be used to perform specific functions
		* with the device.
		*/
		int (*command)(struct i2c_client *client, unsigned int cmd, void *arg);
		
		struct device_driver driver;
		const struct i2c_device_id *id_table;
		
		/* Device detection callback for automatic device creation */
		int (*detect)(struct i2c_client *client, struct i2c_board_info *info);
		const unsigned short *address_list;
		struct list_head clients;
		  
		u32 flags;
	};
	```
2. **`i2c_client`结构体：**
	``` C
	struct i2c_client {
		...
		unsigned short addr;//从机地址
		struct device dev;
		struct i2c_adapter *adapter;
		int init_irq;
		int irq;
		...
	};
	```
- 例
	``` C
	#include <linux/init.h>
	#include <linux/module.h>
	#include <linux/i2c/i2c.h>
	#include <linux/cdev.h>
	#include <linux/fs.h>
	#include <linux/kdev_t.h>
	
	static int my_probe(struct i2c_client *client, const struct i2c_device_id *id) {
		return 0;
	}
	static void myremove(struct i2c_client *client) {
		
	}
	
	static const struct i2c_device_id my_id_table[] = {
		{"my_device"},
		{ }
	};
	MODULE_DEVICE_TABLE(i2c, my_id_table);
	
	static const struct of_device_id my_of_match_table[] = {
		{.compatible = "my_device"},
		{ }
	};
	MODULE_DEVICE_TABLE(of, my_of_match_table);
	
	static struct i2c_driver my_i2c_driver = {
		.probe = my_probe,
		.remove = my_remove,
		.driver = {
			.name = "my_device",
			.owner = THIS_MODULE,
			.of_match_table = my_of_match_table,		
		},
		.id_table = my_id_table,
	};
	module_i2c_driver(my_i2c_driver);
	
	MOUDLE_LICENCE("GPL");
	```
## 三、信息收发
- 封装好的版本
	``` C
	s32 i2c_smbus_read_byte(struct i2c_client *client)
	s32 i2c_smbus_write_byte(struct i2c_client *client, u8 value)
	s32 i2c_smbus_read_byte_data(struct i2c_client *client, u8 command)
	s32 i2c_smbus_write_byte_data(struct i2c_client *client, u8 command, u8 value)
	s32 i2c_smbus_read_word_data(struct i2c_client *client, u8 command)
	s32 i2c_smbus_write_word_data(struct i2c_client *client, u8 command, u16 value)
	```
- 底层版本
	``` C
	struct i2c_msg {
		__u16 addr;//从机地址
		__u16 flags;//flags = 0时，表示发；flags = I2C_M_RD时，表示收
		__u16 len;
		__u8 *buf;
	};
	int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
	```
- 命令行操作
	``` bash
	#探测设备
	
	
	
	
	# 解绑设备
	echo "6-002e" > /sys/bus/i2c/drivers/max96724_v2/unbind
	
	向寄存器中读值：i2ctransfer -y 6 w2@0x2e 0x00 0x0D r1
	向寄存器中写值：i2ctransfer -y 6 w3@0x2e 0x00 0x0D 0x55
	```
# input子系统
- Linux专门为输入类设备编写的一个子系统（框架）， 从而规范驱动开发，降低开发难度，提高驱动的通用性与兼容性，具有以下特点：
	- 兼容所有输入设备
	- 统一的驱动编程方式
	- 统一的应用操作接口：`dev/input`
- **确认设备与节点的对应关系**
	- 方法一
		``` bash
		cat /dev/input/xxx
		```
		该目录下会显示通用设备名与专属设备名
	- 方法二
		``` bash
		cat /proc/bus/inpit/devices
		```
- **input子系统的框架**
	事件处理层：`struct input_handler`
	input核心层：`struct input_handle`
	设备驱动层：`struct input_dev`
- `input_dev`结构体
	``` C
	struct input_dev {
		unsigned long evbit[BITS_TO_LONGS(EV_CNT)];
		unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];
		unsigned long relbit[BITS_TO_LONGS(REL_CNT)];
		unsigned long absbit[BITS_TO_LONGS(ABS_CNT)];
		unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)];
		unsigned long ledbit[BITS_TO_LONGS(LED_CNT)];
		unsigned long sndbit[BITS_TO_LONGS(SND_CNT)];
		unsigned long ffbit[BITS_TO_LONGS(FF_CNT)];
		unsigned long swbit[BITS_TO_LONGS(SW_CNT)];
	};
	```
	`evbit`
		`EV_SYN 0x00`：同步事件，表示一个事件结束
		`EV_KEY 0x01`：按键事件，表示按下，释放或重复yigejian
		`EV_REL 0x02`
		`EV_ABS 0x03`
		`EV_MSC 0x04`
		`EV_SW 0x05`
		`EV_LED 0x11`
		`EV_SND 0x12`
		`EV_REP 0x14`
		`EV_FF 0x15`
		`EV_PWR 0x16`
		`EV_FF_STATUS 0x17`
## 二、注册流程
1. **分配输入设备**
	``` C
	struct input_dev *input_allocate_device(void)
	struct input_dev *devm_input_allocate_device(struct device *dev)
	```
- **例**
	``` C
	struct input_dev *my_input_dev;
	
	my_input_dev = input_allocate_device();
	
	my_input_dev->name = "my_dev";
	__set_bit(EV_KEY, my_input_dev->evbit);
	__set_bit(KET_1, my_input_dev->keybit);
	
	input_register_device(my_input_dev);
	```
# `PWM`子系统
## 一、device部分
- 在**芯片**的设备树文件中寻找`PWM`控制器的节点![[Pasted image 20251004152824.png]]
- 如果`PWM`控制器的引脚复用等需要修改，那么可以在**开发板**的设备树文件中以追加的形式修改
	```
	&pwm2_8ch_6 {
		status = "okay";
	};
	```
- 在**开发板**的设备树文件的**根节点**中添加节点
	格式：`pwms = <&pwm节点 占空比 周期(单位：ns) 极性(1或0)>`
	``` dts
	/ {
		...
		pwm-device {
			compatible = "pwm-device";
			pwms = <&pwm2_8ch_6 0 500000 0>;
		};
		...
	};
	```
## 二、driver部分
- **`pwm-device`结构体**
	``` C
	struct pwm_device {
		const char *label;
		unsigned long flags;
		unsigned int hwpwm;
		unsigned int pwm;
		struct pwm_chip *chip;
		void *chip_data;
		
		struct pwm_args args;
		struct pwm_state state;
		struct pwm_state last;
	};
	```
- **从设备树中获得`pwm`设备**
	``` C
	struct pwm_device *devm_pwm_get(
		struct device *dev, const char *con_id
	)
	struct pwm_device *devm_pwm_get_optional(
		struct device *dev, const char *con_id
	)
	```
	- `con_id`
		- 设备树端为`pwms = <&pwm2_8ch_6 0 500000 0>`，应为`NULL`
		- 设备树端为
			```  dts
			pwms = <&pwm2_8ch_6 0 500000 0>;
			pwm-names = "name0";
			```
			应为`"name0"`
- **例**
	``` C
	#include <linux/module.h>
	#include <linux/platform_device.h>
	#include <linux/io.h>
	
	static struct cdev chrdev;
	static dev_t dev_num;
	struct class *my_class;
	
	static struct pwm_device *pwm_device;
	
	static struct file_operations fops = {
		.owner = THIS_MODULE,
		.open = cdev_open,
		.read = cdev_read,
		.write = cdev_write,
		.release = cdev_release
	};
	
	static int my_probe(struct platform_device *pdev) {
		
		int ret;
		ret = alloc_chrdev_region(&dev_num, 0, 1, "pwm");
		cdev_init(&chrdev, &fops);
		cdev_add(&chrdev, dev_num, 1);
		
		my_class = class_create(THIS_MODULE, "my_device_class");
		device_create(my_class, NULL, dev_num, NULL, "my_device");
		
		pwm_device = devm_pwm_get(pdev->dev, NULL);
		
		return 0;
	}
	
	static void my_remove(struct platform_device *pdev) {
		  
	}
	
	
	static const struct platform_device_id my_id_table[] = {
		{"my_device"},
		{ }
	};
	MODULE_DEVICE_TABLE(platform, my_id_table);
	
	static const struct of_device_id my_of_match[] = {
		{ .compatible = "my-platform-device" },
		{ }
	};
	MODULE_DEVICE_TABLE(of, my_of_match);//可写可不写
	
	static struct platform_driver my_driver = {
		.probe = my_probe,
		.remove = my_remove,
		.driver = {
			.name = "my-platform-device",
			.of_match_table = my_of_match,
			.owner = THIS_MODULE,
		},
		.id_table = my_id_table
	};
	
	module_platform_driver(my_driver);//替换了那两行
	
	MODULE_LICENSE("GPL");
	```
## 三、PWM配置
``` C
struct pwm_state {
    u64 period;                    // 周期（纳秒）
    u64 duty_cycle;               // 占空比时间（纳秒）
    enum pwm_polarity polarity;   // 极性
    bool enabled;                 // 是否启用
};
// 极性定义
enum pwm_polarity {
    PWM_POLARITY_NORMAL,    // 正常极性
    PWM_POLARITY_INVERSED,  // 反转极性
};
//PWM 配置方式
int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state)
int pwm_apply_might_sleep(struct pwm_device *pwm, const struct pwm_state *state)

// 启用/禁用
int pwm_enable(struct pwm_device *pwm)
void pwm_disable(struct pwm_device *pwm)

// 获取当前状态
void pwm_get_state(const struct pwm_device *pwm, struct pwm_state *state)
```
# ADC子系统
## 一、device部分
- 在**芯片**的设备树文件中寻找`ADC`控制器的节点![[Pasted image 20251004222703.png]]
- 如果`ADC`控制器的引脚复用等需要修改，那么可以在**开发板**的设备树文件中以追加的形式修改
	``` dts
	&saradc {
		status = "okay";
	};
	```
- 在**开发板**的设备树文件的**根节点**中添加节点
	``` dts
	/ {
		...
		my-adc:my-adc {
			compatible = "my-adc";
			io-channels = <&saradc 1>;
		};
		...
	};
	```
## 二、driver部分
- **从设备树中获得`adc`设备**
	``` C
	struct iio_channel *devm_iio_channel_get(
		struct device *dev,
		const char *consumer_channel
	);
	```
	- `consumer_channel`
		- 设备树端为`io-channels = <&saradc 1>`，应为`NULL`
		- 设备树端为
			```  dts
			io-channels = <&saradc 0>, <&saradc 1>;
			io-channel-names = "name0", "name1";
			```
			应为`"name0"或"name1"`
- **例**
	``` C
	#include <linux/module.h>
	#include <linux/platform_device.h>
	#include <linux/fs.h>
	#include <linux/iio/consumer.h>
	#include <linux/cdev.h>
	
	struct iio_channel *my-adc;
	static struct cdev chrdev;
	static dev_t dev_num;
	struct class *my_class;
	
	long adc_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
		
		int value;
		
		switch (cmd) {
			case CMD_READ_RAW: {
				iio_read_channel_raw(my-adc, &value);
				copy_to_user((int *)arg, &value, sizeof(value));
				break;
			}
			default:
				break;
		}		
	}
	
	static struct file_operations fops = {
		.owner = THIS_MODULE,
		.open = cdev_open,
		.unlocked_ioctl = adc_dev_ioctl,
	};
	
	static int my_probe(struct platform_device *pdev) {
		
		my-adc = devm_iio_channel_get(pdev->dev, NULL);
		alloc_chrdev_region(&dev_num, 0, 1, "my_device");
		
		cdev_init(&chrdev, &fops);
		cdev_add(&chrdev, dev_num, 1);
		
		my_class = class_create(THIS_MODULE, "my_device_class");
		device_create(my_class, NULL, dev_num, NULL, "my_device");
		
	    return 0;
	}
	
	static void my_remove(struct platform_device *pdev) {
		  
	}
	
	static const struct platform_device_id my_platform_ids[] = {
	    {"my_platform_device"},
	    { }
	};
	MODULE_DEVICE_TABLE(platform, my_platform_ids);
	
	static const struct of_device_id my_of_match[] = {
	    { .compatible = "my-adc" },
	    { }
	};
	MODULE_DEVICE_TABLE(of, my_of_match);
	
	static struct platform_driver my_driver = {
	    .probe = my_probe,
	    .remove = my_remove,
	    .driver = {
	        .name = "my-adc",
	        .of_match_table = my_of_match,
	        .owner = THIS_MODULE,
	    },
	    .id_table = my_platform_ids,
	};
	
	module_platform_driver(my_driver);//替换了那两行
	
	MODULE_LICENSE("GPL");
	```
# MMC子系统
`MMC` 是 **MultiMediaCard** 的缩写，中文是**多媒体卡**。它是一个广泛使用的存储卡标准。
- **SD 卡 / TF 卡**
	``` dts
	&mmc {
	    // SD 卡配置
	    bus-width = <4>;
	    cap-sd-highspeed;
	    sd-uhs-sdr104;
	    cd-gpios = <&gpio0 RK_PA4 GPIO_ACTIVE_LOW>;  // 卡检测
	    status = "okay";
	};
	```
- **eMMC 存储**
	``` dts
	&mmc {
	    // eMMC 配置
	    bus-width = <8>;
	    cap-mmc-highspeed;
	    mmc-hs200-1_8v;
	    non-removable;      // 嵌入式，不可移除
	    no-sd;             // 禁用 SD 卡
	    no-sdio;           // 禁用 SDIO
	    status = "okay";
	};
	```
- **SDIO 设备（如 WiFi 模块）**
	``` dts
	&mmc {
	    // SDIO WiFi 配置
	    bus-width = <4>;
	    cap-sd-highspeed;
	    no-sd;             // 禁用 SD 卡
	    no-mmc;            // 禁用 MMC
	    non-removable;     // 不可移除
	    mmc-pwrseq = <&sdio_pwrseq>;  // 专用电源序列
	    sd-uhs-sdr104;
	    status = "okay";
	};
	```
# V4L2子系统
是 Linux 内核中针对视频捕获设备的一套驱动框架和应用程序接口。它允许用户空间的应用程序（比如 Skype, OBS, 简单的摄像头预览程序等）以统一的方式与各种不同的视频设备进行通信，而无需关心设备的具体型号和制造商。
## 一、核心概念
在Linux中，一切皆文件。V4L2设备通常在 `/dev/` 目录下表现为设备文件，例如：
- `/dev/video0`： 第一个视频设备
- `/dev/video1`： 第二个视频设备
- `/dev/videoN`： 第N+1个视频设备
应用程序通过打开（`open`）这些设备文件，然后使用 `ioctl` 系统调用来进行控制和数据交换
## 二、V4L2 应用程序开发流程
1. **打开设备**
	``` c
	int fd = open("/dev/video0", O_RDWR);
	if (fd == -1) {
	    // 错误处理
	}
	```
2. **查询设备能力**
	``` c
	struct v4l2_capability cap;
	ioctl(fd, VIDIOC_QUERYCAP, &cap);
	```
3. **设置采集格式**
	``` c
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // 或 V4L2_PIX_FMT_YUYV
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	ioctl(fd, VIDIOC_S_FMT, &fmt);
	```
4. **申请缓冲区**
	``` c
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = 4; // 申请4个缓冲区
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_REQBUFS, &req);
	```
5. **查询内存映射缓冲区**
	``` c
	struct v4l2_buffer buf;
	for (int i = 0; i < req.count; i++) {
	    memset(&buf, 0, sizeof(buf));
	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    buf.memory = V4L2_MEMORY_MMAP;
	    buf.index = i;
	    ioctl(fd, VIDIOC_QUERYBUF, &buf); // 查询缓冲区信息
		
	    // 内存映射
	    void* buffer_start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
	    
	    // 将缓冲区放入驱动队列
	    ioctl(fd, VIDIOC_QBUF, &buf);
	}
	```
6. **开始采集**
	``` c
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMON, &type);
	```
7. **主循环：获取帧**
	``` c
	while (1) {
	    fd_set fds;
	    // ... 使用 select 或 poll 等待设备数据可读 ...
		
	    // 从驱动队列中取出一个已填充数据的缓冲区
	    struct v4l2_buffer buf;
	    memset(&buf, 0, sizeof(buf));
	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    buf.memory = V4L2_MEMORY_MMAP;
	    ioctl(fd, VIDIOC_DQBUF, &buf); // 出队
		
	    // 此时，buffer_start[buf.index] 指向的数据就是一帧图像
	    // 处理图像数据 (e.g., process_image(buffer_start[buf.index], buf.bytesused);)
		
	    // 处理完后，将缓冲区重新放回驱动队列，以便驱动再次使用它采集数据
	    ioctl(fd, VIDIOC_QBUF, &buf);
	}
	```
## 三、V4L2 驱动开发
1. probe函数
	Probe函数中，首先对dts进行解析，获取regulator, gpio, clk等信息以对sensor上下电，其次注册media entity, v4l2 controller信息，注意到v4l2 subdev的注册是异步的，如以下几个关键函数调用
	- v4l2_i2c_subdev_init();注册一个subdev，参数中提供回调函数
	- imx415_initialize_controls();初始化v4l2 controls
	- media_entity_init();注册成为一个media entity， im415仅有一个输出，即source pad
	- v4l2_async_register_subdev()，声明sensor需要异步注册，因为rkisp1及cif都采用异步注册sub device， 所以这个调用是必须的
	1. 定义结构体struct v4l2_subdev等
	2. 解析设备树信息
	3. 初始化并注册vcl2_subdev结构体
## 关键文件节点
- **`/dev/video*`**：`Video`设备节点
	- **用途**：实际的视频捕获/输出设备
	- **例子**：
	    `/dev/video0` - 摄像头捕获设备
	    `/dev/video1` - `ISP`输出设备
	    `/dev/video2` - 编码器/解码器
	- **操作**：==可以直接打开==、读取视频帧、设置格式等
	- **应用**：用户空间程序（如`ffmpeg`、`gstreamer`）直接操作这些节点获取视频数据
	- 注册`api`
		==`rkisp_register_platform_subdevs`==
	``` bash
	# 查看video设备信息
	v4l2-ctl --list-devices
	v4l2-ctl -d /dev/video0 --all
	```
- **`/dev/v4l-subdev*`** - `V4L2`子设备节点
	- **用途**：独立的==硬件组件==（通常是`ISP`管道的一部分）
	- **功能**：配置单个硬件模块（`sensor`、`lens`、`ISP`模块等）
	- **例子**：
	    `/dev/v4l-subdev0` - 摄像头`sensor`
	    `/dev/v4l-subdev1` - `VCM`对焦马达（如`dw9714`）
	    `/dev/v4l-subdev2` - `ISP`处理单元
	- **操作**：设置格式、裁剪、控制参数等
	- **特点**：不直接产生数据流，而是配置管道中的组件
	- 注册`api`
		==`v4l2_device_register_subdev_nodes`==
	``` bash
	# 查看subdev信息
	v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls
	media-ctl --print-dot
	```
- **`/dev/media*`** - `Media`控制器设备
	- **用途**：管理整个媒体管道的==拓扑结构==
	- **功能**：描述和配置`video`设备、`subdev`之间的连接关系
	- **例子**：
	    `/dev/media0` (或 media1, media2... 取决于系统中已有的 media 设备数量)
	- **操作**：查看设备拓扑、建立链接、配置数据流路径
	- **重要性**：在复杂的`ISP`系统中，需要通过`media`设备来配置完整的数据流管道
	- 注册`api`
		- `media_device_init`
		- ==`media_device_register`==
		- `media_device_unregister`
		- `media_device_cleanup`
	- 查看拓扑
		``` bash
		media-ctl -d /dev/media0 -p
		```
- **`/sys/class/video4linux/`** - `sysfs`接口
	- **用途**：系统文件系统中的==设备信息==
	- **功能**：提供设备的元数据和属性
	- **内容**：包含所有`video`和`v4l-subdev`设备的链接
	``` bash
	# 查看所有V4L设备
	ls -l /sys/class/video4linux/
	# 输出示例:
	# video0 -> ../../devices/.../video0
	# video1 -> ../../devices/.../video1
	# v4l-subdev0 -> ../../devices/.../v4l-subdev0
	```
- **`/sys/class/video4linux/v4l-subdev*/`**-subdev的`sysfs`信息
	- **用途**：子设备的详细信息
	- **功能**：查看子设备名称、索引等属性
	``` bash
	# 查看子设备名称
	cat /sys/class/video4linux/v4l-subdev0/name
	# 可能输出: ov5647 3-0036 或 dw9714 3-000c
	```
# 固定电压电源(regulator)
- 例
	``` dts
	vcc5v0_usb_port1: vcc5v0-usb-port1 {
	    compatible = "regulator-fixed";
	    regulator-name = "vcc5v0_usb_port1";
	    regulator-min-microvolt = <5000000>;
	    regulator-max-microvolt = <5000000>;
	    gpio = <&gpio2 RK_PC2 GPIO_ACTIVE_HIGH>;
	    enable-active-high;
	};
	```
### MAX96724
1. 引脚
	![[Pasted image 20251105203608.png]]
	- GMSL2串行链路引脚
		SIOAP/SIOAN (Pin 5/6): 串行数据I/O正负对
		SIOCP/SIOCN (Pin 10/9): 串行数据I/O正负对
		SIOBP/SIOBN (Pin 18/19): 串行数据I/O正负对
		SIOAP/SIOAN (Pin 23/22): 串行数据I/O正负对
	- CSI-2输出接口
		- 端口A（4通道）
			DA0P/DA0N (Pin 31/32): CSI-2数据通道0正负对
			DA1P/DA1N (Pin 35/36): CSI-2数据通道1正负对
			DA2P/DA2N (Pin 37/38): CSI-2数据通道2正负对
			DA3P/DA3N (Pin 41/42): CSI-2数据通道3正负对
			CKAP/CKAN (Pin 39/40): CSI-2时钟通道正负对
		- 端口B（4通道）
			DB0P/DB0N (Pin 43/44): CSI-2数据通道0正负对
			DB1P/DB1N (Pin 47/48): CSI-2数据通道1正负对
			DB2P/DB2N (Pin 49/50): CSI-2数据通道2正负对
			DB3P/DB3N (Pin 53/54): CSI-2数据通道3正负对
			CKBP/CKBN (Pin 45/46): CSI-2时钟通道正负对
	控制和配置引脚
		I2C接口:
			SDA (Pin 28): I2C数据线
			SCL (Pin 29): I2C时钟线
复位和使能:
XRES (Pin 17): 硬件复位（低电平有效）
PWDNB (Pin 11): 功耗控制（低电平进入掉电模式）
时钟相关:
X1/X2 (Pin 15/16): 25MHz晶振连接
	GPIO和多功能引脚
	电源引脚
	特殊功能引脚
2. 内部原理图
	![[Pasted image 20251105203712.png]]
	![[Pasted image 20251105203905.png]]
``` bash
python3 -m http.server 8080 --directory /path/to/your/directory

# 设置视频格式
v4l2-ctl -d /dev/v --set-fmt-subdev-fmt pad=0,which=active,format=SBGGR10/1920x1080



# 检查日志,应该看到:
# "Streaming started (colorbar pattern active)"
# 持续的帧采集,没有 "frame end is stopped" 错误
dmesg | tail -50

# 拍摄一张 RAW 格式照片
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=BG10 --stream-mmap --stream-count=1 --stream-to=photo.raw

# 解绑设备
echo "6-0027" > /sys/bus/i2c/drivers/max96724_v2/unbind

向寄存器中读值：i2ctransfer -y 6 w2@0x27 0x00 0x0D r1
向寄存器中写值：i2ctransfer -y 6 w3@0x27 0x00 0x0D 0x55

echo 493 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio493/direction
echo 1 > /sys/class/gpio/gpio493/value

cat /sys/kernel/debug/regmap/6-0027/registers
# 写单个寄存器
echo "0x1051 0x10" > registers
# 读寄存器
cat registers | grep 1051

media-ctl -d /dev/media0 -p

dmesg | grep "max967"


	向寄存器中读值：i2ctransfer -y 6 w2@0x27 0x00 0x0D r1
	向寄存器中写值：i2ctransfer -y 6 w3@0x27 0x00 0x0D 0x55
	
	
i2ctransfer -f -y 6 w3@0x27 0x00 0x13 0x75

i2ctransfer -f -y 6 w3@0x27 0x08 0xa0 0x04
i2ctransfer -f -y 6 w3@0x27 0x04 0x0b 0x00
i2ctransfer -f -y 6 w3@0x27 0x08 0xa0 0x04
i2ctransfer -f -y 6 w3@0x27 0x09 0x4a 0xc0
i2ctransfer -f -y 6 w3@0x27 0x08 0xa3 0xe4
i2ctransfer -f -y 6 w3@0x27 0x08 0xa5 0x00
i2ctransfer -f -y 6 w3@0x27 0x04 0x15 0x3d
i2ctransfer -f -y 6 w3@0x27 0x04 0x18 0x3d
i2ctransfer -f -y 6 w3@0x27 0x08 0xa2 0x34
i2ctransfer -f -y 6 w3@0x27 0x10 0x52 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x53 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x54 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x55 0x01
i2ctransfer -f -y 6 w3@0x27 0x10 0x56 0x7a
i2ctransfer -f -y 6 w3@0x27 0x10 0x57 0x20
i2ctransfer -f -y 6 w3@0x27 0x10 0x58 0x25
i2ctransfer -f -y 6 w3@0x27 0x10 0x59 0x9a
i2ctransfer -f -y 6 w3@0x27 0x10 0x5a 0x80
i2ctransfer -f -y 6 w3@0x27 0x10 0x5b 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x5c 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x5d 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x5e 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x5f 0x2c
i2ctransfer -f -y 6 w3@0x27 0x10 0x60 0x08
i2ctransfer -f -y 6 w3@0x27 0x10 0x61 0x6c
i2ctransfer -f -y 6 w3@0x27 0x10 0x62 0x04
i2ctransfer -f -y 6 w3@0x27 0x10 0x63 0x65
i2ctransfer -f -y 6 w3@0x27 0x10 0x64 0x01
i2ctransfer -f -y 6 w3@0x27 0x10 0x65 0x61
i2ctransfer -f -y 6 w3@0x27 0x10 0x66 0x18
i2ctransfer -f -y 6 w3@0x27 0x10 0x67 0x07
i2ctransfer -f -y 6 w3@0x27 0x10 0x68 0x80
i2ctransfer -f -y 6 w3@0x27 0x10 0x69 0x01
i2ctransfer -f -y 6 w3@0x27 0x10 0x6a 0x18
i2ctransfer -f -y 6 w3@0x27 0x10 0x6b 0x04
i2ctransfer -f -y 6 w3@0x27 0x10 0x6c 0x38
i2ctransfer -f -y 6 w3@0x27 0x10 0x50 0xfb
i2ctransfer -f -y 6 w3@0x27 0x10 0x74 0x3c
i2ctransfer -f -y 6 w3@0x27 0x10 0x75 0x3c
i2ctransfer -f -y 6 w3@0x27 0x10 0x76 0x3c
i2ctransfer -f -y 6 w3@0x27 0x10 0x6e 0xfe
i2ctransfer -f -y 6 w3@0x27 0x10 0x6f 0xcc
i2ctransfer -f -y 6 w3@0x27 0x10 0x70 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x71 0x00
i2ctransfer -f -y 6 w3@0x27 0x10 0x72 0x6a
i2ctransfer -f -y 6 w3@0x27 0x10 0x73 0xa7
i2ctransfer -f -y 6 w3@0x27 0x10 0x51 0x10
i2ctransfer -f -y 6 w3@0x27 0x04 0x0b 0x02
i2ctransfer -f -y 6 w3@0x27 0x08 0xa0 0x84
	
	
	
	
	
	
	# 刷新固件后测试
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=RGB3
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/tmp/frame.yuv
	
# 1. 检查设备
v4l2-ctl -d /dev/video0 --list-formats-ext
# 2. 配置格式
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=RGB3
# 3. 查看当前配置
v4l2-ctl -d /dev/video0 --get-fmt-video
# 4. 采集一张照片
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/tmp/test.raw
# 5. 查看文件大小（应该约为 1920*1080*2 = 4MB）
ls -lh /tmp/test.raw
```

### RK3576的mipi csi
- **csi2_dcphy0**(used)
	- 依赖
		rockchip,hw = <&csi2_dphy0_hw>, <&csi2_dphy1_hw>
		phys = <&mipidcphy0>
- **csi2_dphy0**
- **csi2_dphy1**(used)
- **csi2_dphy2**(used)
- **csi2_dphy3**(used)
- **csi2_dphy4**(used)
- **csi2_dphy5**(used)
```
MIPI 摄像头
    ↓ (MIPI D-PHY 差分信号)
mipidcphy0 (phys)          ← 电气信号 → 数字信号转换
    ↓ (并行数据)
csi2_dphy0_hw (rockchip,hw[0])  ← CSI-2 协议处理、lane0~lane3
csi2_dphy1_hw (rockchip,hw[1])  ← CSI-2 协议处理、lane4~lane7
    ↓ (标准化数据)
csi2_dcphy0/csi2_dphy0~csi2_dphy5
```

- **mipi0_csi2**(used)
	- 依赖
		mipi0_csi2_hw
		mipi1_csi2_hw
		mipi2_csi2_hw
		mipi3_csi2_hw
		mipi4_csi2_hw
- **mipi1_csi2**(used)
- **mipi2_csi2**(used)
- **mipi3_csi2**(used)
- **mipi4_csi2**(used)
```
	**MIPI CSI-2 Host 控制器负责**：
	
	1. **CSI-2 协议解析**
	    - 包头解析（SOT, EOT）
	    - 数据类型识别（RAW8/10/12, YUV422, RGB等）
	    - 虚拟通道（VC）分离
	    - 错误检测与校正（ECC, CRC）
	2. **数据处理**
	    - Lane 合并与数据对齐
	    - 时钟域转换（MIPI → AXI）
	    - FIFO 缓冲
	    - 帧同步（Frame Start/End）
	3. **中断管理**
	    - csi-intr1：帧开始/结束中断
	    - csi-intr2：错误中断（CRC错误、ECC错误等）
```

- **rkcif_dvp**
	rkcif
	rkcif_mmu
- **rkcif_dvp_sditf**
	rkcif_dvp

- **rkcif_mipi_lvds/rkcif_mipi_lvds0**(used)
	- **依赖**
		rkcif
	- **SDITF**
		**rkcif_mipi_lvds_sditf**(used)
		**rkcif_mipi_lvds_sditf_vir1**
		**rkcif_mipi_lvds_sditf_vir2**
		**rkcif_mipi_lvds_sditf_vir3**
- **rkcif_mipi_lvds1**(used)
	- **SDITF**
		**rkcif_mipi_lvds1_sditf**(used)
		**rkcif_mipi_lvds1_sditf_vir1**
		**rkcif_mipi_lvds1_sditf_vir2**
		**rkcif_mipi_lvds1_sditf_vir3**
- **rkcif_mipi_lvds2**(used)
	- **SDITF**
		**rkcif_mipi_lvds2_sditf**(used)
		**rkcif_mipi_lvds2_sditf_vir1**
		**rkcif_mipi_lvds2_sditf_vir2**
		**rkcif_mipi_lvds2_sditf_vir3**
- **rkcif_mipi_lvds3**(used)
	- **SDITF**
		**rkcif_mipi_lvds3_sditf**(used)
		**rkcif_mipi_lvds3_sditf_vir1**
		**rkcif_mipi_lvds3_sditf_vir2**
		**rkcif_mipi_lvds3_sditf_vir3**
- **rkcif_mipi_lvds4**(used)
	- **SDITF**
		**rkcif_mipi_lvds4_sditf**(used)
		**rkcif_mipi_lvds4_sditf_vir1**
		**rkcif_mipi_lvds4_sditf_vir2**
		**rkcif_mipi_lvds4_sditf_vir3**

- **rkisp_vir0**(used)
	**rkisp_vir0_sditf**
- **rkisp_vir1**(used)
- **rkisp_vir2**(used)
- **rkisp_vir3**(used)
- **rkisp_vir4**(used)
- **rkisp_vir5**(used)

- **rkisp_vir0_sditf**(used)
	rkisp_vir0
- **rkisp_vir1_sditf**(used)
	rkisp_vir1
- **rkisp_vir2_sditf**(used)
	rkisp_vir2
- **rkisp_vir3_sditf**(used)
	rkisp_vir3
- **rkisp_vir4_sditf**(used)
	rkisp_vir4
- **rkisp_vir5_sditf**(used)
	rkisp_vir5

- **rkvpss_vir0**(used)
	rkvpss
- **rkvpss_vir1**(used)
	rkvpss
- **rkvpss_vir2**(used)
	rkvpss
- **rkvpss_vir3**(used)
	rkvpss
- **rkvpss_vir4**(used)
	rkvpss
- **rkvpss_vir5**(used)
	rkvpss

- mipidcphy0_grf
- mipidphy0_grf
- mipidphy1_grf

- mipidcphy0
	mipidcphy0_grf

- **rkcif**
- **rkcif_mmu**

- **rkisp**
- **rkisp_mmu**

- **rkvpss**
- **rkvpss_mmu**

- mipi0_csi2_hw
- mipi1_csi2_hw
- mipi2_csi2_hw
- mipi3_csi2_hw
- mipi4_csi2_hw

- csi2_dphy0_hw
	mipidphy0_grf
- csi2_dphy1_hw
	mipidphy1_grf
# USB
## USB 
![[Pasted image 20251120221243.png]]
- **USB OTG 控制器**
	OTG 控制器有两个模式：正常模式(normal mode)和低功耗模式(low power mode)。 OTG 控制器都可以运行在高速模式(HS 480Mbps)、全速模式(LS 12Mbps)和低速模式(1.5Mbps)。正常模 式下每个 OTG 控制器都可以工作在主机(HOST)或从机(DEVICE)模式下，每个 USB 控制器都 有其对应的接口。
- **USB Host 控制器**
	USBH 控制器这是一个主机控制器，此控制器由 EHCI 控制器和 OHCI 控制器组成。USBH 控制器只能做主机模式。
	- **OHCI**：全称为 Open Host Controller Interface，这是一种 USB 控制器标准，厂商在设计 USB 控制器的时候需要遵循此标准，用于 USB1.1 标准。OHCI 不仅仅用于 USB，也支持一些其他的 接口，比如苹果的 Firewire 等，OHCI 由于硬件比较难，所以软件要求就降低了，软件相对来说 比较简单。OHCI 主要用于非 X86 的 USB，比如扩展卡、嵌入式 USB 控制器。
	- **EHCI**：全称是 Enhanced Host Controller Interface，是 Inter 主导的一个用于 USB2.0 的 USB 控制器标准。EHCI 仅提供 USB2.0 的高速功能，至于全速和低速功能就由 OHCI 或 UHCI 来提供。
	``` c
	# RK3568
	
	// USB 2.0 Host_2 EHCI controller for high speed
	usb_host0_ehci: usb@fd800000 {// USB2.0
		-->u2phy1_otg
	};
	
	// USB 2.0 Host_2 OHCI controller for full/low speed
	usb_host0_ohci: usb@fd840000 {// USB1.1
		-->u2phy1_otg
	};
	
	// USB 2.0 Host_3 EHCI controller for high speed
	usb_host1_ehci: usb@fd880000 {// USB2.0
		-->u2phy1_host
	};
	
	// USB 2.0 Host_3 OHCI controller for full/low speed
	usb_host1_ohci: usb@fd8c0000 {// USB1.1
		-->u2phy1_host
	};
	
	// USB 3.0 OTG_0 controller
	usbdrd30: usbdrd {// USB3.0
		usbdrd_dwc3: dwc3@fcc00000{
			-->u2phy0_otg
			-->combphy0_us
		};
	};
	
	// USB 3.0 Host_1 controller
	usbhost30: usbhost {
		usbhost_dwc3: dwc3@fd000000 {
			-->u2phy0_host
			-->combphy1_usq
		};
	};
	```
	使用 USB2.0 就要配置 usb_hostX_ehci(X=0,1) 节点
	使用 USB1.1 就要配置 usb_hostX_ohci (X=0,1)节点
	使用 USB3.0 就要配置 usbdrd30 节点。
	这两个节点的信息我们是 不需要修改的，这是 RK3568 一些通用配置信息。
- **USB HS PHY 控制器**
	``` c
	//  USB2 Comb PHY_0 port1/2
	usb2phy0: usb2-phy@fe8a0000 {
		u2phy0_host: host-port {
			-->usbhost_dwc3
		};
		u2phy0_otg: otg-port {
			-->usbdrd_dwc3
		};
	};
	
	usb2phy1: usb2-phy@fe8b0000 {
		// USB2 Comb PHY_1 port1
		u2phy1_host: host-port {
			-->usb_host1_ehci
			-->usb_host1_ohci
		};
		// USB2 Comb PHY_1 port0
		u2phy1_otg: otg-port {
		-->usb_host0_ehci
		-->usb_host0_ohci
		};
	};
	
	// USB3/SATA Combo PHY_0
	combphy0_us: phy@fe820000 {
		
	};
	
	// USB#/SATA/QSGMII Combo PHY_1
	combphy1_usq: phy@fe830000 {
		
	};
	```
	usb2phy1 和 usb2phy0 就是前面所说的 RK3568 的两个 USB2.0 PHY。每 个 PHY 控制器有两个端口。usb2phy1 节点里的两个子节点 usbphy1_host 和 usbphy1_otg 分别对应 usb_host1_X(X=ehci，ohci)和 usb_host0_X(X=ehci，ohci)节点
``` c
combphy0_us: phy@fe820000 {
compatible = "rockchip,rk3568-naneng-combphy";
reg = <0x0 0xfe820000 0x0 0x100>;
#phy-cells = <1>;
clocks = <&pmucru CLK_PCIEPHY0_REF>, <&cru PCLK_PIPEPHY0>,
```
## USB Typec
# 以太网
嵌入式网络硬件分为两部分：MAC 和 PHY。一般常见的通用 SoC 都会集成网络 MAC 外设内部的 MAC 外设会通过相应的接口来连接外部 PHY 芯片，根据数据传输模式不同，大致 可以分为以下两类：
- MII/RMII 接口：支持 10Mbit/s 和 100Mbit/s 数据传输模式
- GMII/RGMII 接口：支持 10Mbit/s、100Mbit/s 以及 1000Mbit/s 数据传输模式
一般把 MII/RMII 称为百兆以太网接口，而把 GMII/RGMII 称 为千兆以太网接口![[Pasted image 20251120131000.png]]
## MII 接口
MII 接口一共有 16 根信号线，含义如下： 
![[Pasted image 20251120131316.png]]
- TX_CLK：发送时钟，如果网速为 100M 的话时钟频率为 25MHz，10M 网速的话时钟频率 为 2.5MHz，此时钟由 PHY 产生并发送给 MAC。
- TX_EN：发送使能信号。
- TX_ER：发送错误信号，高电平有效，表示 TX_ER 有效期内传输的数据无效。10Mpbs 网 速下 TX_ER 不起作用。
- TXD[3:0]：发送数据信号线，一共 4 根。
- RXD[3:0]：接收数据信号线，一共 4 根。
- RX_CLK：接收时钟信号，如果网速为 100M 的话时钟频率为 25MHz，10M 网速的话时钟 频率为 2.5MHz，RX_CLK 也是由 PHY 产生的。
- RX_ER：接收错误信号，高电平有效，表示 RX_ER 有效期内传输的数据无效。10Mpbs 网 速下 RX_ER 不起作用。
- RX_DV：接收数据有效，作用类似 TX_EN。
- CRS：载波侦听信号。
- COL：冲突检测信号。
## RMII 接口
RMII 接口只需要 7 根数据线，相比 MII 直接减少了 9 根，极大的 方便了板子布线
![[Pasted image 20251120131349.png]]
- TX_EN：发送使能信号。
- TXD[1:0]：发送数据信号线，一共 2 根。
- RXD[1:0]：接收数据信号线，一共 2 根。
- CRS_DV：相当于 MII 接口中的 RX_DV 和 CRS 这两个信号的混合。
- REF_CLK：参考时钟，由外部时钟源提供，频率为 50MHz。这里与 MII 不同，MII 的接 收和发送时钟是独立分开的，而且都是由 PHY 芯片提供的。
## GMII 接口
GMII（Gigabit Media Independant Interface），千兆 MII 接口。GMII 采用 8 位接口数据，工 作时钟 125MHz，因此传输速率可达 1000Mbps；同时兼容 MII 所规定的 10/100Mbps 工作方式。 GMII 接口数据结构符合 IEEE 以太网标准，该接口定义见 IEEE 802.3-2000。
![[Pasted image 20251120131612.png]]
- GTX_CLK：1000M 工作模式下的发送时钟（125MHz）。
- TX_EN：发送使能信号。
- TX_ER：发送错误信号，高电平有效，表示 TX_ER 有效期内传输的数据无效。
- TXD[7:0]：发送数据信号线，一共 8 根。
- RXD[7:0]：接收数据信号线，一共 8 根。
- RX_CLK：接收时钟信号。
- RX_ER：接收错误信号，高电平有效，表示 RX_ER 有效期内传输的数据无效。
- RX_DV：接收数据有效，作用类似 TX_EN。
- CRS：载波侦听信号。
- COL：冲突检测信号。
与 MII 接口相比，GMII 的数据宽度由 4 位变为 8 位，GMII 接口中的控制信号如 TX_ER、 TX_EN、RX_ER、RX_DV、CRS 和 COL 的作用同 MII 接口中的一样，发送参考时钟 GTX_CLK 和接收参考时钟 RX_CLK 的频率均为 125MHz(在 1000Mbps 工作模式下)。
在实际应用中，绝大多数 GMII 接口都是兼容 MII 接口的，所以，一般的 GMII 接口都有两个发送参考时钟：TX_CLK 和 GTX_CLK(两者的方向是不一样的，前面已经说过了)，在用作 MII 模式时，使用 TX_CLK 和 8 根数据线中的 4 根。
## RGMII 接口
RGMII(Reduced Gigabit Media Independant Interface),精简版 GMII 接口。将接口信号线数量 从 24 根减少到 14 根(COL/CRS 端口状态指示信号，这里没有画出)。TX/RX 数据宽度从 8 为变为 4 位，时钟频率仍旧为 125MHz，为了保持 1000Mbps 的传输速率不变，RGMII 接口在时钟的上升沿和下降沿都采样数据
![[Pasted image 20251120131704.png]]
## MDIO 接口
MDIO 全称是 Management Data Input/Output，直译过来就是管理数据输入输出接口，是一 个简单的两线串行接口，一根 MDIO 数据线，一根 MDC 时钟线。驱动程序可以通过 MDIO 和 MDC 这两根线访问 PHY 芯片的任意一个寄存器。MDIO 接口支持多达 32 个 PHY。同一时刻 内只能对一个 PHY 进行操作，那么如何区分这 32 个 PHY 芯片呢？和 IIC 一样，使用器件地址 即可。同一 MDIO 接口下的所有 PHY 芯片，其器件地址不能冲突，必须保证唯一，具体器件 地址值要查阅相应的 PHY 数据手册。![[Pasted image 20251120131944.png]]
## PHY芯片
PHY 芯片寄存器地址空间为 5 位，地 址 0~31 共 32 个寄存器，IEEE 定义了 0~15 这 16 个寄存器的功能，16~31 这 16 个寄存器由厂 商自行实现。
## Linux 内核网络驱动框架
### net_device 结构体
网络驱动的核心就是初始化 net_device 结构体中的各个成员变量，然后将初始化完成以 后的 net_device 注册到 Linux 内核中
### net_device_ops 结构体
net_device 有个非常重要的成员变量：net_device_ops，为 net_device_ops 结构体指针类型，这就是网络设备的操作集
- ndo_open 函数
- ndo_stop 函数
- ndo_start_xmit 函数
### sk_buff 结构体
对于应用层而言不用关心具体的底层是如何工作的，只需要按照协议将要 发送或接收的数据打包好即可
- dev_queue_xmit 函数
- netif_rx 函数
### 网络 NAPI 处理机制
## RK3568 网络驱动简介
### 网络外设设备树
``` c
&mdio0 {
    status = "okay";
    rgmii_phy: phy@0 {
        compatible = "ethernet-phy-ieee802.3-c22";
        reg = <0x0>;    // PHY MDIO 地址(硬件决定)
    };
};

&gmac0 { // RGMII 1000Mbps
    status = "okay";
    phy-mode = "rgmii-rxid";    // 接口类型
    clock_in_out = "output";    // 时钟方向

    snps,reset-gpio = <&gpio0 RK_PB4 GPIO_ACTIVE_LOW>;
    snps,reset-active-low;
    snps,reset-delays-us = <0 20000 100000>;


    pinctrl-names = "default";
    pinctrl-0 = <&rgmiim0_miim  // 4 B2 B3
            &rgmiim0_tx_bus2    // 4 A2 A3 A4
            &rgmiim0_rx_bus2    // 4 A5 A6 A7
            &rgmiim0_rgmii_clk  // 4 A1 3 D6
            &rgmiim0_rgmii_bus  // 3 D4 D5 D7 4 A0
            &rgmiim0_clk    // 4 B7
            &ethm0_pins // 4 B1
            &gmac2_reset_gpio>; // 0 B4

    /* 25MHz 时钟输出给 PHY */
    assigned-clocks = <&cru CLK_GMAC_ETH_OUT2IO>;
    assigned-clock-rates = <25000000>;


    max-speed = <1000>;
    phy-handle = <&rgmii_phy>;  // 关联 PHY
};
```
- **phy-mode**
	定义 MAC 与 PHY 之间的接口类型和时钟延迟模式
	常见的 phy-mode 值:

| 模式           | 说明         | 延迟位置         |
| ------------ | ---------- | ------------ |
| `rgmii`      | 无内部延迟      | 需要PCB走线补偿    |
| `rgmii-rxid` | 仅 RX 延迟    | PHY 内部 RX 延迟 |
| `rgmii-txid` | 仅 TX 延迟    | PHY 内部 TX 延迟 |
| `rgmii-id`   | RX + TX 延迟 | PHY 内部双向延迟   |
| `rmii`       | RMII 接口    | 100Mbps 接口   |
- **clock_in_out**
	定义 GMAC 的参考时钟方向
	- 可选值:
		**`"output"`** - MAC 作为时钟源,输出时钟给 PHY
		**`"input"`** - PHY 作为时钟源,MAC 接收时钟
- **assigned-clocks**
	表示 MAC 的时钟源
- **assigned-clock-rates**
设置时钟频率为 25MHz (25,000,000 Hz)
以太网 PHY 芯片的**标准参考时钟频率**:

|速度|参考时钟|说明|
|---|---|---|
|10 Mbps|2.5 MHz|PHY 内部 PLL 倍频|
|100 Mbps|25 MHz|← **你的配置**|
|1000 Mbps|125 MHz|RGMII 时钟|
- **phy-handle**
	表示连接到此网络设备的 PHY 芯片句柄















