# Special tools.
LD		= gcc
LDFLAGS	= -L$(BUILD_DIR) -lSDL2 -lc -lgpx -lm

# Source files.
SRCS	=	$(wildcard *.c)
OBJS	=	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET	=	zwin

# Rules.
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(LD) -o $(BUILD_DIR)/$(TARGET) $(OBJS) $(LDFLAGS) 

$(BUILD_DIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) -I $(INC_DIR)
