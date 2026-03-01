CFLAGS = -std=c++17
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

build: main.cpp
	mkdir -p build
	g++ $(CFLAGS) -o build/engine *.cpp $(LDFLAGS)

.PHONY: run clean

run: engine
	./engine

clean:
	rm -f engine

format:
	./clang_format.sh