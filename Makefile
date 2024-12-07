CC = gcc
CFLAGS = -Wall -Wextra
TARGET = main
SRC_DIR = src
SRC = $(SRC_DIR)/main.c

# Detect platform
ifeq ($(OS),Windows_NT) # Windows
    RM = del /Q
    EXE_EXT = .exe
else # macOS or Linux
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin) # macOS
        RM = rm -f
        EXE_EXT =
    else ifeq ($(UNAME),Linux) # Linux
        RM = rm -f
        EXE_EXT =
    else
        $(error Unsupported platform: $(UNAME))
    endif
endif

# Add platform-specific extensions
TARGET := $(TARGET)$(EXE_EXT)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -lm

clean:
	$(RM) $(TARGET)

run: $(TARGET)
	./$(TARGET)
