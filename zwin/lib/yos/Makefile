# Source files.
SRCS	=	$(wildcard *.c)
OBJS	=	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET	=	libyos

# Rules.
.PHONY: all
all: $(BUILD_DIR)/$(TARGET).a

$(BUILD_DIR)/$(TARGET).a: $(OBJS)
	$(AR) -o $(BUILD_DIR)/$(TARGET).a $(OBJS) $(ARFLAGS) 

$(BUILD_DIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
