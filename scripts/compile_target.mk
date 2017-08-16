
CXX:=$(XS_CROSS_COMPILE)g++
CC:=$(XS_CROSS_COMPILE)gcc

LIBS=-lwiringPi -lc -lpthread
INCLUDES=-I$(XS_INCLUDE)
CXXFLAGS=-O2 -Wall -g
LCXXFLAGS=-L$(XS_LIB)
DCXXFLAGS=-M

OBJS=$(patsubst %.cpp, %.o, $(SRCS))
DEPS=$(patsubst %.cpp, %.d, $(SRCS))
DEPS_TARGET=$(XS_PREFIX)/deplists

-include $(DEPS_TARGET)

$(TARGET):$(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LIBS) $(LCXXFLAGS)
	@rm -rf $(DEPS_TARGET) $(DEPS)

%.o:%.cpp
	$(CXX) $(INCLUDES) $@-c $< -o

%.o:%.c
	$(CC) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

%.d:%.cpp
	@$(CXX) $(INCLUDES) $(DCXXFLAGS) $< >> $(DEPS_TARGET)

%.d:%.c
	@$(CC) $(INCLUDES) $(DCXXFLAGS) $< >> $(DEPS_TARGET)

$(DEPS_TARGET):$(DEPS)

.PHONY : clean

clean:
	rm -rf $(OBJS) $(TARGET) $(DEPS_TARGET)
