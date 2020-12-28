# Flags and pkg-config options
CFLAGS = -Wall -s -O1 -std=c99 -MMD -MP `pkg-config --cflags glfw3 freetype2`
LDFLAGS = `pkg-config --libs glfw3 freetype2`

# Source and object files
NFD      =nfd/src/
nfd_src  =$(NFD)nfd_zenity.c $(NFD)nfd_common.c
app_src  =$(wildcard src/glfw/*.c)
gfx_src  =$(wildcard src/gfx/*.c)

src = $(wildcard src/*.c) $(glfw_src) $(gfx_src) $(nfd_src)
src_min = main.c
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

clean:
	rm -f $(obj) $(target)