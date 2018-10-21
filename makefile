CXX = clang++
CXX_FLAGS = -std=c++17 -I . -fno-exceptions -fno-rtti
CXX_DEBUG_FLAGS = -g -O0 -fno-omit-frame-pointer
CXX_RELEASE_FLAGS = -O3

CXX_WARN_FLAGS = -Weverything 							  \
	 	 -Wno-c++98-compat-pedantic -Wno-c99-extensions			  \
	 	 -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct \
		 -Wno-nested-anon-types 					  \
		 -Wno-sign-conversion						  \
		 -Wno-padded -Wno-zero-length-array				  \
		 -Wno-vla-extension -Wno-vla

LINK_FLAGS = $(shell sdl2-config --libs --cflags)			          \
	     $(shell pkg-config --libs --cflags freetype2)

.PHONY: all debug release valgr

all: debug

debug:
	$(CXX) $(CXX_FLAGS) $(CXX_DEBUG_FLAGS) $(CXX_WARN_FLAGS) $(LINK_FLAGS)	  \
	main.cpp -o program

release:
	$(CXX) $(CXX_FLAGS) $(CXX_RELEASE_FLAGS) $(CXX_WARN_FLAGS) $(LINK_FLAGS)  \
	main.cpp -o program

valgr:
	valgrind --leak-check=full --track-origins=yes --log-file=valg.log        \
	./program &> /dev/null 						          \
	&& cat valg.log
