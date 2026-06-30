
CXX      := g++
CXXFLAGS := -std=c++17 -Iinclude -Wall -Wextra -g

SRCS     := $(shell find src -name '*.cpp')
OBJS     := $(patsubst src/%.cpp,build/%.o,$(SRCS))

TEST_SRC := test/test_library.cpp
TARGET   := test_library

$(TARGET): $(OBJS) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(OBJS) $(TEST_SRC) -o $@

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)

.PHONY: run clean