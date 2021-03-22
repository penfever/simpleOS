################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Sources/FAT.c" \
"../Sources/SDHC_FAT32_Files.c" \
"../Sources/bootSector.c" \
"../Sources/delay.c" \
"../Sources/devices.c" \
"../Sources/fsInfo.c" \
"../Sources/led.c" \
"../Sources/main.c" \
"../Sources/microSD.c" \
"../Sources/myerror.c" \
"../Sources/mymalloc.c" \
"../Sources/pushbutton.c" \
"../Sources/simpleshell.c" \
"../Sources/uart.c" \
"../Sources/uartNL.c" \
"../Sources/univio.c" \
"../Sources/util.c" \

C_SRCS += \
../Sources/FAT.c \
../Sources/SDHC_FAT32_Files.c \
../Sources/bootSector.c \
../Sources/delay.c \
../Sources/devices.c \
../Sources/fsInfo.c \
../Sources/led.c \
../Sources/main.c \
../Sources/microSD.c \
../Sources/myerror.c \
../Sources/mymalloc.c \
../Sources/pushbutton.c \
../Sources/simpleshell.c \
../Sources/uart.c \
../Sources/uartNL.c \
../Sources/univio.c \
../Sources/util.c \

OBJS += \
./Sources/FAT.o \
./Sources/SDHC_FAT32_Files.o \
./Sources/bootSector.o \
./Sources/delay.o \
./Sources/devices.o \
./Sources/fsInfo.o \
./Sources/led.o \
./Sources/main.o \
./Sources/microSD.o \
./Sources/myerror.o \
./Sources/mymalloc.o \
./Sources/pushbutton.o \
./Sources/simpleshell.o \
./Sources/uart.o \
./Sources/uartNL.o \
./Sources/univio.o \
./Sources/util.o \

C_DEPS += \
./Sources/FAT.d \
./Sources/SDHC_FAT32_Files.d \
./Sources/bootSector.d \
./Sources/delay.d \
./Sources/devices.d \
./Sources/fsInfo.d \
./Sources/led.d \
./Sources/main.d \
./Sources/microSD.d \
./Sources/myerror.d \
./Sources/mymalloc.d \
./Sources/pushbutton.d \
./Sources/simpleshell.d \
./Sources/uart.d \
./Sources/uartNL.d \
./Sources/univio.d \
./Sources/util.d \

OBJS_QUOTED += \
"./Sources/FAT.o" \
"./Sources/SDHC_FAT32_Files.o" \
"./Sources/bootSector.o" \
"./Sources/delay.o" \
"./Sources/devices.o" \
"./Sources/fsInfo.o" \
"./Sources/led.o" \
"./Sources/main.o" \
"./Sources/microSD.o" \
"./Sources/myerror.o" \
"./Sources/mymalloc.o" \
"./Sources/pushbutton.o" \
"./Sources/simpleshell.o" \
"./Sources/uart.o" \
"./Sources/uartNL.o" \
"./Sources/univio.o" \
"./Sources/util.o" \

C_DEPS_QUOTED += \
"./Sources/FAT.d" \
"./Sources/SDHC_FAT32_Files.d" \
"./Sources/bootSector.d" \
"./Sources/delay.d" \
"./Sources/devices.d" \
"./Sources/fsInfo.d" \
"./Sources/led.d" \
"./Sources/main.d" \
"./Sources/microSD.d" \
"./Sources/myerror.d" \
"./Sources/mymalloc.d" \
"./Sources/pushbutton.d" \
"./Sources/simpleshell.d" \
"./Sources/uart.d" \
"./Sources/uartNL.d" \
"./Sources/univio.d" \
"./Sources/util.d" \

OBJS_OS_FORMAT += \
./Sources/FAT.o \
./Sources/SDHC_FAT32_Files.o \
./Sources/bootSector.o \
./Sources/delay.o \
./Sources/devices.o \
./Sources/fsInfo.o \
./Sources/led.o \
./Sources/main.o \
./Sources/microSD.o \
./Sources/myerror.o \
./Sources/mymalloc.o \
./Sources/pushbutton.o \
./Sources/simpleshell.o \
./Sources/uart.o \
./Sources/uartNL.o \
./Sources/univio.o \
./Sources/util.o \


# Each subdirectory must supply rules for building sources it contributes
Sources/FAT.o: ../Sources/FAT.c
	@echo 'Building file: $<'
	@echo 'Executing target #1 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/FAT.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/FAT.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/SDHC_FAT32_Files.o: ../Sources/SDHC_FAT32_Files.c
	@echo 'Building file: $<'
	@echo 'Executing target #2 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/SDHC_FAT32_Files.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/SDHC_FAT32_Files.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/bootSector.o: ../Sources/bootSector.c
	@echo 'Building file: $<'
	@echo 'Executing target #3 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/bootSector.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/bootSector.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/delay.o: ../Sources/delay.c
	@echo 'Building file: $<'
	@echo 'Executing target #4 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/delay.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/delay.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/devices.o: ../Sources/devices.c
	@echo 'Building file: $<'
	@echo 'Executing target #5 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/devices.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/devices.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/fsInfo.o: ../Sources/fsInfo.c
	@echo 'Building file: $<'
	@echo 'Executing target #6 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/fsInfo.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/fsInfo.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/led.o: ../Sources/led.c
	@echo 'Building file: $<'
	@echo 'Executing target #7 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/led.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/led.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/main.o: ../Sources/main.c
	@echo 'Building file: $<'
	@echo 'Executing target #8 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/main.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/main.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/microSD.o: ../Sources/microSD.c
	@echo 'Building file: $<'
	@echo 'Executing target #9 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/microSD.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/microSD.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/myerror.o: ../Sources/myerror.c
	@echo 'Building file: $<'
	@echo 'Executing target #10 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/myerror.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/myerror.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/mymalloc.o: ../Sources/mymalloc.c
	@echo 'Building file: $<'
	@echo 'Executing target #11 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/mymalloc.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/mymalloc.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/pushbutton.o: ../Sources/pushbutton.c
	@echo 'Building file: $<'
	@echo 'Executing target #12 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/pushbutton.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/pushbutton.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/simpleshell.o: ../Sources/simpleshell.c
	@echo 'Building file: $<'
	@echo 'Executing target #13 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/simpleshell.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/simpleshell.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/uart.o: ../Sources/uart.c
	@echo 'Building file: $<'
	@echo 'Executing target #14 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/uart.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/uart.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/uartNL.o: ../Sources/uartNL.c
	@echo 'Building file: $<'
	@echo 'Executing target #15 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/uartNL.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/uartNL.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/univio.o: ../Sources/univio.c
	@echo 'Building file: $<'
	@echo 'Executing target #16 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/univio.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/univio.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/util.o: ../Sources/util.c
	@echo 'Building file: $<'
	@echo 'Executing target #17 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/util.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/util.o"
	@echo 'Finished building: $<'
	@echo ' '


