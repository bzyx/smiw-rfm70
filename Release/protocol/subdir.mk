################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../protocol/_main.c \
../protocol/protocol.c 

OBJS += \
./protocol/_main.o \
./protocol/protocol.o 

C_DEPS += \
./protocol/_main.d \
./protocol/protocol.d 


# Each subdirectory must supply rules for building sources it contributes
protocol/%.o: ../protocol/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


