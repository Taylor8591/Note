---
Data: 2025-08-18
---
# 第一章 Linux初识
## §1.1：Linux系统的组成
- **Linux系统内核**
	内核提供系统最核心的功能，如：调度CPU，调度内存，调度文件系统， 调度网络通讯， 调度IO等
- **系统级应用程序**
	出场自带应用程序，如：文件管理器，图片查看，音乐播放等	![[Linux系统构成.png]]
## §1.2：Linux常用命令
- 命令参数有长格式`-—`与短格式`-`之分，长格式与长格式之间不能合并，但短格式与短格式之间可以合并。
### 一、常用系统工作命令
- **在终端设备上输出字符串或变量提取后的值**
	``` bash
	echo [字符串] [$变量]
	```
	- $符号的意思是提取变量的实际值。
- `reboot/poweroff`：重启/关机
- **查看系统中的==进程==状态**
	``` bash
	ps [参数]
	```
	![[ps命令参数.png]]
### 二、系统状态检测命令
- **获取网卡配置与网络状态等信息/查看系统内核版本与系统架构等信息**
	``` bash
	ifconfig/uname
	```
- **测试主机之间的网络联通性**
	``` bash
	ping [参数] 主机地址
	```
	![[ping命令参数.png]]
### 三、 查找定位文件命令
- **显示用户当前所处的工作目录**
	``` bash
	pwd
	```
- **切换当前工作路径**
	``` bash
	cd [参数] [目录]
	```
	`cd ../`：==返回上一级目录==
	`cd ~`：返回当前用户的home
	`cd ~ username`：返回制定用户的home
- **显示目录中文件信息**
	``` bash
	ls [参数] [文件名称]
	```
	`ls -a`：显示全部文件
	`ls -l`：现实文件详细信息
	`ls *查找字符*`：==列出包含查找字符的文件==
- **按照指定条件查找文件所对应位置**
	``` bash
	find [查找范围] 寻找条件
	```
	![[find命令参数.png]]
### 四、文本文件编辑命令
- `cat [参数] 文件名称`：查看内容较少的纯文本文件
	`cat -n`：显示行数
- `more [参数] 文件名称`：查看内容较多的纯文本文件
- `head [参数] 文件名称`：查看纯文本的前N行
- `tail [参数] 文件名称`：查看纯文本的后N行或持续刷新的内容
	`tail -f 文件名称`：持续刷新。
- `tr [原始字符] [目标字符]`：替换文本内容中的字符
	注：`[]`不能省略
- `wc [参数] 文件名称`：统计文本文件的行数、字数、字节数
	![[wc命令参数.png]]
- `stat 文件名称`：查看文件的具体存储细节和时间等信息
### 五、 文件目录管理命令
- `touch [参数] 文件名称`：创建空白文件或设置文件的时间
	![[touch命令参数.png]]
- `mkdir [参数] 目录名称`：创建空白目录
- `cp [参数] 源文件名称 目标文件名称`：复制文件或目录
	- 复制的三种情况：
		- 如果目标文件是目录，则会把源文件复制到该目录中
		- 如果目标文件也是普通文件，则会询问是否要覆盖它
		- 如果目标文件不存在，则执行正常的复制操作
	- 在复制目录时要加上`-r`参数
- `mv [参数] 源文件名称 目标文件名称`：剪切或重命名文件
- `rm [参数] 文件名称`：删除文件或目录
	`rm -f`可以强制删除
	`rm -r`删除目录
- `file 文件名称`：查看文件类型
- `tar 参数 文件名称`：对文件进行打包压缩或解压
	`tar -czvf 压缩包名.tar.gz 要打包的目录名`来把指定文件进行压缩
# 第二章 Linux应用开发基础知识
## §2.1：交叉编译
交叉编译（Cross Compilation）是指在一个平台上生成另一个平台上的可执行代码的过程。
- 在Ubuntu中可以执行以下命令编译、执行：
	``` bash
	gcc hello.c -o hello   //已经在当前目录下编译出可执行文件hello
		./hello               //运行当前目录下的hello，未传入参数
		Hello, world!
		./hello weidongshan   //运行当前目录下的hello，传入参数
		Hello, weidongshan!
	```
- 上述命令编译得到的可执行程序hello可以在Ubuntu中运行，但是如果把 它放到ARM板子上去，它是无法执行的。因为它是使用gcc编译的，是给PC机 编译的，里面的机器指令是x86的。 我们要想给ARM板编译出hello程序，需要使用交叉编译工具链：
	``` bash
	arm-buildroot-linux-gnueabihf-gcc -o hello hello.c
	```
## §2.2：GCC编译器
- **基本介绍**
	源文件需要经过编译才能生成可执行文件。Linux下也有很优秀的集成开发工具，但是更多的时候是**直接使用编译**工具；即使使用集成开发工具，也需要掌握一些编译选项
	PC机上编译出来的程序在`x86`平台上运行。要编译出能在`ARM`平台上运行的程序，必须使用交叉编译工具xxx-gcc、xxx-ld等
	- 可以使用以下来安装
		``` bash
		sudo apt update
		sudo apt install gcc
		```
- **GCC编译过程**
	一个C/C++文件要经过：预处理(preprocessing)、编译(compilation)、汇 编(assembly)和链接(linking)等4步才能变成可执行文件。![[编译过程.png]]
	**一步执行**：
		`gcc main.c hello.c -o test [-I</include>]     //main.c hello.c-->test`
	**分布执行**：
		`gcc -E hello.c -o hello.i      //预处理：hello.c-->hello.i`
		`gcc -S hello.c/i -o hello.s    //编译hello.c/i-->hello.s`
		`gcc -c hello.c/s -o hello.o    //汇编hello.c/s-->hello.o`
		`gcc main.o hello.o -o test     //链接main.o hello.o-->test`
	1. **预处理(preprocessing)**
		预处理就是将要包含(include)的文件插入原文件中、将宏定义展开、根据条件 编译命令选择要使用的代码，最后将这些东西输出到一个“.i”文件中等待进一 步处理。
		C/C++源文件中，以“#”开头的命令被称为预处理命令，如包含命令 “#include”、宏定义命令“#define”、条件编译命令“#if”、“#ifdef”等。
	2. **编译(compilation)**
		编译就是把C/C++代码(比如上述的“.i”文件)“翻译”成汇编代码，所用 到的工具为cc1(它的名字就是cc1，x86有自己的cc1命令，ARM板也有自己的 cc1 命令)。
	3. **汇 编(assembly)**
		汇编就是将第二步输出的编译代码翻译成符合一定格式的机器代码，在 Linux 系统上一般表现为ELF目标文件(OBJ 文件)，用到的工具为as。x86有 自己的 as 命令，ARM 版也有自己的 as 命令，也可能是`xxxx-as`（比如 `arm linux-as`）
	4. **链接(linking)**
		链接就是将上步生成的OBJ文件和系统==库==的OBJ文件、库文件链接起来，最 终生成了可以在特定平台运行的可执行文件，用到的工具为ld或collect2。
## §2.3：Makefile与make命令
指导 make 工具如何自动编译和构建软件项目的配置文件。它定义了一系列的规则，来指定文件之间的依赖关系以及如何从源文件生成目标文件
### 一、Makefile的组成
1. **规则**
	每条规则定义了如何以及何时创建一个“目标”文件，格式：
	``` makefile
	target: prerequisites
		recipe
	```
	-  **target（目标）**：通常是要生成的文件名、可执行文件名（例如 `main.o`、`hello`）；也可以是一个动作的名称（例如 `clean`），这被称为“伪目标”。
	- **prerequisites（依赖）**：是生成 `target` 所需要的文件列表。
	- **recipe（命令）**：是由一行或多行 shell 命令组成，说明了如何从依赖生成目标。
	- **非常重要：每一行命令必须以一个 Tab 键开头，而不是空格。**
2. **变量**
	类似于编程中的变量，用于简化和维护Makefile。你可以定义变量，然后在规则中引用它们。
3. **注释**
	以==`#`==开头的行是注释。
### 二、Makefile的执行逻辑
1. ==在每一个规则中，当prerequisites（依赖）比 target（目标）**新**或当target（目标）不存在时，就自动执行 recipe（命令）==
2. 也可以通过`make [规则]`来执行特定规则
### 三、Makefile的语法
- **实例**
   ``` cmake
		object := main.o hello.o
		
		test: $(object)
			gcc $(object) -o test
	
		#一定要有hello.h
		main.o: main.c hello.h
			gcc -c main.c [-o main.o]可以省略
		hello.o: hello.c hello.h
			gcc -c hello.c [-o hello.o]可以省略
	
		clean:
			rm -f $(object) test
			.PHONY: clean
	```
1. **通配符`(pattern)`**
	`%.o`：表示任意.o文件
	`$@`：表示当前规则的目标
	`$<`：表示当前规则的==第一个==依赖
	`$^`：表示当前规则的==所有==依赖
	`$()`：变量引用，函数调用
	- 例
		``` makefile
		test: a.o b.o c.o
			gcc $^ -o $@
			
		%.o%: %.c
			gcc $< -o $@ 
		```
2. **假想目标`.PHONY`**
	- 例
		``` makefile
		clean:
			rm -f *.o test
		.PHONY: clean
		```
		- 其中 `-f`可以忽略因没有文件删除的错误
3. **变量赋值**![[变量赋值.png]]
	- 最常使用`:=`
	- `object := a.o b.o`也可以写成`object := a.o/(换行再写)b.o`
4. **自动推导**
	- make可以根据目标自动加入所需的依赖文件和命令。例如main.o目标，会默认将 main.c作为依赖加入，同时也可以自动推导出编译main.o的命令，于是可以对代码进行简化：
		``` cmake
			object := main.o hello.o
			
			test: $(object)
				gcc $(object) -o test
		
			#一定要有hello.h
			main.o: hello.h
			hello.o: hello.h
		
			clean:
				rm -f $(object) test
				.PHONY: clean
		```
### 四、make命令
在Linux 中使用make命令来编译程序，特别是大程序；执行make命令时，它会去当前目录下查找名为“Makefile”的文件，并根 据它的指示去执行操作，生成第一个目标
1. **`make`命令的使用**
	- ==直接使用`make`函数==，会在当前工作区的根目录下寻找`Makefile`文件，比如
		`make`
	- 我们可以使用`-f`选项指定文件，不再使用名为`Makefile`的文件，比如：
		`make -f Makefile.build`
	- 我们可以使用`-C`选项指定目录，切换到其他目录里去，比如：
		`make -C a/ -f Makefile.build`
	- 我们可以指定目标，不再默认生成第一个目标：
		`make -C a/ -f Makefile.build other_target`
2. **变量的导出(export)**
	在编译程序时，我们会不断地使用`make -c dir`切换到其他目录，执行其他 目录里的`Makefile`。如果想让某个变量的值在所有目录中都可见，要把它`export`出来。
		`export <变量>`
3. **Makefile中可以使用shell命令**
4. [20分钟Makefile光速入门教程_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1tyWWeeEpp/?spm_id_from=333.337.search-card.all.click&vd_source=c8e6448ee8e2d5882d06805a4fc82cf6)
## §2.4：文件IO
### 一、文件IO的分类
1. ==**标准IO**：`fopen/fread/fwrite/fseek/fflush/fclose`==
2. ==**系统调用IO**：`open/read/write/lseek/fsync/close`==
3. **注**：
	 - 标准IO效率更高；但是要访问驱动程序时就不能使用标准IO，而是使用系统调用IO。
	 - 标准IO与系统调用IO的关系
		![[两种IO之间的关系1.png]]
### 二、标准IO
遵守ISO标准，基于流的I/O，对于文件指针、==`file`结构体==进行操作。带缓存
- `# include <stdio.h>`
1. **file结构体**
	![[FILE结构体 1.png]]
	- 成员说明
		- `f_count`：当计算机中不同进程同时打开该文件时，用于计数。`open/close`函数就是引用计数+1-1。调用close函数只会将引用计数-1，当引用计数为0时，才会关闭文件
		- `f_pos_lock`：当打开文件时保护文件位置不被移动
		- `inode`：每个文件都有自己唯一的inode，规定了==该文件本身固有的权限==
		- `file_operations`：当前==对文件进行操作时所拥有的权限==
		- `private_data`：
2. **打开/关闭文件**
	1. **`FILE *fopen (const char *__restrict__filename, const char *__restrict__modes)`：打开文件**
		- 参数
			- `char *__restrict__filename`: 字符串表示要打开文件的路径名称
			- `char *__restrict__modes`
				- `r`：只读模式，==没有文件打开失败==，返回`NULL`
				- `r+`：读写模式，文件必须存在，写入是从头一个一个覆盖
				- `w`：只写模式，存在文件写入会清空文件，不存在文件则创建新文件
				- `w+`：读写模式 可读取,写入同样会清空文件内容，不存在则创建新文件
				- `a`：只追加写模式，不会覆盖原有内容 新内容写到末尾，如果文件不存在则创建
				- `a+`：读写追加模式 ，读取,写入从文件末尾开始，如果文件不存在则创建
		- 返回值
			成功返回`FILE *`结构体指针，表示一个文件
			失败返回`NULL`
	2. **`int fclose (FILE *__stream)`：关闭文件****
		- 参数
			`FILE *__stream`：需要关闭的文件结构体指针
		- 返回值
			成功返回`0`
			失败返回`EOF`(负数) 通常失败会造成系统崩溃
3. **向文件中写入数据**
	1. **`int fputc (int __c, FILE *__stream)`：写入文件一个字节**
		- 参数
			- `int __c`：写入的一个字节，按照AICII值写入，可提前声明一个==char==
			- `FILE *__stream`：要写入的文件,写在哪里取决于打开时的模式
		- 返回值
			成功返回写入的字节
			失败返回`EOF`
	2. **`int fputs (const char *__restrict __s, FILE *__restrict__stream)`：写入文件一个字符串**
		- 参数
			- `char *__restrict__s`：需要写入的字符串
			- `FILE *__restrict__stream`：要写入的文件，写在哪里取决于打开时的模式
		- 返回值
			成功返回非负整数(一般是0,1)
			失败返回`EOF`
	3. **`int fprintf (FILE *__restrict__stream, const char *__restrict__fmt, ...)`：以指定格式向文件写入**
		- 参数
			- `FILE *__restrict__stream`：要写入的文件，写在哪里取决于打开时的模式
			- `char *__restrict__fmt, ...`：格式化字符串
				例：`"%s:hello world\n", name`
		- 返回值
			成功返回正整数(写入字符总数不包含换行符) 
			失败返回`EOF`
	4. **`void perror (const char *__s)`：将`errno`当前值对应的错误描述输出到标准错误输出**
		- 参数
			`char *__s`: 自定义的错误信息前缀，会打印在输出的前面 ，中间补充": " 后面跟`errno`。
		- 当系统调用或库函数发生错误时，通常会通过设置全局变量`errno`来指示错误的具体 原因。`errno`是在C语言（及其在Unix、Linux系统下的应用）中用来存储错误号的一 个全局变量。 ==`errno`定义在头文件`<errno.h>`中，引入该文件即可调用全局变量`errno`。==
5. **向文件中读出数据**
	1. **`int fgetc (FILE *__stream)`：读出文件一个字节**
		- 参数
			`FILE *__stream`：需要读取的文件
		- 返回值
			成功返回读取的一个字节
			读到文件结尾或出错返回`EOF`
		- 注：第一次调用读取第1个字节，==然后光标后移1个字节==
	2. **`fgets (char *__restrict__s, int __n, FILE *__restrict__stream)`：读出文件字符串**
		- 参数
			- `char *__restrict __s`：接收读取的数据字符串
			- `int __n`：接收数据的长度
			- `FILE *__restrict__stream`：需要读取的文件`
		- 返回值
			成功返回字符串
			失败返回`NULL`（可以直接用于while）
		- 例：
			``` C
			char buffer[100];
			while(fgets(buffer, sizeof(buffer), ioFile)){
				printf("%s", buffer); 
			}
			```
	4. **`int fscanf (FILE *__restrict__stream, const char *__restrict__format, ...)`：以指定格式从文件读出**
		- 参数
			- `FILE *__restrict __stream`：读取的文件
			- `char *__restrict __format, ...`：读取的匹配表达式
				例：`"%s %d %s\n", name, &age, wife`
		- 返回值
			成功返回参数的个数
			失败返回0，报错或结束返回`EOF`
6. **标准输入/输出/错误**
	- `stdin`: 标准输入`FILE *`
	- `stdout`: 标准输出`FILE *`
	- `stderr`: 错误输出`FILE *`
7. **缓存**
	![[三种缓存.png]]
### 三、系统调用IO
- `#include <unistd.h>`
1. **文件描述符`fd`**
	- 对内核而言，所有打开文件都由文件描述符引用。
	- 文件描述符是一个==从0~1023的**int**类型数据==，当打开或创建文件时，内核向进程返回一个文件描述符，==用于标识该文件==。
	- 在`POSIX`应用程序中，整数0、1、2被替换成符号常数，这些常数定义在了头文件`<unistd.h>`中
		`STDIN_FILENO`         0
		`STDOUT_FILENO`       1
		`STDERR_FILENO`       2
	- 关系
		![[Pasted image 20250830050210.png]]
		每个进程都会有一个自己的**文件描述符表**，当我们执行open()等系统调用时，内核会创建一个新的**文件描述**，然后给进程分配**文件描述符**，以指向内核中的**文件描述**，最后将**文件描述符**返回给应用程序。
		文件描述符表是一个指针数组，其中的每一个元素都是一个指针，文件描述符是指针数组的下标。
2. **系统调用的函数**
	- `open`![[两种open函数.png]]
		- `char *pathname`：要打开或创建的文件路径
		- `int flags`：用来说明此函数的多个选择项。当同时用到多个宏时，将它们`|`在一起
			- `O_RDONLY    //只读`
			- `O_WRONLY    //只写`
			- `O_RDWR      //读写`
			- `O_APPEND    //以追加模式打开`
			- `O_CREAT     //以mode参数来创建新文件`
			- `O_TRUNC     //如果文件存在且为只读或只写成功打开，则清空文件`
		- `mode_t mode`：当`flags`为`O_CREAT`时才有用，确定新建文件时访问权限
	- `create`![[create函数.png]]
		- 此函数等效于：`open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode)`
	- `close`![[close函数.png]]
		- `int fd`：已打开文件的文件描述符
	- `read`		![[read函数.png]]
		- 参数
			- `int fd`：读取文件的文件描述符
			- `void *buf`：存放读数据的缓存
			- `size_t count`：要求读取一次的字节数
		- 读操作从文件的当前位移量处开始，在成功返回之前，该位移量增加实际读到的字节数
		- `read`函数可以是阻塞式的，也可以是非阻塞式的
	- `write`	![[write函数.png]]
		- 参数
			- `int fd`：写入文件的文件描述符
			- `void *buf`：存放待写数据的缓存
			- `size_t count`：要求写取一次的字节数
		- 其返回值通常与参数`count`的值相同，否则表示出错
		- 对于普通文件，写操作从文件的当前位移量出开始。如果在打开该文件时，指定了O_APPEND选择项，则在每次写操作之前，将文件位移量设置在文件的当前结尾处。在一次成功写之后，该文件的位移量增加实际写的字节数
## §2.5：输入输出重定向管道符、重定向与环境变量
### 一、输入输出重定向
1. **输入重定向**
	输入重定向是指把文件导入到命令中，使用较少
	![[输入重定向.png]]
2. **输出重定向**
	输出重定向则是指把原本要输出到屏幕的数据信息写入到指定文件中，常用
	![[输出重定向.png]]
### 二、管道命令符
	把前一个命令原本要输出到屏幕的信息当作后一个命令的标准输入执行格式为：命令A｜命令B
### 三、命令行的通配符
- ![[通配符2.png]]
### 四、常用转义字符
- 反斜杠`/`：使反斜杠后面的一个变量为单纯的字符
- 单引号`’’`：转义其中所有的变量为单纯字符串
- 双引号`””`：保留其中的变量属性#
- 反引号`’’`：把其中的命令执行后返回结果
## §2.6：进程处理
### 一、进程
- 进程(progress)是正在运行的程序，是操作系统进行资源分配的基本单位。程序是储存在计算机硬盘或内存的一段二进制序列，是静态的；进程包括代码，数据以及分配给他们的其他系统资源（如文件描述符，网络连接等）
- 创建子进程后，子进程**可以得到父进程定义的所有变量的拷贝，当然也可以对拷贝进行修改，但修改不会影响父进程的变量；有一种特殊的变量——文件描述符是共享的。**
### 二、进程的创建与删除
- **`int system (const char *__command)`：创建进程**
	- 参数
		`const char *__command`：使用Linux命令创建一个子进程
	- 返回值
		成功返回0，失败返回失败编号
- **`void _exit(int status)`：进程立即退出，不执行任何清理操作**
	- 参数
		`int status`：父进程可接收到的退出状态码 0表示成功 非0表示各种不同的错误
	- 是由POSIX标准定义的系统调用,定义在==`<unistd.h>`==中
- **`void exit(int status)`：执行清理操作，终止当前进程**
	- 参数
		`int status`: 父进程可接收到的退出状态码，0表示成功 非0表示各种不同的错误
	- 是由C标准库提供的，定义在==`<stdlib.h>`==中
- 使用场景
	- 在==父进程中使用`exit()`==，以确保程序在退出前能执行清理操作，如关闭文件和刷新输出。
	- 在==子进程中==，特别是在`fork()`之后立即调用了一个执行操作但执行==失败时，使用`_exit()`或`_Exit()`==来确保子进程的快速、干净地退出，避免执行标准的清理操作，这些操作可能会与父进程发生冲突或不必要的重复。
### 三、进程处理相关的系统调用
	system函数用到的系统调用为fork、execve和waitpid。
1. **C标准语言的两种main函数声明**
	- 无参形式
		`int main(void);     //在C99以后推荐使用该方法声明`
		`int main();`
	- 有参形式
		`int main(int argc, char *argv[])`
			`argc`：传递给程序的命令行参数的数量
			`argv`：指向字符串数组的指针，储存了命令行参数。==其中`argv[0]`指向了程序的名称，`argv[1]`到`argv[argc-1]`是实际的命令行参数==。
2. **`pid_t fork(void)`：创建一个子进程**
	- ==`#include <unistd.h>`==
	- 返回值
		`pid_t`：是一个int值的数字，代表进程的进程号`PID`。
			父进程中，返回子进程的`PID`
			在子进程中，返回0
			发生错误，返回<0
	- 相关函数
		`getpid()`：返回当前进程的`PID`
		`getppid()`：返回当前进程的父进程的`PID`
	- 调用`fork`函数之后，后边的代码会分别在父进程（当前调用fork函数的进程）与子进程（fork所创建的进程）中分别执行。故使用if判断从而在不同的进程中执行不同的操作，如：
		```C
		   //以下代码在一个单独的.c文件中，只是用了两个进程来执行
		   pit_t pid = fork();
			if(pid < 0){
			//发生错误
			}
			else if(pid == 0){
			//子进程代码
			}
			else{
			//父进程代码
			}
			//共同执行的代码
		   ```
		注：执行完`fork()`后，产生了一个子进程，在父进程中返回0，在子进程中返回>0的数，所以在两个进程中的`PID`是两个不同的变量，储存在不同的内存空间中，值也不同。
3. **`unsigned int sleep(unisigned int seconds)`**
	- 参数
		`seconds`：暂停的秒数
	- 返回值
		如果`sleep()`函数正常执行且休眠时间结束，则返回0；如果由于接收到信号而被提前唤醒，则返回剩余的休眠秒数
4. **`int execve(const char *filename, char *const argv[], char *const envp[])`：将当前进程正在执行的程序完全替换为一个新的程序**
	- ==它不是创建一个新进程，而是让**现有进程**“脱胎换骨”，其进程ID（PID）保持不变，但程序代码、数据、堆栈等都被新程序替换。==
	- `#include <unistd.h>`
	- 参数
		- `filename`: 要执行的新程序的**路径名**（必须是绝对路径或相对路径，不会在 `PATH` 中查找）。
		- `argv`: 传递给新程序的**参数列表**（argument vector）。这类似于 `main` 函数的 `argv`。惯例是 `argv[0]` 应该是程序名本身。
		- `envp`: 传递给新程序的**环境变量列表**（environment vector）。这是一个以 `NULL` 结尾的字符串数组，每个字符串的格式通常是 `KEY=VALUE`。
	- 返回值
		- 成功时，**`execve` 不会返回**，因为调用它的进程已经被新程序取代。
		- 失败时，返回 `-1` 并设置 `errno`，此时原程序继续执行。
	- 例
		```C
			char *argv[] = {
			"新进程的路径名", 
			"参数1", "参数2", "参数3" , ...
			NULL};
	    char *envp[] = {"PATH=...", NULL};
	    int a = execve(argv[0], argv, envp);
	    if(a == -1){
	        printf("error!!\n");
	    }
			```
5. **`pid_t waitpid(pid_t pid, int *wstatus, int options)`：等待进程**
	- 参数
		`pid`：要等待进程的`PID`
		`*wstatus`：等待进程的状态码，子进程返回的状态码会保存到该int
		`option`：填0
	- 返回值
		成功等到子进程停止返回`pid`
		出错返回-1
### 四、进程树
	Linux的进程是通过父子关系组织起来的，所有进程之间的父子关系共同构成了进程树（Process Tree）。进程树中每个节点都是其上级节点的子进程，同时又是子结点的父进程。一个进程的父进程只能有一个，而一个进程的子进程可以不止一个
### 五、孤儿进程
	孤儿进程（Orphan Process）是指父进程已结束或终止，而它仍在运行的进程。当父进程结束之前没有等待子进程结束，且父进程先于子进程结束时，那么子进程就会变成孤儿进程。
- 我们可以得出结论：==孤儿进程会被其祖先自动领养==。此时的子进程因为和终端切断了联系，所以==很难再进行标准输入使其停止了==，所以写代码的时候一定要注意避免出现孤儿进程。
### 六、进程间的通信
- 进程之前的内存是隔离的，如果多个进程之间需要进行信息交换，常用的方法有以下几种：
	- Unix Domain Socket IPC
	- 管道（有名管道、无名管道）
	- 共享内存
	- 消息队列
	- 信号量
1. **匿名管道（pipe）**
	匿名管道是位于**内核**的一块缓冲区，用于进程间通信，**只能用于父子进程之间的数据传输**，使用以下库函数以创建匿名管道：
	**`int pipe (int pipefd[2])`**
	- 参数
		- `pipefd[0]`：指向匿名管道的读端的文件描述符
		- `pipefd[1]`：指向匿名管道的写端的文件描述符
	- 返回值
		成功返回0，不成功返回1
	- 注：
		- 调用pipe函数后会将局部变量转变成文件描述符，同时创建两个新的**文件描述**，并由这两个文件描述符指向
		- 创建子进程后子进程也共享这两个文件描述符。
		- 父进程使用write向管道中写入数据，子进程使用read从管道中读出数据
	- 例：
		``` C
		int pipefd[2];//定义局部变量
		pipe(pipefd);//创建管道
		cpid = fork();//创建子进程
		if (cpid == 0) {
			close(pipefd[1]);
			//从pipefd[0]中读取数据
			close(pipefd[0]);
		}
		else{
			close(pipefd[0]);
			//向pipefd[1]中写入数据
			close(pipefd[1]);
		}
			```
2. **`read`的阻塞**
	1. 匿名管道
		==在默认情况下，`read` 函数是阻塞式的==
		- 管道中有数据：`read` 会立即返回，读取到的数据量最多为请求的字节数
		- 管道中无数据，但**仍有写入端**（写端文件描述符未全部关闭）：`read` 调用会阻塞，直到有数据可读
		- 管道中无数据，且**所有写入端都已关闭**：`read` 调用会**立即返回 0**，表示读到了“文件结束符”（EOF）
		也可以设置为非阻塞的
		- `fcntl(fd, F_SETFL, O_NONBLOCK)`：将管道的读端设置为非阻塞模式
		- 管道中有数据：行为与阻塞模式一样，立即返回数据
		- 管道中无数据，但仍有写入端：`read` 会**立即返回 -1**，并设置错误码 `errno`
		- 管道中无数据，且**所有写入端都已关闭**：同样会立即返回 0，表示 EOF
	2. 普通文件
		`read` 函数通常是非阻塞式的
	3. 命名管道
		==在默认情况下，`read` 函数是阻塞式的==
3. **有名管道（FIFO）**
	- 位于**内核**的一块缓冲区，用于进程间通信，**可以用于任何进程之间的通信**，使用以下库函数以创建匿名管道：
		`int mkfifo (const char *pathname, mode_t mode)`
		- 参数
			- `char *pathname`：有名管道的文件路径
			- `mode_t mode`：有名管道的文件的权限，一般为`0664`
		- 返回值
			成功返回0，不成功返回1
		- 例
			``` C
			char *pipe_path = "/tmp/myfifo";
			mkfifo(pipe_path, 0664)
			
			//*******进程A打开有名管道用于写入**********
			fd = open(pipe_path, O_WRONLY);
			//***************************************
			
			//*******进程B打开有名管道用于读取**********
			fd = open(pipe_path, O_RDONLY);
			//***************************************
			```
4. **`open`的阻塞**
	当你使用 `open(pipe_path, O_WRONLY);` **以只写方式**打开一个命名管道时，内核会使这个 `open()` 调用**阻塞（Block）**，直到有另一个进程以**只读方式 (`O_RDONLY`)** 打开同一个命名管道。
	**换句话说：一个命名管道必须同时有“读者”和“写者”两端都准备好，通信通道才能建立。只有一端是无法进行工作的**
	1. `O_RDONLY` (只读打开)：**阻塞**，直到有进程以**只写**方式打开
	2. `O_WRONLY` (只写打开)：**阻塞**，直到有进程以**只读**方式打开
	3. `O_RDWR` (读写打开)：**立即成功返回**，**不阻塞**
5. **共享内存**
	不使用宝贵的内核空间，在**内存**中开辟出一块内存共享对象，我们可以使用文件描述符来使用这块内存对象
	- **`int shm_open(const char *name, int oflag, mode_t mode)`：创建或打开一个==存在于内存中的共享内存对象**==
		- 头文件
			`#include <sys/mman.h> <sys/stat.h> <fcntl.h>`
		- 参数
			- `char *name`：是共享内存对象的名称，是一个字符串指针，格式：`"/名称"`
			- `int oflag`：打开模式
				- `O_CREAT`：如果不存在则创建新的共享内存对象
				- `O_EXCL`：当与`O_CREAT`一起使用时，如果共享内存对象已经存在，则返 回错误（避免覆盖现有对象）
				- `O_RDONLY`：以只读方式打开
				- `O_RDWR`：以读写方式打开
				- `O_TRUNC` 用于截断现有对象至0长度（只有在打开模式中包含`O_RDWR`时 才有效）
			- `mode_t mode`：类似于文件的权限模式，一般`0644`即可
		- 返回值
			- 成功执行它将返回一个新的文件描述符
			- 发生错误返回值为 -1
	- **`int shm_unlink(const char *name)`：删除一个先前由`shm_open()`创建的命名共享内存对象**
		- 参数
			`char *name`：要删除的共享内存对象名称
		- 返回值
			- 成功返回0
			- 失败返回-1
	- **`int truncate(const char *path, off_t length)`：将指定文件扩展或截取到指定大小；`int ftruncate(int fd, off_t length)`将指定文件描述符扩展或截取到指定大小**
		- 参数
			- `char *path`：文件名，**不需要打开**
			- `int fd`： 文件描述符，**需要打开并且有写权限**
			- `off_t length`：指定长度，单位字节
		- 返回值
			- 成功返回0
			- 失败返回1
	- **`void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)`：将一组设备或者文件映射到内存地址**
		将一个文件（或其它资源）的一部分或全部直接映射到调用**进程的虚拟地址空间**。之后，进程就可以像访问普通内存一样，**通过指针**来读写这个文件，而无需使用 `read()` 或 `write()` 等系统调用。
		- 头文件：`<sys/mman.h>`
		- 参数
			- `void *addr`：指向期望映射的内存起始地址的指针，**通常设为`NULL`**，让系统选择合适的地址
			- `size_t length`：要映射的内存区域的长度，以字节为单位
			- `int prot`：内存映射区域的保护标志，可以是以下标志的组合
				- `PROT_READ`：允许读取映射区域
				- `PROT_WRITE`：允许写入映射区域
				- `PROT_EXEC`：允许执行映射区域
				- `PROT_NONE`：页面不可访问
			- `int flags`：映射选项标志
				- `MAP_SHARED`：映射区域是共享的，对映射区域的修改会影响文件和其他映射到同一区域的进程，**一般使用共享**
				- `MAP_PRIVATE`：映射区域是私有的，对映射区域的修改不会影响原始文件，对文件的修改会被暂时保存在一个私有副本中
				- `MAP_ANONYMOUS`：创建一个匿名映射，不与任何文件关联
				- `MAP_FIXED`：强制映射到指定的地址，如果不允许映射，将返回错误
			- `int fd`：文件描述符，用于指定要映射的文件或设备，如果是匿名映射，则传入无效的文件描述符（例如-1）
			- `off_t offset`：从文件开头的偏移量，映射开始的位置
		- 返回值
			- 成功时，返回映射区域的起始地址，一般为`NULL`
			- 如果出错，返回 `(void *) -1`，并且设置`errno` 变量来表示错误原因
	- `int munmap(void *addr, size_t length)`：用于取消之前通过`mmap()`函数建立的内存映射关系
		- 头文件：`<sys/mman.h>`
		- 参数
			- `void *addr`: 这是指向之前通过`mmap()` 映射的内存区域的起始地址的指针，这个地址必须是有效的，并且必须是`mmap()` 返回的有效映射地址
			- `* size_t length`：这是要解除映射的内存区域的大小（以字节为单位），它必须与之前通过`mmap()`映射的大小一致
		- 返回值
			- 成功 0
			- 失败-1
6. **信号**
	在 Linux 中，信号是一种用于通知进程发生了某种事件的机制。信号可以由内核、其他进程或者通过命令行工具发送给目标进程。Linux 系统中有多种信号，每种信号都用一个唯一的整数值来表示，使用以下函数来创建对特定信号进行处理：
	`sighandler_t signal(int signum, sighandler_t handler)`
	- 头文件
		`#include <signal.h>`
	- 参数
		- `int signum`：要处理的信号
		* `sighandler_t handler`：当收到对应的信号时，要调用的函数
	- 返回值
		- 成功返回之前的信号处理函数
		- 失败返回`SIG_ERR`
## §2.7：线程处理
### 一、线程

|特性|进程 (Process)|线程 (Thread)|
|---|---|---|
|**基本单位**|**资源分配**的基本单位|**CPU调度和执行**的基本单位|
|**内存空间**|**独立**的地址空间|**共享**其所属进程的地址空间|
|**数据共享**|复杂，需要 **IPC** 机制|简单，可直接共享**全局变量**|
|**创建/切换开销**|**大**|**小**（因此称为“轻量级进程”）|
|**独立性/稳定性**|**高**，一个进程崩溃不影响其他进程|**低**，一个线程崩溃会导致整个进程崩溃|
|**通信速度**|**慢**（需要经过内核）|**快**（直接内存访问）|
|**同步需求**|不需要（默认隔离）|**强烈需要**（互斥锁、信号量等）|
|**资源拥有**|拥有系统资源（内存、I/O等）|只拥有必不可少的资源（如栈、寄存器）|
### 二、线程的创建与删除
1. **`int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)`**
	- 头文件
		`#include <pthread.h>`
	- 参数
		- `pthread_t *thread`：指向线程标识符的指针，存储新创建线程的**线程标识符**
		- `const pthread_attr_t *attr`：这个参数可以用来设置线程的属性，如优先级、栈大小等。如果不需要定制线程属性，**可以传入NULL**，此时线程将采用默认属性
		- `void *(*start_routine)(void *)`：一个指向函数的指针，它定义了新线程开始执行时的入口点。这个函数必须接受一个`void *`类型的参数，并返回`void *`类型的结果
		- `void *arg: start_routine`函数的参数，可以是一个指向任意类型数据的指针
	- 返回值
		- 成功返回0
		- 失败返回非0
	- 例
		``` C
		void *thread1(void *argv){
			
		}
		int main(void){
			pthread_t pid_thread1;
			pthread_create(&pid_thread1, NULL, thread1, NULL);
			...
			
			pthread_join(pid_thread1, NULL);
		}
		```
2. **`void pthread_exit(void *retval)：在进程内运行用于结束关闭调用该线程**
	- 参数
		`void *retval`：要返回给其它线程的数据
3. **`int pthread_join(pthread_t thread, void **retval)等待指定线程结束，获取目标线程的返回值，并在目标线程结束后回收它的资源**
	- 参数
		`pthread_t thread`：指定线程 ID
		`void **retval`： 这是一个可选参数，用于接收线程结束后传递的返回值。如果非空，pthread_join 会在成功时将线程的 exit status 复制到 *retval 所指向的内存位置。如果线程没有显式地通过 pthread_exit 提供返回值，则该参数将被设为 NULL 或忽略
4. **`int pthread_detach(pthread_t thread)：将线程标记为 detached 状态**
5. **`int pthread_cancel(pthread_t thread)`：向目标线程发送取消请求。目标线程是否和何时响应取决于它的取消状态和类型**
6. `int pthread_setcancelstate(int state, int *oldstate)
7. `int pthread_setcanceltype(int type, int *oldtype)`

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
# 第五章 Linux镜像构建
## 一、三个核心组成部分
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
### 手动从零构建（深入学习）
- 编译 Bootloader
- 编译 Linux 内核
- 构建根文件系统
- 组装
-
``` bash
# 设置启动参数

setenv bootargs console=ttyPS0,115200 root=/dev/mmcblk0p2 rootwait

# 加载 FIT 镜像

fatload mmc 0:1 0x10000000 fit.itb

# 启动内核

bootm 0x10000000
```
## 二、LubanCat_Gen_SDK
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
	
i2ctransfer -y 6 w2@0x2c 0x00 0x0D r1
	
	
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
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=test.raw
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