######################################
#
#
######################################
  
#target you can change test to what you want
#共享库文件名，lib*.so
TARGET  := ./libTransPortEngineSdk.so
  
#compile and lib parameter
#编译参数
CC      := g++
LIBS    := 
LIBS    += 
LDFLAGS := -std=c++11
DEFINES := $(LDFLAGS)
INCLUDE := 

ver = debug  

ifeq ($(ver), debug)  
CFLAGS := -g -Wall -O3 -fPIC $(DEFINES) $(INCLUDE)
else  
CFLAGS := -Wall -O3 -fPIC $(DEFINES) $(INCLUDE)
endif

CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H
SHARE   := -shared -o
  
#i think you should do anything here
#下面的基本上不需要做任何改动了
  
#source file
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(filter-out CNetEventModleIOCP.cpp dllmain.cpp stdafx.cpp, $(wildcard *.c) $(wildcard *.cpp))
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
  
.PHONY : everything objs clean veryclean rebuild
  
everything : $(TARGET)
  
all : $(TARGET)
  
objs : $(OBJS)
  
rebuild: veryclean everything
                
clean :
	rm -fr *.o
    
veryclean : clean
	rm -fr $(TARGET)
  
$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(SHARE) $@ $(OBJS) $(LDFLAGS) $(LIBS)


