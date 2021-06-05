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

