CXX      := g++
CXXFLAGS := -std=c++17 -Iinclude -Wall -Wextra -g

# 所有库源码（不含 main 和 test）
SRCS     := $(shell find src -name '*.cpp')
OBJS     := $(patsubst src/%.cpp,build/%.o,$(SRCS))

# 主程序
APP      := library
MAIN_SRC := main.cpp

# 测试（每个 test_*.cpp 编成一个独立可执行文件）
TEST_SRCS := $(wildcard test/*.cpp)
TESTS     := $(patsubst test/%.cpp,build/%,$(TEST_SRCS))

# ---- 默认目标：构建主程序 ----
all: $(APP)

$(APP): $(OBJS) build/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# 库对象
build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# main 对象
build/main.o: $(MAIN_SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---- 测试 ----
# 每个测试可执行文件链接全部库对象 + 自己的 test 源文件
build/%: test/%.cpp $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< $(OBJS) -o $@

tests: $(TESTS)

# 构建并依次运行所有测试
test: tests
	@for t in $(TESTS); do \
		echo "==== running $$t ===="; \
		./$$t || exit 1; \
	done

# ---- 运行 / 清理 ----
run: $(APP)
	./$(APP)

clean:
	rm -rf build $(APP)

.PHONY: all tests test run clean
