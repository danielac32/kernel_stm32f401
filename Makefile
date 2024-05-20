
FLASH       ?= st-flash
TOOLSET     ?= arm-none-eabi-
CC           = $(TOOLSET)gcc
LD           = $(TOOLSET)gcc
AR           = $(TOOLSET)gcc-ar
OBJCOPY      = $(TOOLSET)objcopy
OPTFLAGS    ?= -Og
STRIP=arm-none-eabi-strip


RM = rm -f
fixpath = $(strip $1)

 
CFLAGS      ?= -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffreestanding -nostdlib -nostartfiles  -fno-builtin
LDFLAGS      = -ffreestanding -nostdlib -nostartfiles  -fno-builtin -I include  
INCLUDES     =   -I include -I stm32lib -I gpio/Inc -I fat32/Inc -I spi/Inc -I w25q/Inc -I usb/Inc -I usb/class
CFLAGS2     ?= $(CFLAGS) -mthumb $(OPTFLAGS)
LDSCRIPT     =  ld.script

 
OBJDIR       = obj
SOURCES      =   $(wildcard system/*.c) $(wildcard system/*.S)
OBJECTS      = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SOURCES)))))
SRCLIB         = $(wildcard lib/*.c)
LIBOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCLIB)))))
SRCSHELL         = $(wildcard shell/*.c)
SHELLOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCSHELL)))))
SRCDEVICE         = $(wildcard device/nam/*.c) $(wildcard device/tty/*.c) $(wildcard device/led/*.c) $(wildcard device/flash/*.c)
DEVICEOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCDEVICE)))))

SRCGPIO         = $(wildcard gpio/Src/*.c)
GPIOOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCGPIO)))))

SRCUSB         = $(wildcard usb/Src/*.c) $(wildcard usb/msc/*.c) $(wildcard usb/cdc/*.c)
USBOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCUSB)))))

SRCSPI         = $(wildcard spi/Src/*.c)
SPIOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCSPI)))))


SRCW25Q         = $(wildcard w25q/Src/*.c)
W25QOBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCW25Q)))))

SRCFAT32         = $(wildcard fat32/Src/*.c)
FAT32OBJ         = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCFAT32)))))



DOUT         = kernel


SRCPATH = $(sort $(dir $(SOURCES) $(SRCLIB) $(SRCSHELL) $(SRCDEVICE) $(SRCSPI) $(SRCFAT32) $(SRCW25Q) $(SRCGPIO) $(SRCUSB) ))
vpath %.c $(SRCPATH)
vpath %.S $(SRCPATH)


 
$(OBJDIR):
	@mkdir $@

flash:
	#st-flash --reset --format ihex write kernel.hex
	st-flash write $(DOUT).bin 0x08010000
	st-flash reset
reset:
	st-flash reset


demo: $(DOUT).bin
		arm-none-eabi-objdump -d kernel.elf > kernel.list


	
$(DOUT).bin : $(DOUT).elf
	@echo building $@
	@$(OBJCOPY) -O binary $< $@



$(DOUT).elf : $(OBJDIR) $(LIBOBJ) $(SHELLOBJ) $(OBJECTS) $(DEVICEOBJ) $(GPIOOBJ) $(USBOBJ) $(SPIOBJ) $(FAT32OBJ) $(W25QOBJ)
	@echo building $@
	@$(LD) $(CFLAGS2) $(LDFLAGS) -Wl,--script='$(LDSCRIPT)' $(LIBOBJ) $(SHELLOBJ) $(DEVICEOBJ) $(OBJECTS) $(GPIOOBJ) $(USBOBJ) $(SPIOBJ) $(FAT32OBJ) $(W25QOBJ)  -o $@

clean: $(OBJDIR)
	$(MAKE) --version
	@$(RM) $(DOUT).*
	@$(RM) $(call fixpath, $(OBJDIR)/*.*)


$(OBJDIR)/%.o: %.S
	@echo assembling $<
	@$(CC) $(CFLAGS2)  $(INCLUDES) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@echo compiling $<
	@$(CC) $(CFLAGS2)  $(INCLUDES) -c $< -o $@
