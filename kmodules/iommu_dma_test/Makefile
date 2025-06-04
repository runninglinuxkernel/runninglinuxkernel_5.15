
BASEINCLUDE ?= /lib/modules/`uname -r`/build

CONFIG_MODULE_SIG=n

iommu-test-objs := iommu_test.o 

obj-m	:=   iommu-test.o
all : 
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) modules;

clean:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) clean;
	rm -f *.ko;


