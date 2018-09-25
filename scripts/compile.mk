CXX:=$(XS_CROSS_COMPILE)g++
CC:=$(XS_CROSS_COMPILE)gcc

LIBS=-lpthread
INCLUDES=-I$(XS_INCLUDE)
CXXFLAGS=-O2 -Wall -g
LCXXFLAGS=-L$(XS_LIB)
DCXXFLAGS=-M

OBJS=$(patsubst %.cpp, %.o, $(SRCS))
DEPS=$(patsubst %.cpp, %.d, $(SRCS))
DEPS_TARGET=$(XS_PREFIX)/deplists

bin:$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(CXXFLAGS) $(LIBS) $(LCXXFLAGS)

lib:$(OBJS)
	$(AR) rc $(TARGET) $(OBJS)

%.o:%.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

%.o:%.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.d:%.cpp
	@$(CXX) $(INCLUDES) $(DCXXFLAGS) $< >> $(DEPS_TARGET)

%.d:%.c
	@$(CC) $(INCLUDES) $(DCXXFLAGS) $< >> $(DEPS_TARGET)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS_TARGET)
endif

.PHONY : clean
clean:
	rm -rf $(DEPS_TARGET) $(TARGET) $(OBJS)
