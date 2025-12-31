ifneq ($(KERNELRELEASE),)
obj-m :=  dir.o
else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# 默认目标：编译内核模块和用户程序
all: modules user

# 编译内核模块
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 编译用户空间程序
user:
	gcc chat.c -o chat -lpthread
	@echo "用户程序编译完成：reader, writer, test, chat"

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f  chat 
	@echo "清理完成"
endif

