
# 一、镜像核心组成部分
- **Bootloader（启动引导程序）**
	板子上电后首先运行的代码，负责初始化硬件，为内核运行做准备，然后加载并启动 Linux 内核![[Pasted image 20251103184725.png]]
- **Linux Kernel（内核）**
	系统的核心，负责管理硬件资源（CPU、内存、设备等），为应用程序提供运行环境
- **DTS（设备树）**
	Uboot 和 Linux 不能直接识别 DTS 文件，而 DTB 可以被内核与 BootLoader 识别解析，通常在制 作 NAND Flash、SD Card 启动镜像时，通常会为 DTB 文件留下一部分存储区域以存储 DTB，在 BootLoader 启动内核时，会先读取 DTB 到内存，再提供给内核使用![[Pasted image 20251103185058.png]]
- **rootfs（根文件系统）**
	根文件系统（rootfs）是 linux 在初始化时加载的第一个文件系统，根文件系统包括根目录和真实文件系统，它包含系统引导和使其他文件系统得以挂载（mount）所必要的文件。根文
	件系统包 函 Linux 启动时所必须的目录和关键性的文件，例如 Linux 启动时必要的初始化文件，它在 init 目录下。此外根文件系统中还包括了许多的应用程序 bin 目录等，任何包括这些 Linux 系统启动 所必须的文件都可以成为根文件系统![[Pasted image 20251103185329.png]]

|方法|适用人群|优点|缺点|
|---|---|---|---|
|**1. 使用现成的系统镜像**|初学者、爱好者、快速原型|简单、快速、稳定|定制化程度低|
|**2. 使用构建工具（如 Buildroot, Yocto）**|开发者、工程师、产品经理|高度可定制、可重复构建|学习曲线较陡，编译时间长|
|**3. 手动从零构建**|深入学习、特定需求|完全控制，理解最深|极其繁琐、易出错、耗时长|

# 二、LubanCat_Gen_SDK
1. **extboot分区**
	extboot 分区系统是野火基于瑞芯微 Linux_SDK 框架搭建的一种 LubanCat-RK 系列板卡通用镜 像实现方式。可以实现一个镜像烧录到 LubanCat 使用同一型号处理器的所有板卡，解决了默认 rkboot 分区方式设备树固定，导致一个镜像只能适配一款板卡的问题，大大降低了由于型号众多 导致的后期维护的复杂性。
	 extboot 分区使用 ext4 文件系统格式，在编译过程中将所有 LubanCat-RK 系列板卡设备树都编译 并打包到分区内，并借助 SDRADC 读取板卡硬件 ID，来实现设备树自动切换。同时支持设备树 插件，自动更新内核 deb 包，在线更新内核和驱动模块等功能。
2. **自动构建**
	``` bash
	./build.sh chip
	```
	如果在编译完一个主芯片的工程后需要切换编译其他的主芯片，要先用以下命令清理 SDK，防 止由缓存或编译环境引起的编译错误
	``` bash
	./build.sh cleanall
	```
	如果已经选择过了主芯片并且不需要切换主芯片，而是要切换同一主芯片的其他板卡或文件系 统类型，则不需要清理 SDK
	``` bash
	./build.sh lunch
	```
3. **分步构建**
	1. 选择SDK配置文件
		``` bash
		./build.sh LunbanCat_rk3576_debian_lite_defconfig
		```
	2. U-Boot构建
		``` bash
		./build.sh uboot
		```
		构建生成的 U-boot 镜像为 u-boot/uboot.img
	3. Kernel构建
		``` bash
		./build.sh kerneldeb
		./build.sh extboot
		```
		构建生成的 kernel 镜像为 kernel/extboot.img
	4. rootfs构建
		首先要确保 SDK 的配置文件与要构建的 rootfs 一致，如果当前配置文件与要构建的 rootfs 不一 致，需要先切换配置文件
		``` bash
		./build.sh LubanCat_rk3576_debain_lite_defconfig
		./buuild.sh debian
		```
		生成的根文件系统镜像的命名规则是 linaro-(SOC 型号)-(桌面版本)-rootfs.img，保存在对应的 de-bian11 或 debian12 目录下
	5. 打包
		当 u-boot，kernel，Rootfs 都构建完成以后，需要再执行./build.sh firmware 进行固件打包，主要是 检查分区表文件是否存在，各个分区是否与分区表配置对应，并根据配置文件将所有的文件复制 或链接到 rockdev/内
		``` bash
		./build.sh firmware
		./build.sh updateimg
		```
## 三、内核编译修改
Linux 内核的配置系统由三个部分组成，分别是
	Makefile
		分布在 Linux 内核源代码顶层目录及各层目录中，定义 Linux 内核的编译规则
	配置文件
		给用户提供配置选择的功能，如 Kconfig 文件定义了配置项，在编译时，使用 arch/arm64/configs/lubancat2_defconfig 文件对配置项进行赋值
	配置工具
		包括配置命令解释器（对配置脚本中使用的配置命令进行解释）和配置用户界 面（linux 提供基于字符界面、基于 Ncurses 图形界面以及基于 Xwindows 图形界面的用户配 置界面，各自对应于 make config、make menuconfig 和 make xconfig）
``` bash
make menuconfig KCONFIG_CONFIG=arch/arm64/configs/lubancat3_rk3576_defconfig ARCH=arm64
```
修改完成后，选择右下角 Save 进行保存，注意不要保存到原路径，而是保存到.config
``` bash
# 保存 defconfig 文件
make savedefconfig ARCH=arm64
# 覆盖原来的配置文件
cp defconfig arch/arm64/configs/lubancat3_rk3576_defconfig
```
这样保存的原因是配置文件默认是精简版本的，编译使用时会和默认的配置文件进行比较从而 得到完整的配置，如果直接保存则是完整版本的，会比精简版多几千行配置，不利于观察、修改。
## 四、一般SDKbuild脚本的使用
- **查看帮助信息**
	``` bash
	./build.sh help
	```
- **选择芯片和配置**
	``` bash
	#使用默认配置
	./build.sh init
	
	# 选择芯片（如 rk3562）
	./build.sh chip
	
	# 或者直接指定芯片和配置
	./build.sh rk3562:rockchip_defconfig
	```
- **配置 SDK**
	在`SDK/kernel/`目录下
	``` bash
	# 使用默认配置
	make rockchip_defconfig
	
	# 自定义配置
	make menuconfig
	
	# 保存配置
	make savedefconfig
	```
- **构建所有组件**
	``` bash
	./build.sh all
	# 或直接运行
	./build.sh
	```
- **构建单个模块**
	``` bash
	# 构建内核
	./build.sh kernel
	
	# 构建 u-boot
	./build.sh uboot
	
	# 构建 rootfs
	./build.sh rootfs
	```
- **清理操作**
	``` bash
	# 清理所有
	./build.sh cleanall
	
	# 清理特定模块
	./build.sh clean:kernel
	./build.sh clean:uboot
	./build.sh clean:kernel:uboot  # 清理多个模块
	```
- **进入开发 Shell**
	``` bash
	./build.sh shell
	```
- **📝 工作流程**
	``` bash
	# 1. 查看支持的目标
	make help
	
	# 2. 选择配置
	make rockchip_defconfig
	
	# 3. （可选）自定义配置
	make menuconfig
	make savedefconfig
	
	# 4. 编译
	./build.sh
	
	# 5. 烧录固件
	# 将 output/firmware/update.img 烧录到设备
	```

## 二、编译过程中的设备树与驱动模块
### 设备树
- 📍 **配置位置**
	`kernel/arch/arm64/boot/dts/rockchip/Makefile`
- 📝 **配置格式**
	``` c
	# 语法
	dtb-$(CONFIG_ARCH_ROCKCHIP) += <dts文件名>.dtb
	
	# 示例 - RK3568
	dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3568-evb1-ddr4-v10.dtb
	dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3568-evb1-ddr4-v10-linux.dtb
	```
- ⚙️ **控制条件**
	**`CONFIG_ARCH_ROCKCHIP`**: 必须在内核配置中启用
	``` c
	# 在 kernel/.config 中
	CONFIG_ARCH_ROCKCHIP=y
	```
- **编译流程**
	``` c
	# 1. Makefile 定义了哪些 DTS 要编译
	kernel/arch/arm64/boot/dts/rockchip/Makefile:
	    dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3568-evb1-ddr4-v10.dtb
		
	# 2. 编译时检查 CONFIG_ARCH_ROCKCHIP
	kernel/.config:
	CONFIG_ARCH_ROCKCHIP=y
	
	# 3. 如果启用，编译对应的 DTS
	kernel/arch/arm64/boot/dts/rockchip/rk3568-evb1-ddr4-v10.dts
		↓ (Device Tree Compiler)
	kernel/arch/arm64/boot/dts/rockchip/rk3568-evb1-ddr4-v10.dtb
	
	# 4. 打包到 boot.img
	output/boot.img
	```
### 驱动模块
- **📍配置位置**
	1. **Kconfig**：`kernel/drivers/media/i2c/Kconfig`
		``` c
		config VIDEO_SC233HGS
	    tristate "SmartSens SC233HGS sensor support"
	    depends on I2C && VIDEO_V4L2
	    ...
		
		config VIDEO_MAX96724
	    tristate "Maxim MAX96724 GMSL2 Deserializer support"
	    depends on I2C && VIDEO_V4L2
	    ...
		```
		这里配置的信息会出现在`kernel`的`menuconfig`中
	2. **Makefile**：`kernel/drivers/media/i2c/Makefile`
		``` c
		obj-$(CONFIG_VIDEO_SC233HGS) += sc233hgs.o
		obj-$(CONFIG_VIDEO_MAX96724) += max96724.o
		```
	3. **rockchip_linux_defconfig**：`kernel/arch/arm64/configs/rockchip_linux_defconfig`
		控制是否编译
		``` c
		# 内置到内核，生成.p文件，然后打包进内核
		CONFIG_VIDEO_SC233HGS=y
		CONFIG_VIDEO_MAX96724=y
		
		# 编译为模块，生成.ko文件
		CONFIG_VIDEO_SC233HGS=m
		CONFIG_VIDEO_MAX96724=m
		
		# 不编译
		# CONFIG_VIDEO_SC233HGS is not set
		```
		==修改以后使用以下命令以应用==
		``` bash
		./build.sh kernel
		```
 - **编译流程**
	 ``` c
	 # 1. Kconfig 定义配置选项
	kernel/drivers/media/i2c/Kconfig:
	    config VIDEO_SC233HGS
	        tristate "..."
	
	# 2. 用户通过 menuconfig 选择
	make menuconfig
	    → Device Drivers
	      → Multimedia support
	        → Camera sensor devices
	          → [*] SmartSens SC233HGS sensor support
	
	# 3. 保存到 .config
	kernel/.config:
	    CONFIG_VIDEO_SC233HGS=y
	
	# 4. Makefile 根据配置编译
	kernel/drivers/media/i2c/Makefile:
	    obj-$(CONFIG_VIDEO_SC233HGS) += sc233hgs.o
	    ↓
	    编译 sc233hgs.c → sc233hgs.o → 链接到内核或生成 sc233hgs.ko
	
	# 5. 最终结果
	# 如果 =y：编译进 Image.gz
	# 如果 =m：生成 sc233hgs.ko
	 ```
### .o文件打包进内核的完整流程
1. **编译阶段 (scripts/Makefile.build)**
	``` c
	# 步骤1: 读取驱动目录的Makefile
	obj-$(CONFIG_VIDEO_MAX96724) += max96724.o
	
	# 步骤2: 编译.c生成.o
	max96724.o: max96724.c
	    $(CC) -c max96724.c -o max96724.o
	```
2. **归档阶段 (scripts/Makefile.build:401-408)**
	``` c
	# 步骤3: 创建built-in.a归档文件
	quiet_cmd_ar_builtin = AR      $@
	cmd_ar_builtin = rm -f $@; $(AR) cDPrST $@ $(real-prereqs)
	
	$(obj)/built-in.a: $(real-obj-y) FORCE
	    $(call if_changed,ar_and_symver)
	```
	**作用**：将该目录下所有的.o文件打包成built-in.a归档文件
	- `drivers/media/i2c/built-in.a` 包含该目录的所有.o文件
	- 使用`ar`命令创建静态库
3. **收集阶段 (kernel/Makefile:1269-1277)**
	``` c
	# 步骤4: 收集所有built-in.a文件
	KBUILD_VMLINUX_OBJS := $(head-y) $(patsubst %/,%/built-in.a, $(core-y))
	KBUILD_VMLINUX_OBJS += $(addsuffix built-in.a, $(filter %/, $(libs-y)))
	KBUILD_VMLINUX_OBJS += $(patsubst %/,%/built-in.a, $(drivers-y))
	```
	**作用**：收集整个内核树中的所有built-in.a文件
	- `init/built-in.a`
	- `kernel/built-in.a`
	- `drivers/built-in.a` (包含所有驱动的built-in.a)
	- `mm/built-in.a`
	- 等等...
4. **链接阶段 (scripts/link-vmlinux.sh:158-208)**
	``` c
	# 步骤5: 链接vmlinux
	vmlinux_link() {
	    objects="--whole-archive              \
	        ${KBUILD_VMLINUX_OBJS}           \
	        --no-whole-archive               \
	        --start-group                    \
	        ${KBUILD_VMLINUX_LIBS}          \
	        --end-group"
	        
	    ${LD} ${KBUILD_LDFLAGS} ${LDFLAGS_vmlinux} \
	        -o vmlinux \
	        -T ${lds} ${objects}
	}
	```
	**关键参数**：
	- `--whole-archive`: 强制链接器包含归档文件中的**所有对象**，即使没有被引用
	- `--no-whole-archive`: 结束全包含模式
## 三、两个文件的作用

### 1️⃣ **rockchip_linux_defconfig**
**性质**: 默认配置模板（源文件）
**作用**:
- 📝 Rockchip 官方提供的默认内核配置
- 🔧 用于生成 `.config` 的基础模板
- 💾 保存在 Git 仓库中，可以提交和共享
- 🎯 定义了 Rockchip 平台推荐的基本配置
**位置**:
```
kernel/arch/arm64/configs/rockchip_linux_defconfig
```
**使用场景**:
- 首次配置内核
- 重置配置到默认状态
- 共享配置给团队其他成员
### 2️⃣ **.config**
**性质**: 当前编译使用的配置（生成文件）
**作用**:
- 🚀 **实际编译时使用的配置文件**
- 🔄 从 defconfig 生成，可以进一步修改
- ⚙️ 包含所有配置项（包括依赖关系解析后的结果）
- 🚫 通常不提交到 Git（在 .gitignore 中）
**位置**:
``` 
kernel/.config
```
**使用场景**:
- 编译内核时自动读取
- menuconfig 修改后自动更新
- 临时测试不同的配置
``` 
# 步骤 1: 从 defconfig 生成 .config
cd kernel
make ARCH=arm64 rockchip_linux_defconfig

# 这会读取:
arch/arm64/configs/rockchip_linux_defconfig
# 生成:
.config

# 步骤 2: 可选 - 使用 menuconfig 进一步修改
make ARCH=arm64 menuconfig
# 修改保存到:
.config

# 步骤 3: 编译（使用 .config）
make ARCH=arm64 Image.gz dtbs -j$(nproc)
```