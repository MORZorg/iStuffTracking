# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main.cpp \
../src/database.cpp \
../src/object.cpp \

OBJS += \
./src/main.o \
./src/database.o \
./src/object.o \

CPP_DEPS += \
./src/main.d \
./src/database.d \
./src/object.d \

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	$(CXX) `pkg-config --cflags opencv` -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
