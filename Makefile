CXX      := g++ #~/clang-built/bin/clang++ #/home/phil/clang/llvm-project/build/bin/clang++
CXXCOMPILEFLAGS := $(shell cat compile_flags.txt)
CXXLINKFLAGS :=  $(CXXCOMPILEFLAGS)
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/obj/%.o)
TARGET   := build/decoder

all: makedir $(TARGET)

makedir:
	@mkdir -p build/obj

$(TARGET): $(OBJ)
	$(CXX) $(CXXLINKFLAGS) $^ -o $@

build/obj/%.o: src/%.cpp
	$(CXX) $(CXXCOMPILEFLAGS) -c $< -o $@

clean:
	rm -rf build
