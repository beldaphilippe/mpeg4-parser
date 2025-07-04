CXX      := clang++ #~/clang/clang+llvm-18.1.7-x86_64-linux-gnu-ubuntu-18.04/bin/clang++ #g++ #~/clang-built/bin/clang++ #/home/phil/clang/llvm-project/build/bin/clang++
CXXCOMPILEFLAGS := $(shell cat compile_flags.txt) -stdlib=libstdc++ -L/usr/lib/gcc/x86_64-linux-gnu/13 -I/usr/include/c++/13
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

run:
	@build/decoder

clean:
	rm -rf build
