# OSH Lab2 实验报告

PB19051055 彭浩然

## 编译运行方式

该项目使用rust语言，使用cargo管理依赖等，在将工作目录切换到本文件所在的文件夹后，使用下面的命令编译运行：

```bash
cargo run
```

## 实现的功能

### 基础功能

#### 管道语法

```bash
hurrypeng:/home/hurrypeng/shell# env | wc
     42      69    4275
hurrypeng:/home/hurrypeng/shell# ls | cat -n
     1  Cargo.lock
     2  Cargo.toml
     3  functions.txt
     4  out.txt
     5  src
     6  target
hurrypeng:/home/hurrypeng/shell# ls | cat -n | grep Cargo
     1  Cargo.lock
     2  Cargo.toml
```

#### 基本的文件重定向

```bash
hurrypeng:/home/hurrypeng/shell# ls > out.txt
hurrypeng:/home/hurrypeng/shell# cat out.txt
Cargo.lock
Cargo.toml
functions.txt
out.txt
src
target
hurrypeng:/home/hurrypeng/shell# echo newCargoLine!!! >> out.txt
hurrypeng:/home/hurrypeng/shell# cat < out.txt
Cargo.lock
Cargo.toml
functions.txt
out.txt
src
target
newCargoLine!!!
hurrypeng:/home/hurrypeng/shell# cat < out.txt | cat -n | grep Cargo >> out.txt
hurrypeng:/home/hurrypeng/shell# cat out.txt
Cargo.lock
Cargo.toml
functions.txt
out.txt
src
target
newCargoLine!!!
     1  Cargo.lock
     2  Cargo.toml
     7  newCargoLine!!!
```

#### Ctrl+C处理

```bash
hurrypeng:/home/hurrypeng/shell# oopstypo^C
hurrypeng:/home/hurrypeng/shell# sleep 5
^Churrypeng:/home/hurrypeng/shell#
hurrypeng:/home/hurrypeng/shell#
```

#### Ctrl+D处理

```bash
hurrypeng:/home/hurrypeng/shell# bye!
```

### 拓展功能

#### 拓展的文件重定向

```bash
hurrypeng:/home/hurrypeng/shell# cat -n << End
> these
> lines
> are
> fed
> to
> cat
> oops
> not
> fish
> End
     1  these
     2  lines
     3  are
     4  fed
     5  to
     6  cat
     7  oops
     8  not
     9  fish
hurrypeng:/home/hurrypeng/shell# wc <<< thoseLinesWereFedToCat
      0       1      23
```

#### 用户名和当前工作目录显示

```bash
hurrypeng:/home/hurrypeng/shell#
```

#### 环境变量

```bash
hurrypeng:/home/hurrypeng/shell# echo $HOME
/home/hurrypeng
hurrypeng:/home/hurrypeng/shell# export MYFILE=/home/hurrypeng/shell/myfile.txt
hurrypeng:/home/hurrypeng/shell# echo $MYFILE | cat -n > $MYFILE
hurrypeng:/home/hurrypeng/shell# cat myfile.txt
     1  /home/hurrypeng/shell/myfile.txt
hurrypeng:/home/hurrypeng/shell#
```

#### 别名

```bash
hurrypeng:/home/hurrypeng/shell# alias myecho='echo MYECHO!'
hurrypeng:/home/hurrypeng/shell# myecho
MYECHO!
```

## 使用 strace 工具追踪系统调用

执行命令`strace cat < out.txt | cat -n | grep Cargo >> out.txt`后，得到下列结果，其中省略了一些行：

```bash
execve("/bin/cat", ["cat"], 0x7fffcfacd480 /* 42 vars */) = 0
brk(NULL)                               = 0x7fffe3985000
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7fdb9f250000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/home/hurrypeng/shell/target/debug/deps/tls/haswell/x86_64/libc.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/hurrypeng/shell/target/debug/deps/tls/haswell/x86_64", 0x7fffeb6c3270) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/home/hurrypeng/shell/target/debug/deps/tls/haswell/libc.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/hurrypeng/shell/target/debug/deps/tls/haswell", 0x7fffeb6c3270) = -1 ENOENT (No such file or directory)
......
read(0, "Cargo.lock\nCargo.toml\nfunctions."..., 131072) = 269
write(1, "Cargo.lock\nCargo.toml\nfunctions."..., 269) = 269
read(0, "", 131072)                     = 0
munmap(0x7fdb9f030000, 139264)          = 0
close(0)                                = 0
close(1)                                = 0
close(2)                                = 0
exit_group(0)                           = ?
+++ exited with 0 +++
```

察阅一些系统调用的功能：

### execve()

将目前正在执行的进程替换成要执行的程序，相当于调用其他程序但不返回。

### mmap()

一种内存映射文件的方法，即将一个文件或者其它对象映射到进程的地址空间，实现文件磁盘地址和进程虚拟地址空间中一段虚拟地址的一一对映关系。

### stat()

取得指定文件的文件属性，包括设备编号、inode等信息，返回的信息存储在一个结构体中。

