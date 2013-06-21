obj-m := kthread_test.o
user-obj := test_usr.o
CC ?= gcc

PWD := $(shell pwd)
KERNELDIR := /lib/modules/$(shell uname -r)/build

all: usr modules

usr: $(user-obj)
	$(CC) -o $@ $(user-obj)

modules:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.unsigned *.order usr
