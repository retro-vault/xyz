# Top configuration (OS dependent)
ifeq ($(OS),Windows_NT)
CC		=	cl
CFLAGS	=	/Fo:$(BUILD_DIR)/ /Fe:$(BIN_DIR)/tools/
RM		=	DEL
else ifeq ($(shell uname), Linux)
$(error TODO!)
else
$(error OS must be Windows or Linux)
endif

.PHONY: all clean

all: $(BIN_DIR)/tools/makezxbin.exe

$(BIN_DIR)/tools/makezxbin.exe: $(BUILD_DIR)/makezxbin.obj

$(BUILD_DIR)/makezxbin.obj: makezxbin.c
	$(CC) $< $(CFLAGS)

clean:
	$(RM) /s $(BIN_DIR)/tools/makezxbin.exe 2>&1
	$(RM) /s $(BUILD_DIR)/makezxbin.obj 2>&1
