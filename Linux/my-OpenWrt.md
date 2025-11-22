# 第四章 OpenWRT

## 一、命令
``` bash
make clean    # 清理编译生成的文件，但保留配置和下载的源码包
make dirclean # 更彻底的清理，包括交叉编译工具链（但保留下载的源码包）
make distclean # 最彻底的清理，恢复源代码到初始状态，删除所有下载内容

make -j$(nproc) # 自动使用你电脑的所有核心进行编译
make download -j$(nproc) # 仅下载所有必需的源码包，不编译。在网速慢的情况下可以先下载
make -j1 V=s # 单线程编译，并显示详细输出。用于调试编译错误
```
## 二、镜像格式
- **传统格式：**
	**`zImage`** - 压缩的ARM内核镜像（小于512KB）
	**`uImage`** - U-Boot传统格式，包含U-Boot头部信息
	**`bzImage`** - x86架构的压缩内核镜像
	**`Image`** - 未压缩的原始内核二进制文件（ARM64常用）
	**`Image.gz`** - gzip压缩的内核镜像
- **现代FIT格式：**
	**`fit.itb`** - FIT (Flattened Image Tree) 格式
	**`uImage.itb`** - FIT格式的U-Boot镜像
	**优势**：可以包含内核+设备树+initramfs，支持数字签名
## 三、全局变量
1. **`DTS_DIR`**
	**让构建系统知道在哪里查找Xilinx Zynq相关的设备树文件**
	``` C
	#DTS_DIR：默认定义
	DTS_DIR:=$(LINUX_DIR)/arch/$(LINUX_KARCH)/boot/dts
	```
	- `$(LINUX_DIR)` = 内核源码目录
	- `$(LINUX_KARCH)` = `arm` (对于ARM架构)
	- 所以初始值大概是：
		build_dir/target-arm_cortex-a9+neon_musl_eabi/linux-6.6.xx/arch/arm/boot/dts
2. **`DEVICE_DTS_DIR`**
	**默认值为空!!!**
3. **`DEVICE_DTS`**
## 四、OpenWrt 设备树文件查找机制
``` C
# 在 include/image.mk 中的关键逻辑：
$$(if $$(DEVICE_DTS_DIR),$$(DEVICE_DTS_DIR),$$(DTS_DIR))
```
**编译器会优先在 `target/linux/zynq/dts` 目录下寻找设备树文件**
## 五、OpenWrt编译生成镜像的流程
1. **`make menuconfig`**
	生成`.config` - 主配置文件
2. **构建交叉编译工具链**
	``` bash
	make tools/install
	make toolchain/install
	```
	生成`staging_dir/toolchain-arm_cortex-a9+neon_gcc-14.3.0_musl_eabi/`
3. **内核编译**
	``` BASH
	make target/linux/compile
	```
	1. 内核源码准备
		``` bash
		build_dir/target-arm_cortex-a9+neon_musl_eabi/linux-zynq_yourboard/linux-6.6.110/
		```
	2. 设备树编译
		``` 
		build_dir/target-arm_cortex-a9+neon_musl_eabi/linux-zynq_yourboard/
		```
	3. 内核镜像生成
4. **软件包构建**
	``` bash
	make package/compile
	```
5. **根文件系统生成**
	``` bash
	make target/install
	```
	1. 创建根目录结构
		``` 
		build_dir/target-arm_cortex-a9+neon_musl_eabi/root-zynq/
		├── bin/
		├── etc/
		├── lib/
		├── sbin/
		├── usr/
		└── var/
		```
	2. 安装软件包
		``` bash
		opkg install --root=./root-zynq package.ipk
		```
	3. 生成文件系统镜像
6. **U-Boot 构建**
	``` bash
	make package/boot/uboot-zynq/compile
	```
## 六、从原始 Zynq 目录添加自定义板卡的完整指南
1. **创建自定义子目标**
	`openwrt/target/linux/zynq/yourboard/target.mk`
	```
	BOARDNAME:=YourBoard
	
	define Target/Description
		Build firmware image for custom Zynq board.
	endef
	```
2. **添加设备树文件**
	1. `target/linux/zynq/dts/zynq-your-board.dts`
		``` dts
		/dts-v1/;
		
		#include "zynq-7000.dtsi"
			
		/ {
			model = "Your Custom Zynq Board";
			compatible = "your,board", "xlnx,zynq-7000";
			
			memory@0 {
				device_type = "memory";
				reg = <0x0 0x40000000>;  // 1GB RAM, 根据实际调整
			};
		
			chosen {
				bootargs = "console=ttyPS0,115200 root=/dev/mmcblk0p2 rootwait";
				stdout-path = "serial0:115200n8";
			};
		};
			
		&uart0 {
			status = "okay";
		};
			
		&sdhci0 {
			status = "okay";
		};
			
		&gem0 {
			status = "okay";
			phy-mode = "rgmii-id";
			phy-handle = <&ethernet_phy>;
			
			ethernet_phy: ethernet-phy@0 {
				reg = <0>;
			};
		};
		```
	2. 复制`.dtsi`文件（如果需要）
3. **修改镜像构建配置**
	1. 修改主芯片`Makefile`
		在`openwrt/target/linux/zynq/Makefile`中
		``` 
		# 原始：
		SUBTARGETS:=generic
		
		# 修改为：
		SUBTARGETS:=generic yourboard
		```
	2. 修改主镜像`Makefile`
		在`openwrt/target/linux/zynq/image/Makefile`中
		```
		# 确保 Device/Default 使用本地 DTS 目录
		define Device/Default
		    PROFILES := Default
		    DEVICE_DTS_DIR := ../dts
		    KERNEL_DEPENDS = $$(wildcard $$(DEVICE_DTS_DIR)/$$(DEVICE_DTS).dts)
		    KERNEL_LOADADDR := 0x8000
		    IMAGES := sdcard.img.gz
		    IMAGE/sdcard.img.gz := zynq-sdcard | gzip
		endef
		
		# 确保 FitImageGzip 使用正确路径
		define Device/FitImageGzip
		    KERNEL_SUFFIX := -fit-uImage.itb
		    KERNEL = kernel-bin | gzip | fit gzip $$(KDIR)/image-$$(DEVICE_DTS).dtb
		    KERNEL_NAME := Image
		endef
		
		# 末尾添加
		include yourboard.mk
		```
	3. 创建设备定义`.mk`
		`target/linux/zynq/image/yourboard.mk`
		```
		define Device/your_vendor_zynq-your_board
		    $(call Device/FitImageGzip)
		    DEVICE_VENDOR := YourVendor
		    DEVICE_MODEL := YourBoard
		    DEVICE_DTS := zynq-your-board
		    IMAGES := sdcard.img.gz
		    IMAGE/sdcard.img.gz := zynq-sdcard | gzip
		endef
		TARGET_DEVICES += your_vendor_zynq-your_board
		```
4. **配置 U-Boot 支持**
	1. 在`openwrt/package/boot/uboot-zynq/Makefile`中
		``` 
		# 修改默认子目标（如果需要）
		define U-Boot/Default
			BUILD_TARGET:=zynq
			BUILD_SUBTARGET:=yourboard  # 如果只构建自定义板卡
			BUILD_SUBTARGET:=yourboard
			UBOOT_IMAGE:=spl/boot.bin u-boot.img
			UBOOT_CONFIG:=zynq_$(1)
			UENV:=default
			HIDDEN:=1
		endef
		
		# 添加自定义板卡定义
		define U-Boot/your_board
			NAME:=Your Custom Zynq Board
			BUILD_DEVICES:=your_vendor_zynq-your_board
			UBOOT_CONFIG:=zynq_zc702  # 使用兼容的配置
			UENV:=default
		endef
		
		# 更新目标列表
		UBOOT_TARGETS := \
			zc702 \
			zed \
			zybo \
			zybo_z7 \
			your_board
		```
	2. 修复脚本权限和格式
		```
		# 确保脚本有执行权限
		chmod +x target/linux/zynq/image/gen_zynq_sdcard_img.sh
		
		# 修复可能的换行符问题
		sed -i 's/\r$//' target/linux/zynq/image/gen_zynq_sdcard_img.sh
		```
5. **编译**
	``` bash
	make -j$(nproc)
	```