# -Wall -Wextra -Wshadow -Wno-write-strings
all:
	clang++ -g -O0 -std=c++17 -I .                                          \
		-Weverything 							\
		-Wno-c++98-compat-pedantic -Wno-c99-extensions			\
		-Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct\
		-Wno-nested-anon-types 						\
		-Wno-sign-conversion						\
		-Wno-padded -Wno-zero-length-array				\
		-Wno-vla-extension -Wno-vla					\
		-fno-exceptions -fno-rtti					\
		$(shell sdl2-config --libs --cflags)				\
		$(shell pkg-config --libs --cflags freetype2) -lSDL2_ttf        \
		main.cpp -o program

valgr:
	valgrind --leak-check=full --track-origins=yes --log-file=valg.log      \
	./program &> /dev/null 						        \
	&& cat valg.log
