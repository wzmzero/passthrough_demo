CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS := -lpthread  -lyaml-cpp # -lsqlite3

SRC_DIR := .
BUILD_DIR := build
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TARGET := $(BUILD_DIR)/data_bridge

.PHONY: all clean mkdir_build

all: mkdir_build $(TARGET)

mkdir_build:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)