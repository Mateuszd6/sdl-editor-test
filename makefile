all:
	clang++ -g -O0 -std=c++17 -I .                                          \
		-Wall -Wextra -Wshadow -Wno-write-strings			\
		$(shell sdl2-config --libs --cflags)				\
		$(shell pkg-config --libs --cflags freetype2) -lSDL2_ttf        \
		main.cpp -o program
