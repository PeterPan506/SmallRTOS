################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/lwIP_Demo/lwip-1.4.0/src/netif/ppp/subdir.mk
-include src/lwIP_Demo/lwip-1.4.0/src/netif/subdir.mk
-include src/lwIP_Demo/lwip-1.4.0/src/core/snmp/subdir.mk
-include src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/subdir.mk
-include src/lwIP_Demo/lwip-1.4.0/src/core/subdir.mk
-include src/lwIP_Demo/lwip-1.4.0/src/api/subdir.mk
-include src/lwIP_Demo/lwIP_port/netif/subdir.mk
-include src/lwIP_Demo/lwIP_port/subdir.mk
-include src/lwIP_Demo/lwIP_Apps/subdir.mk
-include src/lwIP_Demo/subdir.mk
-include src/SmallRTOS/OS/subdir.mk
-include src/SmallRTOS/CPU/GCC/ARM_CA9/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
SmallRTOSDemo.elf.size \


# All Target
all: SmallRTOSDemo.elf secondary-outputs

# Tool invocations
SmallRTOSDemo.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM gcc linker'
	arm-xilinx-eabi-gcc -Wl,-T -Wl,../src/lscript.ld -L../../SmallRTOS_bsp/ps7_cortexa9_0/lib -o "SmallRTOSDemo.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

SmallRTOSDemo.elf.size: SmallRTOSDemo.elf
	@echo 'Invoking: ARM Print Size'
	arm-xilinx-eabi-size SmallRTOSDemo.elf  |tee "SmallRTOSDemo.elf.size"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(S_UPPER_DEPS)$(C_DEPS)$(ELFSIZE) SmallRTOSDemo.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
