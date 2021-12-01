OBJ = obj
SRC = src
SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)
BIN = bin
TARGET = $(BIN)/trash

debug: $(TARGET)
-include $(DEPS)
release: clean $(TARGET)

CC = clang
CC_COMMON = -MMD -std=c11 -pedantic -Wall -Wextra -Werror -Wshadow -Wunused
CC_DEBUG = -g -DDEBUG -fsanitize=undefined
CC_RELEASE = -Ofast

CCFLAGS = $(CC_COMMON) $(CC_DEBUG)
LDFLAGS = -fsanitize=undefined
release: CCFLAGS = $(CC_COMMON) $(CC_RELEASE)
release: LDFLAGS =


$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(DEPS) $(OBJS)

