CC = clang++
BIN = bin
OBJ = obj
SRC = src
TARGET = $(BIN)/trash
SRCS = $(wildcard $(SRC)/*.cpp)
OBJS = $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

CC_COMMON = -std=c++20 -march=native -Wall -Wextra -Wpedantic
CC_DEBUG = -g -DDEBUG -fsanitize=undefined
CC_RELEASE = -O3 -Werror
LD_COMMON = 
LD_DEBUG = -fsanitize=undefined
LD_RELEASE = 

CCFLAGS = $(CC_COMMON) $(CC_DEBUG)
LDFLAGS = $(LD_COMMON) $(LD_DEBUG)
release: CCFLAGS = $(CC_COMMON) $(CC_RELEASE)
release: LDFLAGS = $(LD_COMMON) $(LD_RELEASE)

debug: $(TARGET)
-include $(DEPS)
release: clean $(TARGET)

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) -MMD $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(DEPS) $(OBJS) *.asm *.o *.out
