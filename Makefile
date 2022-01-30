BIN = bin
TEST = test
OBJ = obj
SRC = src
SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# TODO: compile with -Werror
CC_COMMON = -std=c11 -march=native -Wall -Wextra -Wshadow -Wunused -Wpedantic
CC_DEBUG = -g -DDEBUG
CC_RELEASE = -O2
LD_COMMON = 
LD_DEBUG = 
LD_RELEASE = 

# assuming mingw
ifeq ($(OS), Windows_NT)
	TARGET = $(BIN)/trash.exe
	CC_COMMON += -DWINDOWS
	# TODO: do windows specific stuff
else
	TARGET = $(BIN)/trash
	CC_COMMON += -DLINUX
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
	$(CC) -MMD $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)


.PHONY: clean test

test: $(TEST)/testall.c
	$(CC) $(CCFLAGS) $< $(LDFLAGS) -o $(BIN)/$@
ifeq ($(OS), Windows_NT)
	.\$(BIN)\$@
else
	./$(BIN)/$@
endif

clean:
ifeq ($(OS), Windows_NT)
	del $(subst /,\,$(TARGET) $(DEPS) $(OBJS)) $(BIN)\test.exe
else
	rm -f $(TARGET) $(DEPS) $(OBJS) $(BIN)/test
endif

