all:
	clang++ -g -O0 -std=c++17 -I .                                          \
		-Wall -Wextra -Wshadow -Wno-write-strings			\
		-Wzero-as-null-pointer-constant -Wold-style-cast                \
		$(shell sdl2-config --cflags)                 \
		$(shell sdl2-config --libs) -lSDL2_ttf                           \
		main.cpp -o bin/program
