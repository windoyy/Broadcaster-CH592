################################################################################
# MRS Version: 1.9.2
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RVMSIS/core_riscv.c 

OBJS += \
./RVMSIS/core_riscv.o 

C_DEPS += \
./RVMSIS/core_riscv.d 


# Each subdirectory must supply rules for building sources it contributes
RVMSIS/core_riscv.o: C:/Users/44826/Desktop/Broadcaster-CH592/Broadcaster-CH592/RVMSIS/core_riscv.c
	@	@	"C:/MounRiver/MounRiver_Studio/toolchain/RISC-V Embedded GCC12/bin/riscv-none-elf-gcc" -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCLK_OSC32K=0 -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\Startup" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\APP\include" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\Profile\include" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\StdPeriphDriver\inc" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\HAL\include" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\Ld" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\LIB" -I"C:\Users\44826\Desktop\Broadcaster-CH592\Broadcaster-CH592\RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

