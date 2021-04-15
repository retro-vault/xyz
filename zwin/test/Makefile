CXX			=	g++
CXXFLAGS	=	-L $(BUILD_DIR) -lgtest_main -lgtest -lgpx

all: $(BUILD_DIR)/test-gpx $(BUILD_DIR)/test-yos

$(BUILD_DIR)/test-gpx: test-gpx.cpp
	$(CXX) test-gpx.cpp -o $(BUILD_DIR)/test-gpx $(CXXFLAGS)

$(BUILD_DIR)/test-yos: test-yos.cpp
	$(CXX) test-yos.cpp -o $(BUILD_DIR)/test-yos $(CXXFLAGS)
