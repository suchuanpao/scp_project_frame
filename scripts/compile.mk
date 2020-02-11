CPP:=$(SCP_CROSS_COMPILE)g++
CC:=$(SCP_CROSS_COMPILE)gcc

COMPILE:=$(CC)

SRCS += $(C_SRCS) $(CPP_SRCS)

C FLAGS += -I$(SCP_INCLUDE)
C_FLAGS += -Os -Wall -g
C_LIBS += -lpthread -ldl -lm
C_LDFLAGS += -L$(SCP_LIB)
C_DFLAGS += -M

CPP_FLAGS = -O2 -Wall -g
#CPP_LIBS += -lstdc++
CPP_LIBS += -lstdc++
CPP_LDFLAGS = -L$(SCP_LIB)
CPP_DFLAGS = -M

C_OBJS=$(patsubst %.c, %.lo, $(filter %.c, $(SRCS)))
C_DEPS=$(patsubst %.c, %.d, $(filter %.c, $(SRCS)))

CPP_OBJS=$(patsubst %.cpp, %.oo, $(filter %.cpp, $(SRCS)))
CPP_DEPS=$(patsubst %.cpp, %.d, $(filter %.cpp, $(SRCS)))

DEPS_TARGET=$(SCP_PREFIX)/.deps

bin:$(C_OBJS) $(CPP_OBJS)
	$(CC) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(CPPFLAGS) $(C_LDFLAGS) $(CPP_LDFLAGS) $(C_LIBS) $(CPP_LIBS)

lib:$(OBJS)
	$(AR) rc $(TARGET) $(C_OBJS) $(CPP_OBJS)

%.oo:%.cpp
	$(CPP) $(CPP_FLAGS) -c $< -o $@

%.lo:%.c
	$(CC) $(C_FLAGS) -c $< -o $@

%.d:%.cpp
	@$(CPP) $(CPP_DFLAGS) $(CPP_FLAGS) $< >> $(DEPS_TARGET)

%.d:%.c
	@$(CC) $(C_DFLAGS) $(C_FLAGS) $< >> $(DEPS_TARGET)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS_TARGET)
endif

.PHONY : clean
clean:
	rm -rf $(DEPS_TARGET)
	rm -rf $(TARGET)
	rm -rf $(CPP_OBJS)
	rm -rf $(C_OBJS)
