CXX:=$(XS_CROSS_COMPILE)g++
CC:=$(XS_CROSS_COMPILE)gcc
AR:=$(XS_CROSS_COMPILE)ar
INCLUDES=-I$(XS_INCLUDE)
DCXXFLAGS=-M -I$(XS_INCLUDE)

OBJS=$(patsubst %.cpp, %.o, $(SRCS))
DEPS=$(patsubst %.cpp, %.d, $(SRCS))
DEPS_TARGET=$(XS_PREFIX)/deplists

$(TARGET):$(OBJS)
	$(AR) rc $@ $(OBJS)
	@rm -rf $(DEPS_TARGET) $(DEPS)

%.o:%.cpp
	$(CXX) $(INCLUDES) -c $< -o $@

%.d:%.cpp
	@$(CXX) $(DCXXFLAGS) $(INCLUDES) $< >> $(DEPS_TARGET)

$(DEPS_TARGET):$(DEPS)

-include $(DEPS_TARGET)

.PHONY : clean
clean:
	@rm -rf $(OBJS) $(TARGET) $(DEPS_TARGET)
