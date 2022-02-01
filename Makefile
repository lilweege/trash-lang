BIN = bin
OBJ = obj
SRC = src
TARGET = $(BIN)/trash
SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# TODO: compile with -Werror
CC_COMMON = -std=c11 -march=native -Wall -Wextra -Wshadow -Wunused -Wpedantic -DLINUX
CC_DEBUG = -g -DDEBUG -fsanitize=undefined
CC_RELEASE = -O2
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

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -MMD $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(DEPS) $(OBJS) *.asm *.o *.out
