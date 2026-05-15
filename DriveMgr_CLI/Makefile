# Compiler
CXX := g++

# Directories
SRC_DIR := src
INC_DIR := include
INC_DIR_UI := include/ui
INC_DIR_UTILS := include/utils


# Output binary
TARGET := DMgr_CLI

# Flags
CXXFLAGS := -std=c++17 -I$(INC_DIR) -I$(INC_DIR_UI) -I$(INC_DIR_UTILS)
LDFLAGS := -flto -s -O2 -lssl -lcrypto

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
SRCS_2 := $(wildcard $(SRC_DIR)/ui/*.cpp)
SRCS_3 := $(wildcard $(SRC_DIR)/utils/*.cpp)

# OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(SRCS) $(SRCS_2) $(SRCS_3)))
OBJS := $(SRCS:.cpp=.o, SRCS_2:.cpp=.o, SRCS_3:.cpp=.o)
# OBJS := $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@rm -f $(OBJS)

# Compile each .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Run
run: all
	./$(TARGET)
