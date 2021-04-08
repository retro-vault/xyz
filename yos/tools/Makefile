.PHONY: all clean

all: $(BIN_DIR)/tools/makezxbin $(BIN_DIR)/tools/relocator

$(BIN_DIR)/tools/makezxbin: $(BUILD_DIR)/makezxbin.o
	$(CC) $< $(CFLAGS) -o $@

$(BUILD_DIR)/makezxbin.o: makezxbin.c
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN_DIR)/tools/relocator: $(BUILD_DIR)/relocator.o
	$(CXX) $< $(CXXFLAGS) -o $@

$(BUILD_DIR)/relocator.o: relocator.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@