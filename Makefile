# Define the path to your header files
HEADERS = include/Camera.hpp include/Mesh.hpp include/Model.hpp include/Shader.hpp include/Scene.hpp

# Default target placed at the top so 'make' runs it automatically
all: build/main

# Include the headers as prerequisites
build/main: src/main.cpp utils/glad.c $(HEADERS)
	g++ src/main.cpp utils/glad.c -o build/main -Iinclude -Iinclude/third-party -lglfw -ldl -lGL -lassimp -O2

clean:
	rm -f build/main