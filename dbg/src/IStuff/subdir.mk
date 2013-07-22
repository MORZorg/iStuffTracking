# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
						../src/IStuff/object.cpp \
						../src/IStuff/database.cpp \
						../src/IStuff/manager.cpp \
						../src/IStuff/recognizer.cpp \
						../src/IStuff/tracker.cpp \

OBJS += \
				./src/IStuff/object.o \
				./src/IStuff/database.o \
				./src/IStuff/manager.o \
				./src/IStuff/recognizer.o \
				./src/IStuff/tracker.o \

CPP_DEPS += \
						./src/IStuff/object.d \
						./src/IStuff/database.d \
						./src/IStuff/manager.d \
						./src/IStuff/recognizer.d \
						./src/IStuff/tracker.d \


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	$(CXX) `pkg-config --cflags opencv` -std=c++11 \
		-c -fmessage-length=0 -MMD -MP \
		-MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

