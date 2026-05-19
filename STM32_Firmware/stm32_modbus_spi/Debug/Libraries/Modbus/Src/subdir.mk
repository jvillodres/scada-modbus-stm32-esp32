################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Libraries/Modbus/Src/modbus.cpp 

OBJS += \
./Libraries/Modbus/Src/modbus.o 

CPP_DEPS += \
./Libraries/Modbus/Src/modbus.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/Modbus/Src/%.o Libraries/Modbus/Src/%.su Libraries/Modbus/Src/%.cyclo: ../Libraries/Modbus/Src/%.cpp Libraries/Modbus/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Asus/Documents/Universidad/UTP/Sistemas Embebidos/scada-modbus-stm32-esp32/STM32_Firmware/stm32_modbus_spi/Libraries/RS485/Inc" -I"C:/Users/Asus/Documents/Universidad/UTP/Sistemas Embebidos/scada-modbus-stm32-esp32/STM32_Firmware/stm32_modbus_spi/Libraries/Modbus/Inc" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Libraries-2f-Modbus-2f-Src

clean-Libraries-2f-Modbus-2f-Src:
	-$(RM) ./Libraries/Modbus/Src/modbus.cyclo ./Libraries/Modbus/Src/modbus.d ./Libraries/Modbus/Src/modbus.o ./Libraries/Modbus/Src/modbus.su

.PHONY: clean-Libraries-2f-Modbus-2f-Src

