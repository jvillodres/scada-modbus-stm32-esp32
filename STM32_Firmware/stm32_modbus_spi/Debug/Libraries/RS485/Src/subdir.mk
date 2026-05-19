################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/RS485/Src/rs485.c 

C_DEPS += \
./Libraries/RS485/Src/rs485.d 

OBJS += \
./Libraries/RS485/Src/rs485.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/RS485/Src/%.o Libraries/RS485/Src/%.su Libraries/RS485/Src/%.cyclo: ../Libraries/RS485/Src/%.c Libraries/RS485/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Asus/Documents/Universidad/UTP/Sistemas Embebidos/scada-modbus-stm32-esp32/STM32_Firmware/stm32_modbus_spi/Libraries/Modbus/Inc" -I"C:/Users/Asus/Documents/Universidad/UTP/Sistemas Embebidos/scada-modbus-stm32-esp32/STM32_Firmware/stm32_modbus_spi/Libraries/RS485/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Libraries-2f-RS485-2f-Src

clean-Libraries-2f-RS485-2f-Src:
	-$(RM) ./Libraries/RS485/Src/rs485.cyclo ./Libraries/RS485/Src/rs485.d ./Libraries/RS485/Src/rs485.o ./Libraries/RS485/Src/rs485.su

.PHONY: clean-Libraries-2f-RS485-2f-Src

