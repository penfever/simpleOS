################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Sources/delay.c" \
"../Sources/devices.c" \
"../Sources/main.c" \
"../Sources/myerror.c" \
"../Sources/mymalloc.c" \
"../Sources/simpleshell.c" \
"../Sources/uart.c" \
"../Sources/uartNL.c" \

C_SRCS += \
../Sources/delay.c \
../Sources/devices.c \
../Sources/main.c \
../Sources/myerror.c \
../Sources/mymalloc.c \
../Sources/simpleshell.c \
../Sources/uart.c \
../Sources/uartNL.c \

OBJS += \
./Sources/delay.o \
./Sources/devices.o \
./Sources/main.o \
./Sources/myerror.o \
./Sources/mymalloc.o \
./Sources/simpleshell.o \
./Sources/uart.o \
./Sources/uartNL.o \

C_DEPS += \
./Sources/delay.d \
./Sources/devices.d \
./Sources/main.d \
./Sources/myerror.d \
./Sources/mymalloc.d \
./Sources/simpleshell.d \
./Sources/uart.d \
./Sources/uartNL.d \

OBJS_QUOTED += \
"./Sources/delay.o" \
"./Sources/devices.o" \
"./Sources/main.o" \
"./Sources/myerror.o" \
"./Sources/mymalloc.o" \
"./Sources/simpleshell.o" \
"./Sources/uart.o" \
"./Sources/uartNL.o" \

C_DEPS_QUOTED += \
"./Sources/delay.d" \
"./Sources/devices.d" \
"./Sources/main.d" \
"./Sources/myerror.d" \
"./Sources/mymalloc.d" \
"./Sources/simpleshell.d" \
"./Sources/uart.d" \
"./Sources/uartNL.d" \

OBJS_OS_FORMAT += \
./Sources/delay.o \
./Sources/devices.o \
./Sources/main.o \
./Sources/myerror.o \
./Sources/mymalloc.o \
./Sources/simpleshell.o \
./Sources/uart.o \
./Sources/uartNL.o \


# Each subdirectory must supply rules for building sources it contributes
Sources/delay.o: ../Sources/delay.c
	@echo 'Building file: $<'
	@echo 'Executing target #1 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/delay.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/delay.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/devices.o: ../Sources/devices.c
	@echo 'Building file: $<'
	@echo 'Executing target #2 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/devices.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/devices.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/main.o: ../Sources/main.c
	@echo 'Building file: $<'
	@echo 'Executing target #3 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/main.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/main.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/myerror.o: ../Sources/myerror.c
	@echo 'Building file: $<'
	@echo 'Executing target #4 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/myerror.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/myerror.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/mymalloc.o: ../Sources/mymalloc.c
	@echo 'Building file: $<'
	@echo 'Executing target #5 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/mymalloc.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/mymalloc.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/simpleshell.o: ../Sources/simpleshell.c
	@echo 'Building file: $<'
	@echo 'Executing target #6 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/simpleshell.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/simpleshell.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/uart.o: ../Sources/uart.c
	@echo 'Building file: $<'
	@echo 'Executing target #7 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/uart.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/uart.o"
	@echo 'Finished building: $<'
	@echo ' '

Sources/uartNL.o: ../Sources/uartNL.c
	@echo 'Building file: $<'
	@echo 'Executing target #8 $<'
	@echo 'Invoking: ARM Ltd Windows GCC C Compiler'
	"$(ARMSourceryDirEnv)/arm-none-eabi-gcc" "$<" @"Sources/uartNL.args" -MMD -MP -MF"$(@:%.o=%.d)" -o"Sources/uartNL.o"
	@echo 'Finished building: $<'
	@echo ' '


