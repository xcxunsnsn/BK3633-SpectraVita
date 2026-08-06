
BUILD_DIR = build
TARGET    = BK3633_GATT

AR = arm-none-eabi-gcc-ar
CC = arm-none-eabi-gcc

CCFLAGS  = -Os -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -ffunction-sections -fdata-sections
CCFLAGS += -flto -fno-strict-aliasing -fgnu89-inline -ffast-math -fmessage-length=0
CCFLAGS += -Wall
CCFLAGS += -DCONFIG_AOS_CLI

LKFLAGS  = -Os -mcpu=arm968e-s -mthumb -mthumb-interwork -flto -ffunction-sections -fdata-sections
LKFLAGS += -nostartfiles


CCFLAGS += -I../../src/ip/ble/ll/api \
      -I../../src/ip/ble/ll/import/reg \
      -I../../src/ip/ble/ll/src \
      -I../../src/ip/bt/api \
      -I../../src/ip/bt/import/reg \
      -I../../src/ip/bt/src \
      -I../../src/ip/em/api \
      -I../../src/ip/hci/api \
      -I../../src/ip/sch/api \
      -I../../src/ip/sch/import \
      -I../../src/modules/aes/api \
      -I../../src/modules/common/api \
      -I../../src/modules/dbg/api \
      -I../../src/modules/ecc_p256/api \
      -I../../src/modules/h4tl/api \
      -I../../src/modules/ke/api \
      -I../../src/plf/nvds/api \
      -I../../src/driver/rf \
      -I../../src/plf/rwip/api \
      -I../../src/plf/rwip/import/reg \
      -I../../src/plf/refip/import/reg \
      -I../../src/plf/refip/src/arch \
      -I../../src/plf/refip/src/arch/boot/rvds \
      -I../../src/plf/refip/src/arch/compiler/armgcc \
      -I../../src/plf/refip/src/arch/ll/armgcc \
      -I../../src/driver/flash \
      -I../../src/driver/led \
      -I../../src/driver/reg \
      -I../../src/driver/syscntl \
      -I../../src/driver/timer \
      -I../../src/driver/uart \
      -I../../src/driver/uart2 \
      -I../../src/header \
      -I../../src/header/header_file \
      -I../../src/plf/refip/src/driver \
      -I../../src/ip/ble/hl/api \
      -I../../src/ip/ble/hl/inc \
      -I../../src/ip/ble/hl/src/gap/gapc \
      -I../../src/ip/ble/hl/src/gap/gapm \
      -I../../src/ip/ble/hl/src/gatt \
      -I../../src/ip/ble/hl/src/gatt/attc \
      -I../../src/ip/ble/hl/src/gatt/attm \
      -I../../src/ip/ble/hl/src/gatt/atts \
      -I../../src/ip/ble/hl/src/gatt/gattc \
      -I../../src/ip/ble/hl/src/gatt/gattm \
      -I../../src/ip/ble/hl/src/l2c/l2cc \
      -I../../src/ip/ble/hl/src/l2c/l2cm \
      -I../../src/app/api \
      -I../../src/profiles/dis/diss/api \
      -I../../src/profiles/fee0/api \
      -I../../src/profiles/bas/bass/api \
      -I../../src/profiles/fcc0_128/api \
      -I../../src/driver/gpio \
      -I../../src/driver/spi \
      -I../../src/driver/icu \
      -I../../src/driver/wdt \
      -I../../src/driver/adc \
      -I../../src/profiles/ancs/ancsc/api \
      -I../../src/profiles/oad/api \
      -I../../src/profiles/ancs/ancsc \
      -I./config \
      -I./app/api \
      -I../../src/driver/aon_rtc \
      -I../../src/plf/prf \
      -I../../Src/driver/intc \
      -I../../src/driver/dma \
      -I../../src/driver/i2c \
      -I../../src/device/lcd \
      -I../../src/profiles/hogp/hogpd/api \
      -I../../src/profiles/hogp \
      -I../../src/driver/pwm  \
      -I../../src/driver/intc \
      -I../../src/driver/i2s

LKFLAGS += -L../../libs
LKFLAGS += -T../../src/system/link_app.lds
LKFLAGS += -Wl,--gc-sections
LKFLAGS += -Wl,--just-symbols=../../src/system/bk3633_stack_gcc.elf
LKFLAGS += --specs=nosys.specs

ASMS = 
ASM1 = 
ASM2 = src/system/startup_boot.S
SRCS = app/src/main.c \
       app/src/app_sec.c \
       app/src/app.c \
       app/src/app_batt.c \
       app/src/app_dis.c \
       app/src/app_oads.c \
       app/src/app_task.c \
       app/src/app_fcc0.c \
       app/src/app_fee0.c \
       
SRC1 =     
SRC2 = src/plf/rwip/src/rwbt.c \
       src/plf/rwip/src/rwble.c \
       src/plf/rwip/src/rwip.c \
       src/plf/nvds/src/nvds.c \
       src/plf/prf/prf_utils.c \
       src/plf/prf/prf.c \
       src/driver/icu/icu.c \
       src/driver/intc/intc.c \
       src/driver/i2s/i2s.c \
       src/driver/uart/uart.c \
       src/driver/uart2/uart2.c \
       src/driver/flash/flash.c \
       src/driver/rf/rf_xvr.c \
       src/driver/wdt/wdt.c \
       src/driver/dma/dma.c \
       src/driver/spi/spi.c \
       src/driver/aon_rtc/aon_rtc.c \
       src/driver/gpio/gpio.c \
       src/driver/pwm/pwm.c \
       src/profiles/bas/bass/src/bass.c \
       src/profiles/bas/bass/src/bass_task.c \
       src/profiles/dis/diss/src/diss.c \
       src/profiles/dis/diss/src/diss_task.c \
       src/profiles/fee0/src/fee0s.c \
       src/profiles/fee0/src/fee0s_task.c \
       src/profiles/fcc0_128/src/fcc0s.c \
       src/profiles/fcc0_128/src/fcc0s_task.c \
       src/profiles/hogp/hogpd/src/hogpd.c \
       src/profiles/hogp/hogpd/src/hogpd_task.c \
       src/profiles/oad/src/oads.c \
       src/profiles/oad/src/oads_task.c 
   
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS += $(SRC1:%.c=$(BUILD_DIR)/dd1/%.o)
OBJS += $(SRC2:%.c=$(BUILD_DIR)/dd2/%.o)
OBJS += $(ASMS:%.S=$(BUILD_DIR)/%.o)
OBJS += $(ASM1:%.S=$(BUILD_DIR)/dd1/%.o)
OBJS += $(ASM2:%.S=$(BUILD_DIR)/dd2/%.o)
DEPS := $(OBJS:.o=.d)

target: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf : $(OBJS)
    @echo Build target: $@
    @$(CC) $(LKFLAGS) -o $@ $(OBJS) $(LKLIBS)
    @echo 'Finished building target: $@'
    @echo ' '

%.bin : %.elf
    @echo Build target: $@
    @arm-none-eabi-objcopy -O binary $< $@
    @arm-none-eabi-objdump -date     $<    > $(<:%.elf=%.dmp)
    @arm-none-eabi-nm                $<    > $(<:%.elf=%.map)
    @mkdir -p ./output/app
ifeq ($(OS),Windows_NT)
    @arm-none-eabi-readelf.exe       $< -a > $(<:%.elf=%.txt)
    
    @cp -R ./build/BK3633_GATT.bin ./output/app
    @./output/BinConvert.exe  -oad ./output/boot/BK3633_BIM.bin  ./output/stack/bk3633_stack_gcc.bin ./output/app/BK3633_GATT.bin -m 0x1F00 -l 0x25b00 -v 0x1220 -rom_v 0x1235 -e 00000000 00000000 00000000 00000000

else


    @arm-none-eabi-readelf           $< -a > $(<:%.elf=%.txt)
    @cp -R ./build/BK3633_GATT.bin ./output/app
    @./BinConvert  -oad ./output/boot/BK3633_BIM.bin  ./output/stack/bk3633_stack_gcc.bin ./output/app/BK3633_GATT.bin -m 0x1F00 -l 0x25b00 -v 0x1220 -rom_v 0x1235 -e 00000000 00000000 00000000 00000000
endif

    @echo 'Finished building target: $@'
    @echo ' '

clean:
    @echo Clean project: $(TARGET)
    @-rm -rf $(OBJS) $(DEPS) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin
    -@echo ' '

prev-build:
    -@echo 'Pre-building'
    -@rm -rf $(BUILD_DIR)/$(TARGET)*.*
    -@echo ' '

post-build:
    -@echo 'Post-building'
    -@echo ' '

$(BUILD_DIR)/%.o : %.c
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

$(BUILD_DIR)/dd1/%.o : ../%.c
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

$(BUILD_DIR)/dd2/%.o : ../../%.c
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

$(BUILD_DIR)/%.o : %.S
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

$(BUILD_DIR)/dd1/%.o : ../%.S
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

$(BUILD_DIR)/dd2/%.o : ../../%.S
    @echo Compiling $<
    @mkdir -p $(dir $@)
    @$(CC) -c $(CCFLAGS) -MMD -MP -MF$(@:%.o=%.d) -MT$(@) $< -o $@

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif

.PHONY : all clean pre-build post-build
