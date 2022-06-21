CXX = g++
CXXFLAGS = -std=c++11 -Wall -ggdb -Wno-unused
LINKS = -lpthread

BUILD_DIR = ./build
BIN_DIR = ./bin

# 创建出build和bin文件夹
$(shell test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR))
$(shell test -d $(BIN_DIR) || mkdir -p $(BIN_DIR))

DIR_SRC_DIR = ./src
TEST_SRC_DIR = ./test
BENCHMARK_TEST_SRC_DIR = ./benchmark
TOOL_SRC_DIR = ./tools

SRC1 = $(wildcard $(DIR_SRC_DIR)/*.cpp $(DIR_SRC_DIR)/net/*.cpp)
SRC = $(filter-out $(DIR_SRC_DIR)/main.cpp, $(SRC1))
OBJ = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SRC)))
INC = $(patsubst %, %, $(shell find src -name '[a-zA-Z0-9]*'.h))

# 主程序
KVMAIN_TARGET = $(BIN_DIR)/kvmain

# 测试程序
TEST_SRCS = $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_TARGETS = $(patsubst %.cpp, $(BIN_DIR)/%, $(notdir $(TEST_SRCS)))

# benchmark测试程序
BENCHMARK_SRCS = $(wildcard $(BENCHMARK_TEST_SRC_DIR)/*.cpp)
BENCHMARK_TARGETS = $(patsubst %.cpp, $(BIN_DIR)/%, $(notdir $(BENCHMARK_SRCS)))

# 两个工具程序
TOOLS_SRCS = $(wildcard $(TOOL_SRC_DIR)/*.cpp)
TOOLS_TARGETS = $(patsubst %.cpp, $(BIN_DIR)/%, $(notdir $(TOOLS_SRCS)))

D_TCMALLOC_FLAG =
TCMALLOC_LINK = 
TCMALLOC_FOUND = $(shell find /usr/lib/ -name '*tcmalloc*')
ifneq ($(strip $(TCMALLOC_FOUND)),)
D_TCMALLOC_FLAG = -O2 -DTCMALLOC_FOUND
TCMALLOC_LINK = -ltcmalloc
endif

all: $(KVMAIN_TARGET) $(TEST_TARGETS) $(BENCHMARK_TARGETS) $(TOOLS_TARGETS)

kvmain: $(KVMAIN_TARGET)

test: $(TEST_TARGETS)

benchmark: $(BENCHMARK_TARGETS)

tools: $(TOOLS_TARGETS)

$(TOOLS_TARGETS) : $(BIN_DIR)/% : $(TOOL_SRC_DIR)/%.cpp $(OBJ) $(INC)
	$(CXX) $(CXXFLAGS) $(OBJ) $< -o $@ $(LINKS) $(D_TCMALLOC_FLAG) $(TCMALLOC_LINK)
	@echo "Done generating tools"

$(BENCHMARK_TARGETS) : $(BIN_DIR)/% : $(BENCHMARK_TEST_SRC_DIR)/%.cpp $(OBJ) $(INC)
	$(CXX) $(CXXFLAGS) $(OBJ) $< -o $@ $(LINKS) $(D_TCMALLOC_FLAG) $(TCMALLOC_LINK)
	@echo "Done generating benchmark test targets"

$(TEST_TARGETS) : $(BIN_DIR)/% : $(TEST_SRC_DIR)/%.cpp $(OBJ) $(INC)
	$(CXX) $(CXXFLAGS) $(OBJ) $< -o $@ $(LINKS)
	@echo "Done generating test targets"

$(KVMAIN_TARGET) : $(OBJ) $(INC)
	$(CXX) $(CXXFLAGS) -c $(DIR_SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o $(D_TCMALLOC_FLAG)
	$(CXX) $(CXXFLAGS) $(OBJ) $(BUILD_DIR)/main.o -o $@ $(LINKS) $(TCMALLOC_LINK)
	@echo "Done generating kvmain"

$(BUILD_DIR)/%.o : $(DIR_SRC_DIR)/net/%.cpp $(INC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : $(DIR_SRC_DIR)/%.cpp $(INC)
	$(CXX) $(CXXFLAGS) -c $< -o $@


.PHONY: clean display check_tcmalloc

display:
	@echo $(TEST_TARGETS)
	@echo $(BENCHMARK_TARGETS)
	@echo $(OBJ)
	@echo $(SRC)

clean:
	-rm -r build/*
	-rm $(TEST_TARGETS)
	-rm $(KVMAIN_TARGET)
	-rm $(BENCHMARK_TARGETS)
	-rm $(TOOLS_TARGETS)

check_tcmalloc:
	@echo $(D_TCMALLOC_FLAG)
	