# ne-semu
A NES emulator written in C

Simple implementation of NES emulation. 

## Status

Passes Kevtris' NESTEST rom and Blargg's instruction test (legal and illegal opcodes)

Fails most PPU tests, as PPU is still not cycle-accurate and updates pixels per frame only

Mappers 0 and 1 working

## Dependencies

GLFW for graphics and input, Native File Dialog for opening files via GUI

To be continued...
