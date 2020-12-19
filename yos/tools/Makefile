.PHONY: all clean

all: $(BIN_DIR)/tools/makezxbin

$(BIN_DIR)/tools/makezxbin: $(BUILD_DIR)/makezxbin.o
	$(CC) $< $(CFLAGS) -o $@

$(BUILD_DIR)/makezxbin.o: makezxbin.c
	$(CC) -c $< $(CFLAGS) -o $@
