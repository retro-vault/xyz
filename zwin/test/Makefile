CXX			=	g++
CXXFLAGS	=	-L $(BUILD_DIR) -lgtest_main -lgtest -lgpx

all: $(BUILD_DIR)/test-gpx

$(BUILD_DIR)/test-gpx: test-gpx.cpp
	$(CXX) test-gpx.cpp -o $(BUILD_DIR)/test-gpx $(CXXFLAGS)
