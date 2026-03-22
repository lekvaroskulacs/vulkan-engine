CFLAGS = -std=c++17
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
THIRDPARTY_INCLUDE_PATH = ./thirdparty/

build: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -o build/engine ./src/*.cpp $(LDFLAGS) -I$(THIRDPARTY_INCLUDE_PATH)

dbg: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -ggdb -o build/engine ./src/*.cpp $(LDFLAGS) -I$(THIRDPARTY_INCLUDE_PATH)


.PHONY: run clean

run: build/engine
	build/engine

clean:
	rm -f build/

format:
	./clang_format.sh

shader:
	./compile_shaders.sh