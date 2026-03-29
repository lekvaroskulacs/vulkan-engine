CFLAGS = -std=c++20
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
THIRDPARTY_INCLUDE_PATH = ./thirdparty/
DEFINES = -DVULKAN_HPP_NO_CONSTRUCTORS -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC
build: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -o build/engine ./src/*.cpp $(LDFLAGS) -I$(THIRDPARTY_INCLUDE_PATH) $(DEFINES)

dbg: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -ggdb -o build/engine_debug ./src/*.cpp $(LDFLAGS) -I$(THIRDPARTY_INCLUDE_PATH) $(DEFINES)


.PHONY: run clean

run: build/engine
	build/engine

clean:
	rm -f build/

format:
	./clang_format.sh

shader:
	./compile_shaders.sh

.PHONY: build dbg