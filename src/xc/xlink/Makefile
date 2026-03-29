BUILD_DIR       = build

SOURCES         = $(wildcard src/*.cpp)
HEADERS         = $(wildcard include/xlink/*.hpp)
TARGET          = xlink
INCLUDE         = -Iinclude

TEST_MAIN       = tests/test_main.cpp
TEST_FILTER     = $(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET     = xlink_tests

.PHONY: all tools test clean
all: tools

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

tools: $(BUILD_DIR) $(SOURCES) $(HEADERS)
	$(CXX) -o $(BUILD_DIR)/$(TARGET) $(SOURCES) $(CXXFLAGS) $(INCLUDE)

test: $(BUILD_DIR) $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(BUILD_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(CXXFLAGS) $(INCLUDE) -Itests
	$(BUILD_DIR)/$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)
