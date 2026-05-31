CFLAGS = -std=c++20
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lshaderc_shared
THIRDPARTY_INCLUDE_PATH = ./thirdparty/
DEFINES = -DVULKAN_HPP_NO_CONSTRUCTORS -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC
INCLUDES = -I./thirdparty -I./thirdparty/imgui -I./thirdparty/imgui/backends -I./src/include/ -I./texture/
IMGUI = ./thirdparty/imgui/*.cpp ./thirdparty/imgui/*.h ./thirdparty/imgui/backends/imgui_impl_vulkan.cpp ./thirdparty/imgui/backends/imgui_impl_vulkan.h ./thirdparty/imgui/backends/imgui_impl_glfw.cpp ./thirdparty/imgui/backends/imgui_impl_glfw.h
CPP = ./src/source/*.cpp ./src/*.cpp ./src/pipeline/*.cpp

build: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -o build/engine $(CPP) $(IMGUI) $(LDFLAGS) $(INCLUDES) $(DEFINES)

dbg: src/main.cpp
	./compile_shaders.sh
	./src/clang_format.sh
	mkdir -p build
	g++ $(CFLAGS) -ggdb -o build/engine $(CPP) ./src/include/*.h ./src/texture/*.h $(IMGUI) $(LDFLAGS) -I./src/include/ $(INCLUDES) $(DEFINES)


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