CXX := g++

SRC_DIR := src
INC_DIR := include
INC_DIR_UI := include/ui
INC_DIR_UTILS := include/utils

BUILD_DIR := build

TARGET := DMgr_CLI

CXXFLAGS := -std=c++17 -I$(INC_DIR) -I$(INC_DIR_UI) -I$(INC_DIR_UTILS)
LDFLAGS := -flto -s -O2 -lssl -lcrypto

SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(SRC_DIR)/ui/*.cpp) \
        $(wildcard $(SRC_DIR)/utils/*.cpp)

OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
        $(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Create build directory if missing
$(BUILD_DIR):
        mkdir -p $(BUILD_DIR)

# Compile rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
        $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
        rm -rf $(BUILD_DIR) $(TARGET)

run: all
        ./$(TARGET)
