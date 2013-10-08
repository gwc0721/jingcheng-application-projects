################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./source/jcexception.cpp \
./source/jchelptool.cpp \
./source/jcinterface.cpp \
./source/jclogger_appenders.cpp \
./source/jclogger_base.cpp \

#./source/jcstring.cpp \
#./source/jcstring_helptools.cpp 

OBJS += \
./source/jcexception.o \
./source/jchelptool.o \
./source/jcinterface.o \
./source/jclogger_appenders.o \
./source/jclogger_base.o \

#./source/jcstring.o \
#./source/jcstring_helptools.o 

CPP_DEPS += \
./source/jcexception.d \
./source/jchelptool.d \
./source/jcinterface.d \
./source/jclogger_appenders.d \
./source/jclogger_base.d \

#./source/jcstring.d \
#./source/jcstring_helptools.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: source/%.cpp
#($OBJS) : ($CPP_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_UNICODE -D_DEBUG -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


