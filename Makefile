# Define the target as the actual file path
build/main: src/main.cpp utils/glad.c
	g++ src/main.cpp utils/glad.c -o build/main -Iinclude -Iinclude/third-party -lglfw -ldl -lGL -lassimp

# A "phony" target so you can just type 'make' instead of 'make build/main'
all: build/main

clean:
	rm -f build/main