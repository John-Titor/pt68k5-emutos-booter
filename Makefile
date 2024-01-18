# Build rules

BUILDDIR	 = build

TOOLPREFIX	 = m68k-elf
CC		 = $(TOOLPREFIX)-gcc

CFLAGS		 = -m68020 \
		   -Os \
		   -nostdlib \
		   -ffreestanding \
		   -ffunction-sections \
		   -fdata-sections \
		   -flto=auto \
		   -Ilib \
		   -I. \
		   -estart

LIB_SRCS	 = $(wildcard lib/*.[cS])

FATFS_SRCS	 = $(wildcard fatfs/*.c)

BOOT1		 = $(BUILDDIR)/boot1.bin
BOOT1_SRCS	 = $(wildcard boot1/*.[cS]) \
		   $(FATFS_SRCS) \
		   $(LIB_SRCS)


all: $(BOOT1)

$(BOOT1): $(BOOT1_SRCS) $(MAKEFILE_LIST)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wl,--oformat=binary -o $@ $(BOOT1_SRCS)

clean:
	rm -rf $(BUILDDIR)
