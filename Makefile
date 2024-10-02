# Directories
SRC_DIR := src
INC_DIR := inc
BUILD_DIR := build

# Compiler and flags
CXX := g++
CFLAGS := -Wall -I$(INC_DIR) -std=c++11

# Libraries
LIBS := -lmosquitto -lsqlite3

# Source files and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Executable files
EXEC_SENDER := $(BUILD_DIR)/gnss_sender
EXEC_RECEIVER := $(BUILD_DIR)/gnss_receiver

# Rules
all: $(EXEC_SENDER) $(EXEC_RECEIVER)

$(EXEC_SENDER): $(BUILD_DIR)/gnss_sender.o
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

$(EXEC_RECEIVER): $(BUILD_DIR)/gnss_receiver.o
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
