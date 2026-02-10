SOURCES			=	$(wildcard src/*.cpp)
HEADERS			=	$(wildcard include/xlink/*.hpp)
TARGET			=	xlink
INCLUDE			=	-Iinclude

TEST_MAIN		=	tests/test_main.cpp
TEST_FILTER		=	$(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET		=	xlink_tests

.PHONY: tools
tools: $(SOURCES) $(HEADERS)
	$(CXX) -o $(BUILD_DIR)/$(TARGET) $(SOURCES) $(CXXFLAGS) $(INCLUDE)

.PHONY: test
test: $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(BUILD_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(CXXFLAGS) $(INCLUDE) -Itests
	cd $(dir $(TEST_MAIN)).. && $(BUILD_DIR)/$(TEST_TARGET)
	$(BUILD_DIR)/$(TEST_TARGET)
