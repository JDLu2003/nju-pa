AM_SRCS := platform/nemu/trm.c \
           platform/nemu/ioe/ioe.c \
           platform/nemu/ioe/timer.c \
           platform/nemu/ioe/input.c \
           platform/nemu/ioe/gpu.c \
           platform/nemu/ioe/audio.c \
           platform/nemu/ioe/disk.c \
           platform/nemu/mpe.c

CFLAGS    += -fdata-sections -ffunction-sections
CFLAGS    += -I$(AM_HOME)/am/src/platform/nemu/include
LDSCRIPTS += $(AM_HOME)/scripts/linker.ld
LDFLAGS   += --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0
LDFLAGS   += --gc-sections -e _start

# 默认的 DIFTEST 共享库路径 (请根据你的实际 NEMU 编译路径调整)
# 这个路径指向你编译出来的 libriscv.so 或 riscv32-spike-so
# 如果你在NEMU_HOME/tools/spike-diff/build/riscv32-spike-so 找不到，请检查你difftest的编译配置
DIFF_SO_FILE ?= $(NEMU_HOME)/tools/spike-diff/build/riscv32-spike-so

# 默认的 NEMU 日志路径
NEMU_LOG_FILE ?= $(shell dirname $(IMAGE).elf)/nemu-log.txt

# 用于传递给 NEMU 的完整参数字符串
# 始终包含 --log
NEMU_ALL_ARGS = --log=$(NEMU_LOG_FILE)

# 只有当 NEMU 配置了 CONFIG_DIFFTEST=y 时才添加 --diff 参数
ifeq ($(shell grep -q '^CONFIG_DIFFTEST=y' $(NEMU_HOME)/.config 2>/dev/null && echo y), y)
  NEMU_ALL_ARGS += --diff=$(DIFF_SO_FILE)
endif

# 将从 am-kernels 传递下来的额外 NEMUFLAGS (例如 -b) 添加进去
NEMU_ALL_ARGS += $(NEMUFLAGS)

# 最后添加 -e 和 ELF 路径
NEMU_ALL_ARGS += -e $(IMAGE).elf


MAINARGS_MAX_LEN = 64
MAINARGS_PLACEHOLDER = the_insert-arg_rule_in_Makefile_will_insert_mainargs_here
CFLAGS += -DMAINARGS_MAX_LEN=$(MAINARGS_MAX_LEN) -DMAINARGS_PLACEHOLDER=$(MAINARGS_PLACEHOLDER)

insert-arg: image
	@python $(AM_HOME)/tools/insert-arg.py $(IMAGE).bin $(MAINARGS_MAX_LEN) $(MAINARGS_PLACEHOLDER) "$(mainargs)"

image: image-dep
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin

run: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMU_ALL_ARGS)" IMG=$(IMAGE).bin

gdb: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMU_ALL_ARGS)" IMG=$(IMAGE).bin

.PHONY: insert-arg
