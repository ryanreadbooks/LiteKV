CXX = g++
CXXFLAGS = -std=c++11 -Wall -ggdb -Wno-unused
BUILD_DIR = ./build
OUTPUT_DIR = ./bin

# 指定源文件
SRC_ROOT = $(wildcard src/*.cpp)
HEADER_ROOT = $(wildcard src/*.h)
OBJ_ROOT = $(patsubst %.cpp, %.o, $(SRC_ROOT))
SRC_NET = $(wildcard src/net/*.cpp)
HEADER_NET = $(wildcard src/net/*.h)
OBJ_NET = $(patsubst %.cpp, %.o, $(SRC_NET))

# 指定要生成的中间目标
SRCS = $(SRC_ROOT)
SRCS += $(SRC_NET)
HEADERS = $(HEADER_ROOT)
HEADERS += $(HEADER_NET)
OBJS_RAW = $(patsubst %.cpp, %.o, $(SRCS))
OBJS = $(addprefix ,$(notdir $(OBJS_RAW)))

TEST_PROGS = $(wildcard test/*.cpp)
TOOLS_PROGS = $(wildcard tools/*.cpp)
BENCHMARK_PROGS = $(wildcard benchmark/*.cpp)
MAIN_PROG = src/main.cpp

PROGS = $(MAIN_PROG) $(TEST_PROGS) $(BENCHMARK_PROGS) $(TOOLS_PROGS)

TARGETS = $(addprefix bin/, $(basename $(notdir $(PROGS))))

LINKS = -lpthread

# TODO 如果有tcmalloc则有些文件链接tcmalloc

all: $(TARGETS)

.PHONY: clean display

clean:
	rm -f $(BUILD_DIR)/*

display:
	@echo SRCS = $(SRCS)
	@echo "-------------------------------------------------"
	@echo HEADERS = $(HEADERS)
	@echo "-------------------------------------------------"
	@echo OBJS = $(OBJS)
	@echo "-------------------------------------------------"
	@echo TARGETS = $(TARGETS)



