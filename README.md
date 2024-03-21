# png_utils

Simple png loader intended for loading simple images for minecraft texturepacks ![](images/oak_log_top.png)

## Supported features:

### Chunk Types:

- [x] IHDR
- [x] IDAT
- [x] IEND
- [ ] PLTE
- ~~[ ] tRNS~~
- ~~[ ] sRGB~~

### Img Properties:

- Depth = 8 only (Might explore 16bit depth later)
- Color = RGBA, RGB, grayscale and grayscale+A
- Comp = 0 only, as in specification
- Filter = 0 only, as in specification
- Interlace = 0 only (No reason to support ADAM7 interlace)

### zlib Compression:

- ~~[ ] FDICT~~

- [x] No Compression, BTYPE = 00
- [x] Fixed Huffman Codes, BTYPE = 01
- [x] Dynamic Huffman Codes, BTYPE = 10

### Filter Methods:

- [x] NONE
- [x] SUB
- [x] UP
- [x] AVG
- [x] PAETH (TODO: Optimize?)

### Test cases:

Passes for all cases defined in [hexdumps.md](hexdumps.md)


