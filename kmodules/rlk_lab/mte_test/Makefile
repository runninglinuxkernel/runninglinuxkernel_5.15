
GNU ?= gcc

COPS = -g --static -march=armv8.5-a+memtag

all : mte_test

%_c.o:%.c
	$(GNU) $(COPS) -c $< -o $@

%_s.o:%.S
	$(GNU) $(COPS) -c $< -o $@

C_FILES = $(wildcard *.c)
ASM_FILES = $(wildcard *.S)
OBJ_FILES = $(C_FILES:%.c=%_c.o)
OBJ_FILES += $(ASM_FILES:%.S=%_s.o)

mte_test: $(OBJ_FILES)
	$(GNU) -o mte_test  $(OBJ_FILES)

clean:
	rm *.o
	rm mte_test
