CXX := g++

SRC_DIR := src
INC_DIR := include

# Output binary name
TARGET := DriveMgr_CLI

# Flags
CXXFLAGS := -std=c++17 -I$(INC_DIR)
LDFLAGS := -lssl -lcrypto

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(SRC_DIR)/%.o,$(SRCS))

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile each .cpp into .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(SRC_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# finished
run: all
	./$(TARGET)
