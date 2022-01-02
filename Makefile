OBJ = obj
SRC = src
SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)
BIN = bin

# TODO: compile with -Werror
CC_COMMON = -MMD -std=c11 -march=native -Wall -Wextra -Wshadow -Wunused -Wpedantic
CC_DEBUG = -g -DDEBUG
CC_RELEASE = -O2
LD_COMMON = 
LD_DEBUG = 
LD_RELEASE = 

# assuming mingw
ifeq ($(OS), Windows_NT)
	TARGET = $(BIN)/trash.exe
	# TODO: do windows specific stuff
else
	TARGET = $(BIN)/trash
	# unfortunately mingw does not seem to support asan
	CC_DEBUG += -fsanitize=undefined
	LD_DEBUG += -fsanitize=undefined
endif

CCFLAGS = $(CC_COMMON) $(CC_DEBUG)
LDFLAGS = $(LD_COMMON) $(LD_DEBUG)
release: CCFLAGS = $(CC_COMMON) $(CC_RELEASE)
release: LDFLAGS = $(LD_COMMON) $(LD_RELEASE)

debug: $(TARGET)
-include $(DEPS)
release: clean $(TARGET)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
ifeq ($(OS), Windows_NT)
	del $(subst /,\,$(TARGET) $(DEPS) $(OBJS))
else
	rm -f $(TARGET) $(DEPS) $(OBJS)
endif

