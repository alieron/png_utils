### 1. Decoding the PNG format
Using [2x2_uncompressed.png](2x2_uncompressed.png) as the example unless specified

See https://www.w3.org/TR/png for detailed specifications regarding the PNG encoding format

#### PNG Signature
```
89 50 4e 47 0d 0a 1a 0a
__  P  N  G __ __ __ __
```

#### Header Chunk(First chunk)

```
Chunk Size  | Chunk Type  | Img Width   | Img Height  | Depth | Color | Comp | Filter | Interlace | CRC
00 00 00 0d | 49 48 44 52 | 00 00 00 02 | 00 00 00 02 | 08    | 06    | 00   | 00     | 00        | 72 b6 0d 24
__ __ __ 13 |  I  H  D  R | __ __ __  2 | __ __ __  2 | 8 bit | RGBA  |  0   |  0     |  0        | __ __ __ __
                          | <-                            13-bytes                             -> |

```

#### Data Chunks

```
Chunk Size  | Chunk Type  | zlib Compressed Data        | CRC
00 00 00 1d | 49 44 41 54 | 08 1d 01 12 ... 1c e2 05 3c | fa 32 7c d3
__ __ __ 29 |  I  D  A  T | __ __ __ __ ... __ __ __ __ | __ __ __ __
                          | <-     n-bytes, n=29     -> |

```

#### End Chunk(Last chunk)

The end chunk is always the same as the example below

```
Chunk Size  | Chunk Type  | CRC
00 00 00 00 | 49 45 4e 44 | ae 42 60 82
__ __ __  0 |  I  E  N  D | __ __ __ __

```

### 2. zlib deflate/huffman
#### zlib Header
See https://www.rfc-editor.org/rfc/rfc1950 for zlib compressed data format

```
Bytes ->  | 08 (CMF)          | 1d (FLG)                   | -> Data
Bits  ->  | 0 0 0 0 | 1 0 0 0 | 0 0    | 0     | 1 1 1 0 1 |
          | CINFO   | CM      | FLEVEL | FDICT | FCHECK    |
```
LZ77 window size = `2^(CINFO + 8)` or `1 << (CINFO + 8)`

`FCHECK` is set such that `CMF*256 + FLG % 31 = 0`

There's only one zlib header, but there can me several blocks of compressed data

#### Data Block
See https://www.rfc-editor.org/rfc/rfc1951 for DEFLATE compressed data format

```
          | uncompressed               | | fixed huffman              | | dynamic huffman            |
Bytes ->  | 01                         | | 63                         | | 85                         |
Bits  ->  | 0 0 0 0 0 | 0 0   | 1      | | 0 1 1 0 0 | 0 1   | 1      | | 1 0 0 0 0 | 1 0   | 1      |
          | (Skipped) | BTYPE | BFINAL | | Use later | BTYPE | BFINAL | | Use later | BTYPE | BFINAL |
```
BTYPE specifies how the data are compressed, as follows:

- 00 - no compression
- 01 - compressed with fixed Huffman codes
- 10 - compressed with dynamic Huffman codes
- 11 - reserved (error)

#### Uncompressed Data Block(BTYPE = 00)
Using [2x2_uncompressed.png](2x2_uncompressed.png) for this example

```
LEN   | NLEN  | Raw Data                                              | ADLER32
12 00 | ed ff | 00 00 00 00 ff 00 00 00 00 00 32 3c 39 ff 4f 8e ba ff | 1c e2 05 3c
              | <-                LEN-bytes, LEN=18                -> |
```
NLEN is the one's complement of LEN, ie. `NLEN = LEN ^ 0xffff = 65535 - LEN`

Using LSB that means `LEN = 00 12 = 18` and `NLEN = ff ed = 65517`

#### PNG Length and Distance Codes
These codes are used by both fixed and dynamic huffman encoding

Taken from https://www.rfc-editor.org/rfc/rfc1951

##### Length Table
| Code | Extra Bits | Lengths | Code | Extra Bits | Lengths | Code | Extra Bits | Lengths |
|------|------------|---------|------|------------|---------|------|------------|---------|
| 257  | 0          | 3       | 267  | 1          | 15,16   | 277  | 4          | 67-82   |
| 258  | 0          | 4       | 268  | 1          | 17,18   | 278  | 4          | 83-98   |
| 259  | 0          | 5       | 269  | 2          | 19-22   | 279  | 4          | 99-114  |
| 260  | 0          | 6       | 270  | 2          | 23-26   | 280  | 4          | 115-130 |
| 261  | 0          | 7       | 271  | 2          | 27-30   | 281  | 5          | 131-162 |
| 262  | 0          | 8       | 272  | 2          | 31-34   | 282  | 5          | 163-194 |
| 263  | 0          | 9       | 273  | 3          | 35-42   | 283  | 5          | 195-226 |
| 264  | 0          | 10      | 274  | 3          | 43-50   | 284  | 5          | 227-257 |
| 265  | 1          | 11,12   | 275  | 3          | 51-58   | 285  | 0          | 258     |
| 266  | 1          | 13,14   | 276  | 3          | 59-66   |      |            |         |


##### Distamce Table
| Code | Extra Bits | Distance | Code | Extra Bits | Distance | Code | Extra Bits | Distance    |
|------|------------|----------|------|------------|----------|------|------------|-------------|
| 0    | 0          | 1        | 10   | 4          | 33-48    | 20   | 9          | 1025-1536   |
| 1    | 0          | 2        | 11   | 4          | 49-64    | 21   | 9          | 1537-2048   |
| 2    | 0          | 3        | 12   | 5          | 65-96    | 22   | 10         | 2049-3072   |
| 3    | 0          | 4        | 13   | 5          | 97-128   | 23   | 10         | 3073-4096   |
| 4    | 1          | 5,6      | 14   | 6          | 129-192  | 24   | 11         | 4097-6144   |
| 5    | 1          | 7,8      | 15   | 6          | 193-256  | 25   | 11         | 6145-8192   |
| 6    | 2          | 9-12     | 16   | 7          | 257-384  | 26   | 12         | 8193-12288  |
| 7    | 2          | 13-16    | 17   | 7          | 385-512  | 27   | 12         | 12289-16384 |
| 8    | 3          | 17-24    | 18   | 8          | 513-768  | 28   | 13         | 16385-24576 |
| 9    | 3          | 25-32    | 19   | 8          | 769-1024 | 29   | 13         | 24577-32768 |

#### Application of <distance,length> codes
Using the compressed block from [2x2_blue.png](2x2_blue.png) for this example

```
63 10 15 7e 0c 44 0c 10 0a 00
          | Reversing of byte order and turned to bits
          V
|0000|0000000|0|10100|0010000|00001100|01000|1000000|110001111|11000010|10100010|00001100|011
|Skip|0->100 |0|5    |4->260 |30->0   |2    |1->257 |1e3->e3  |43->13  |45->15  |30->0   |
             |dist=7 |len=6  |        |dist=3|len=3 |
          |
          V
00 15 13 e3 <3,3>(15 13 e3) 00 <7,6>(15 13 e3 15 13 e3)
            ^ back 3 copy 3    ^ back 7 copy 6
```

#### Fixed Huffman codes(BTYPE = 01)
Using [2x2_fixed_huffman.png](2x2_fixed_huffman.png) for this example

```
Compressed Block        -> 63 60 60 60 f8 cf 00 02 46 36 96 ff fd fb 76 fd 07 00
                                                     V
Reversed Byte Order     -> 00 07 fd 76 fb fd ff 96 36 46 02 00 cf f8 60 60 60 63
                                                     V
Bits(read from R to L)  -> 00000000 00000111 11111101 ... 01100000 01100000 01100|011|
                                                                                ^   ^
                                                                                |   3 bits(BTYPE/BFINAL)
                                    Start reading bits from here, towards the left
```
The first byte in the compressed block(header byte) is moved to the right, and subsequent bytes are added to the left

The byte boundaries should be ignored and the bits can be read from right to left

```
Lit Value    Bits        Codes                  Codes(Hex)
---------    ----        -----                  -----
  0 - 143     8          00110000-10111111      30-bf
144 - 255     9          110010000-111111111    190-1ff
256 - 279     7          0000000-0010111        0-17
280 - 287     8          11000000-11000111      c0-c7
```

The table above represents the default literal alphabet used to deflate fixed huffman codes

```
|000000|00 00000|111 111111|01 0111011|0 1111101|1 1111110|1 11111111 |10010110 |00110110 |01000110| 00000|010 0000|0000 1100|1111 11111|000 01100|000 01100|000 01100|000 01100|011
| Skip | 0->100 | 1ff->ff  | 1ba->ba  | be->8e  | 7f->4f  | 1ff->ff   | 69->39  | 6c->3c  | 62->32 | 0    | 2->258 | 30->0   | 1ff->ff  | 30->0   | 30->0   | 30->0   | 30->0   |
                                                                                                   |dist=1|len=4   |
                                                                                                          V
                                               <1,4> means that len=4 codes from dist=1 are copied, thus code 30 is copied 4 times, as its the last code in the window

The code, read from the left: 00 00 00 00 ff 00 <1,4>(expands to: 00 00 00 00) 3c 39 ff 4f 8e ba ff 100(end of file)
```
Working from the rightmost bit, where `00001100` is read in reverse as `00110000=0x30=0(according to the fixed table)` * Table below

Some codes are length codes, which in addition to extra trailing bits, may indicate the length of data to copy, the subsequent 5 bits indicate the backward distance to the data to copy

#### Dynamic Huffman codes(BTYPE=10)
Using [6x6_dynamic_huffman.png](6x6_dynamic_huffman.png) for this example

Dynamic huffman codes are used in larger images, where it is more efficient then using fixed length codes, since it requires the huffman table to be stored with the compressed block

```
3d cc 21 0e 80 40 10 43 d1 4f 0d 57 d8 a4 87 41 73 05 ee 3f 62 15 7a 12 04 62 92 9a 9f 57 d3 e3 ba 1f 00 70 57 9d 76 97 dc 35 7b ba 17 8a 80 a9 a2 bd f8 5e 03 8a 52 45 b9
          | Reversing of byte order and turned to bits
          V
1011100101000101010100101000101000000011010111101111100010111101101000101010100110000000100010100001011110111010011110110011010111011100100101110111011010011101010101110111000000000000000111111011101011100011110100110101011110011111100110101001001001100010000001000001001001111010000101010110001000111111111011100000010101110011010000011000011110100100110110000101011100001101010011111 | 10100010100001100010000010000001000000000001110001000011100110000111101 
```

The first chunk starting from the right most bit, containes information to build the huffman tree that encodes the literal and distance trees that encode the image data

```
|101|000|101|000|011|000|100|000|100|000|010|000|000|000|011|100|010|000|1110      |01100     |00111     |101
|1  |14 |2  |13 |3  |12 |4  |11 |5  |10 |6  |9  |7  |8  |0  |18 |17 |16 |HCLEN=14+4|HDIST=12+1|HLIT=7+257|
| <-                 HCLEN number of 3-bit segments                  -> | 18       | 13       | 264     
              ^ These segments are read from left to right, unlike the rest of the compressed data
```

        5 bits -> HLIT    - Number of literal/length codes - 257
        5 bits -> HDIST   - Number of distance codes - 1
        4 bits -> HCLEN   - Number of length codes used to encode the 1st huffman tree that was used to encode the literal and distance trees for the actual data - 4
HCLEN x 3 bits -> nbits   - Number of bits for the 1st huffman tree, in the order:
                            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15

```
Deshuffling the length code nbits returns: { 3, 5, 5, 3, 4, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4 }
    V
Counting the number of each size: { 0, 0, 2, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, maximum size 16bits

firstcode   :{ 0, 0, 0, 4, 12, 30, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 0, }
firstsymbol :{ 0, 0, 0, 2, 4, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 0, }
maxcode     :{ 0, 0, 32768, 49152, 61440, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, }
next_code   :{ 0, 0, 2, 6, 15, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, }

zsize       :{ 2, 2, 3, 3, 4, 4, 4, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, ...
zvalue      :{ 6, 17, 0, 3, 4, 5, 18, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, ...
```

### 3. Raw data to colorarray
Using the raw data from [2x2_uncompressed.png](2x2_uncompressed.png) for this example

```
Scanline 1                            | Scanline 2
Filter Type | Color Channels n-bytes  | Filter Type | Color Channels n-bytes
00          | 00 00 00 ff 00 00 00 00 | 00          | 32 3c 39 ff 4f 8e ba ff
            |  R  G  B  A  R  G  B  A |             |  R  G  B  A  R  G  B  A    <- Color Array
```

The number of scanlines = height of the image, each scanline has 1 byte allocated to the filter type for it

The number of bytes allocated to colors in each scan line, `n = width * channels`, thus from the example we have the following image:

|||||
|------|---|---|------|
|(0,0) |   |   | (1,0)|
|      |<img src="https://placehold.co/16/000000/000000.png" width="16"/>|<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/3/3d/Fully_transparent_placeholder.svg/240px-Fully_transparent_placeholder.svg.png" width="16"/>||
|      |<img src="https://placehold.co/16/323c39/323c39.png" width="16"/>|<img src="https://placehold.co/16/4f8eba/4f8eba.png" width="16"/>||
|(0,1) |   |   | (1,1)|

which matches the input image

#### Applying filters

| Type | Name    | Filter Function              | Reconstruction Function        |
| ---- | ------- | ---------------------------- | ------------------------------ |
| 0    | None    | Filt(x) = Orig(x)            | Recon(x) = Filt(x)             |
| 1    | Sub     | Filt(x) = Orig(x) - Orig(a)  | Recon(x) = Filt(x) + Recon(a)  |
| 2    | Up      | Filt(x) = Orig(x) - Orig(b)  | Recon(x) = Filt(x) + Recon(b)  |
| 3    | Average | Filt(x) = Orig(x) - floor((Orig(a) + Orig(b)) / 2)            | Recon(x) = Filt(x) + floor((Recon(a) + Recon(b)) / 2)             |
| 4    | Paeth   | Filt(x) = Orig(x) - PaethPredictor(Orig(a), Orig(b), Orig(c)) | Recon(x) = Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c)) |

`a, b, c` refers to the previous bytes from the same channel that have been reconstructed, their positions defined as below relative to `x`, the current byte, as the data is iterated through scanline by scanline
|||
|-|-|
|c|b|
|a|__x__|

for the first scanline, `b, c = 0` and the first byte in each scanline `a, c = 0` since they are at the border of the array of scanlines, note 

From [2x2_fixed_huffman.png](2x2_fixed_huffman.png)
```
                | Filter Type | Color RGBA              |
Scanline 1      | 00          | 00 00 00 ff 00 00 00 00 |
Reconstructed 1 |             | 00 00 00 ff 00 00 00 00 |
Scanline 2      | 01          | 32 3c 39 ff 1d 52 81 00 |
Reconstructed 2 |             | 32 3c 39 ff 4f 8e ba ff |
                                            ^
                                    1d(x) + 32(a) = 4f
```

From [2x2_blue_filterdemo.png](2x2_blue_filterdemo.png) after deflate
```
                | Filter Type | Colors RGB        |
Scanline 1      | 01          | 15 13 e3 00 00 00 |
Reconstructed 1 |             | 15 13 e3 15 13 e3 |
                                ^
                        15(x) + 00(a) = 15
Scanline 2      | 02          | 00 00 00 00 00 00 |
Reconstructed 2 |             | 15 13 e3 15 13 e3 |
                                      ^
                              00(x) + e3(b) = e3
```


### Helpful resources
PNG Specification - https://www.w3.org/TR/png/
RFC 1950, ZLIB Compressed Data Specification - https://www.rfc-editor.org/rfc/rfc1950
RFC 1951, Deflate Compressed Data Specification - https://www.rfc-editor.org/rfc/rfc1951
Basic PNG reader(Article) - https://handmade.network/forums/articles/t/2822-tutorial_implementing_a_basic_png_reader_the_handmade_way

```cpp
data->channels = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0)/*if color > 4 return 1 else 0*/;
		//^ Understanding the statement above
		//* color	channels	1st ternary	2nd ternary
		//*	0		1 gray		1			0
		//*	2		3 RGB		3			0
		//*	3		1 index		1			0
		//*	4		2 grayA		1			1
		//*	6		4 RGBA		3			1
```

### Interesting Bits

```js
(x & 1) gives 0 if x is even and 1 if x is odd

(6 & 2) is bitwise and on 110 and 010 which returns 10
(5 & 2) is bitwise and on 101 and 010 which returns 00

(y & 1) and (~y & 1) give alternating 0 and 1 as y is incremented
```

### Drawing RGB Colors in Terminal using ANSI Codes

Escape code for linux: `\x1b`

Some terminals support 24-bit color, ie. you can set foreground and background colors using RGB.

| ESC Code Sequence       | Description                  |
| :---------------------- | :--------------------------- |
| `ESC[38;2;{r};{g};{b}m` | Set foreground color as RGB. |
| `ESC[48;2;{r};{g};{b}m` | Set background color as RGB. |

```cpp
#include <stdio.h>

const int width = 16;
const int height = 16;

unsigned char img_RGBA[] = {95,74,43,255,76,61,38,255,95,74,43,255,116,90,54,255,95,74,43,255,76,61,38,255,95,74,43,255,95,74,43,255,116,90,54,255,76,61,38,255,95,74,43,255,95,74,43,255,76,61,38,255,95,74,43,255,95,74,43,255,95,74,43,255,76,61,38,255,184,148,95,255,184,148,95,255,184,148,95,255,184,148,95,255,175,143,85,255,184,148,95,255,175,143,85,255,194,157,98,255,175,143,85,255,175,143,85,255,184,148,95,255,184,148,95,255,184,148,95,255,184,148,95,255,76,61,38,255,95,74,43,255,184,148,95,255,126,98,55,255,126,98,55,255,150,116,65,255,150,116,65,255,159,132,77,255,150,116,65,255,159,132,77,255,159,132,77,255,159,132,77,255,159,132,77,255,150,116,65,255,150,116,65,255,194,157,98,255,95,74,43,255,95,74,43,255,184,148,95,255,126,98,55,255,194,157,98,255,194,157,98,255,184,148,95,255,184,148,95,255,184,148,95,255,184,148,95,255,175,143,85,255,184,148,95,255,184,148,95,255,184,148,95,255,150,116,65,255,184,148,95,255,95,74,43,255,95,74,43,255,175,143,85,255,150,116,65,255,184,148,95,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,184,148,95,255,159,132,77,255,175,143,85,255,95,74,43,255,76,61,38,255,184,148,95,255,159,132,77,255,184,148,95,255,175,143,85,255,150,116,65,255,150,116,65,255,159,132,77,255,159,132,77,255,150,116,65,255,150,116,65,255,175,143,85,255,184,148,95,255,150,116,65,255,175,143,85,255,76,61,38,255,95,74,43,255,175,143,85,255,150,116,65,255,175,143,85,255,175,143,85,255,126,98,55,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,150,116,65,255,175,143,85,255,175,143,85,255,150,116,65,255,184,148,95,255,95,74,43,255,95,74,43,255,175,143,85,255,159,132,77,255,184,148,95,255,175,143,85,255,150,116,65,255,175,143,85,255,159,132,77,255,159,132,77,255,175,143,85,255,159,132,77,255,175,143,85,255,184,148,95,255,159,132,77,255,175,143,85,255,95,74,43,255,95,74,43,255,175,143,85,255,159,132,77,255,184,148,95,255,175,143,85,255,150,116,65,255,175,143,85,255,159,132,77,255,150,116,65,255,175,143,85,255,150,116,65,255,175,143,85,255,184,148,95,255,150,116,65,255,175,143,85,255,95,74,43,255,76,61,38,255,175,143,85,255,159,132,77,255,184,148,95,255,175,143,85,255,150,116,65,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,159,132,77,255,175,143,85,255,175,143,85,255,150,116,65,255,175,143,85,255,76,61,38,255,95,74,43,255,175,143,85,255,150,116,65,255,184,148,95,255,175,143,85,255,150,116,65,255,150,116,65,255,126,98,55,255,150,116,65,255,159,132,77,255,159,132,77,255,175,143,85,255,184,148,95,255,159,132,77,255,175,143,85,255,95,74,43,255,95,74,43,255,175,143,85,255,159,132,77,255,184,148,95,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,175,143,85,255,184,148,95,255,150,116,65,255,175,143,85,255,95,74,43,255,76,61,38,255,184,148,95,255,159,132,77,255,184,148,95,255,184,148,95,255,175,143,85,255,184,148,95,255,184,148,95,255,184,148,95,255,184,148,95,255,184,148,95,255,194,157,98,255,184,148,95,255,150,116,65,255,175,143,85,255,76,61,38,255,95,74,43,255,184,148,95,255,159,132,77,255,150,116,65,255,159,132,77,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,150,116,65,255,184,148,95,255,95,74,43,255,95,74,43,255,184,148,95,255,184,148,95,255,184,148,95,255,175,143,85,255,175,143,85,255,184,148,95,255,175,143,85,255,175,143,85,255,194,157,98,255,175,143,85,255,175,143,85,255,184,148,95,255,184,148,95,255,184,148,95,255,95,74,43,255,95,74,43,255,76,61,38,255,95,74,43,255,95,74,43,255,95,74,43,255,76,61,38,255,95,74,43,255,95,74,43,255,116,90,54,255,76,61,38,255,95,74,43,255,95,74,43,255,76,61,38,255,116,90,54,255,95,74,43,255,95,74,43,255};


void drawRGBAarray(int width, int height, int nchannels, unsigned char *img_RGBA){
    for (int h=0; h<height; h++){
        for (int w=0; w<width; w++){
			int index = nchannels*(h*width+w);

            if (nchannels == 4 && !(img_RGBA[index+3])) {
                printf("\x1b[0m  ");
            } else {
 				printf("\x1b[48;2;%i;%i;%im  ", (int)img_RGBA[index], (int)img_RGBA[index+1], (int)img_RGBA[index+2]);
			}
        }
        printf("\x1b[0m\n");
    }
}


int main() {
    drawRGBAarray(width, height, channels, RGBAarray);
}
```

### Hexdump of a file

```cpp
#include <stdio.h>

void hexdump(void *ptr, int buflen) {
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;
	for (i=0; i<buflen; i+=16) {
		printf("%06x: ", i);
		for (j=0; j<16; j++) 
		if (i+j < buflen)
			printf("%02x ", buf[i+j]);
		else
			printf("   ");
		printf(" ");
		for (j=0; j<16; j++) 
		if (i+j < buflen)
			printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
		printf("\n");
	}
}
void hexdumpfile(const char *filename) {
	FILE *fp = fopen(filename, "rb");
    
    fseek(fp, 0, SEEK_END);
    int len = (int) ftell(fp);
    rewind(fp);

	uint8_t buff[len];
	
    fread(buff, 1, len, fp);
    fclose(fp);

	hexdump(buff, sizeof(buff));

	return;
}

int main() {
  hexdumpfile("<path to file>");
}
```
