# Compiler
CXX := g++

# Directories
SRC_DIR := src
INC_DIR := include

# Output binary
TARGET := DMgr_CLI

# Flags
CXXFLAGS := -std=c++17 -I$(INC_DIR)
LDFLAGS := -flto -s -O2 -lssl -lcrypto

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:.cpp=.o)

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
