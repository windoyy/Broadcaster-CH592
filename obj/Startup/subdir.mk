################################################################################
# MRS Version: 1.9.2
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../Startup/startup_CH592.S 

OBJS += \
./Startup/startup_CH592.o 

S_UPPER_DEPS += \
./Startup/startup_CH592.d 


# Each subdirectory must supply rules for building sources it contributes
Startup/startup_CH592.o: C:/Users/44826/Desktop/Broadcaster-CH592/Broadcaster-CH592/Startup/startup_CH592.S
	@	@	"C:/MounRiver/MounRiver_Studio/toolchain/RISC-V Embedded GCC12/bin/riscv-none-elf-gcc" -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

