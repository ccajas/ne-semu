# Flags and pkg-config options
CFLAGS = -Wall -s -O1 -std=c99 -MMD -MP `pkg-config --cflags glfw3 freetype2`
LDFLAGS = `pkg-config --libs glfw3 freetype2`

# Source and object files
NFD      =nfd/src/
nfd_src  =$(NFD)nfd_zenity.c $(NFD)nfd_common.c
glfw_src =$(wildcard src/glfw/*.c)
gfx_src  =$(wildcard src/gl/*.c)

src = $(wildcard src/*.c) $(gfx_src) $(glfw_src) $(nfd_src)
src_min = src/main.c src/gl/glad.c
src_core =  src/cpu6502.c src/ppu2c02.c src/mapper.c src/rom.c src/palette.c 
lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)
obj_min = main.o

# Output
target =bin/ne-semu
all: glfw

.PHONY: clean

# main build	
glfw: $(obj)
	cc $(CFLAGS) $(src) -o $(target) -lm -ldl $(LDFLAGS)

glfw_min: $(obj)
	cc $(CFLAGS) $(src_core) $(src_min) -o $(target) -lm -ldl $(LDFLAGS)

clean:
	rm -f $(obj) $(target)