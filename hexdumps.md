## 2x2 Example images
Used in the examples in notes.md

<img src="images/2x2_fixed_huffman.png" width="200"/>

### Hexdump of 2x2_uncompressed.png (BTYPE = 00) ✅
Generated with `pngcrush -force -m 1 -l 0 2x2_fixed_huffman.png 2x2_uncompressed.png`
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 02 00 00 00 02 08 06 00 00 00 72 b6 0d  .............r..
000020: 24 00 00 00 1d 49 44 41 54 08 1d 01 12 00 ed ff  $....IDAT.......
000030: 00 00 00 00 ff 00 00 00 00 00 32 3c 39 ff 4f 8e  ..........2<9.O.
000040: ba ff 1c e2 05 3c fa 32 7c d3 00 00 00 00 49 45  .....<.2|.....IE
000050: 4e 44 ae 42 60 82                                ND.B`.
```

### Hexdump of 2x2_fixed_huffman.png (BTYPE = 01)
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 02 00 00 00 02 08 06 00 00 00 72 b6 0d  .............r..
000020: 24 00 00 00 18 49 44 41 54 08 d7 63 60 60 60 f8  $....IDAT..c```.
000030: cf 00 02 46 36 96 ff fd fb 76 fd 07 00 1c e2 05  ...F6....v......
000040: 3c f1 b6 c2 19 00 00 00 00 49 45 4e 44 ae 42 60  <........IEND.B`
000050: 82                                               .
```

<img src="images/2x2_blue_filterdemo.png" width="200"/>

### Hexdump of 2x2_blue_filterdemo.png (BTYPE = 01)
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 02 00 00 00 02 08 02 00 00 00 fd d4 9a  ................
000020: 73 00 00 00 16 49 44 41 54 18 95 63 14 15 7e cc  s....IDAT..c..~.
000030: c0 c0 c0 c4 c0 c0 c0 c0 c0 00 00 0b e0 01 0f b6  ................
000040: 7e 48 df 00 00 00 00 49 45 4e 44 ae 42 60 82     ~H.....IEND.B`.
```

## 3x3 Test Images 
Used to test defiltering

<img src="images/3x3_uncompressed.png" width="200"/>

### Hexdump of 3x3_uncompressed.png (BTYPE = 00) ✅
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 03 00 00 00 03 08 06 00 00 00 56 28 b5  .............V(.
000020: bf 00 00 00 32 49 44 41 54 08 1d 01 27 00 d8 ff  ....2IDAT...'...
000030: 00 32 3c 39 ff 4f 8e ba ff 00 00 00 00 00 df 71  .2<9.O.........q
000040: 26 ff ff ff ff ff 9b ad b7 ff 00 22 20 34 ff ac  &.........." 4..
000050: 32 32 ff 22 20 34 ff 60 72 12 a5 cb c4 9d da 00  22." 4.`r.......
000060: 00 00 00 49 45 4e 44 ae 42 60 82                 ...IEND.B`.
```
```
FIL|  R  G  B  A |  R  G  B  A |  R  G  B  A
00 | 32 3c 39 ff | 4f 8e ba ff | 00 00 00 00
00 | df 71 26 ff | ff ff ff ff | 9b ad b7 ff
00 | 22 20 34 ff | ac 32 32 ff | 22 20 34 ff
```

### Hexdump of 3x3_filter032.png (BTYPE = 00) ✅
! likely invalid checksum !
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 03 00 00 00 03 08 06 00 00 00 56 28 b5  .............V(.
000020: bf 00 00 00 32 49 44 41 54 08 1d 01 27 00 d8 ff  ....2IDAT...'...
000030: 00 32 3c 39 ff 4f 8e ba ff 00 00 00 00 03 c6 53  .2<9.O.........S
000040: 0a 80 68 80 8f 00 1c 2e 38 80 02 43 af 0e 00 ad  ..h.....8..C....
000050: 33 33 00 87 73 7d 00 60 72 12 a5 cb c4 9d da 00  33..s}.`r.......
000060: 00 00 00 49 45 4e 44 ae 42 60 82                 ...IEND.B`.
```
```
00 | 32 3c 39 ff | 4f 8e ba ff | 00 00 00 00
03 | c6 53 0a 80 | 68 80 8f 00 | 1c 2e 38 80    hex((x-((a+b)>>1) & 0xff) & 0xff)
02 | 43 af 0e 00 | ad 33 33 00 | 87 73 7d 00    hex((x-b) & 0xff)
```

### Hexdump of 3x3_filter314.png (BTYPE = 00) ✅
! likely invalid checksum !
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 03 00 00 00 03 08 06 00 00 00 56 28 b5  .............V(.
000020: bf 00 00 00 32 49 44 41 54 08 1d 01 27 00 d8 ff  ....2IDAT...'...
000030: 03 32 3c 39 ff 36 70 9e 80 d9 b9 a3 81 01 df 71  .2<9.6p........q
000040: 26 ff 20 8e d9 00 9c ae b8 00 04 43 af 0e 00 8a  &. ........C....
000050: c1 33 00 87 ee 02 00 60 72 12 a5 cb c4 9d da 00  .3.....`r.......
000060: 00 00 00 49 45 4e 44 ae 42 60 82                 ...IEND.B`.
```
```
03 | 32 3c 39 ff | 36 70 9e 80 | d9 b9 a3 81    hex((x-((a+0)>>1) & 0xff) & 0xff)
01 | df 71 26 ff | 20 8e d9 00 | 9c ae b8 00    hex((x-a) & 0xff)
04 | 43 af 0e 00 | 8a c1 33 00 | 87 ee 02 00    hex((x-(paeth_pdr(a,b,c) & 0xff)) & 0xff)
```

## 6x6 Dynamic Huffman Test Image
Used to test dynamic huffman encoding

<img src="images/6x6_dynamic_huffman.png" width="200"/>

### Hexdump of 6x6_dynamic_huffman.png (BTYPE = 10)
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 06 00 00 00 06 08 02 00 00 00 6f ae 78  .............o.x
000020: 1f 00 00 00 41 49 44 41 54 18 95 3d cc 21 0e 80  ....AIDAT..=.!..
000030: 40 10 43 d1 4f 0d 57 d8 a4 87 41 73 05 ee 3f 62  @.C.O.W...As..?b
000040: 15 7a 12 04 62 92 9a 9f 57 d3 e3 ba 1f 00 70 57  .z..b...W.....pW
000050: 9d 76 97 dc 35 7b ba 17 8a 80 a9 a2 bd f8 5e 03  .v..5{........^.
000060: 8a 52 45 b9 fb 01 8a 52 23 e7 20 cf ca 05 00 00  .RE....R#. .....
000070: 00 00 49 45 4e 44 ae 42 60 82                    ..IEND.B`.
```


## Oak log image
<img src="images/oak_log_top.png" width="200"/>

### Hexdump of oak_log_uncompressed.png (BTYPE = 00) ✅
Generated with `pngcrush -force -m 1 -l 0 oak_log_top.png oak_log_uncompressed.png`
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 10 00 00 00 10 08 06 00 00 00 1f f3 ff  ................
000020: 61 00 00 04 1b 49 44 41 54 38 11 01 10 04 ef fb  a....IDAT8......
000030: 00 5f 4a 2b ff 4c 3d 26 ff 5f 4a 2b ff 74 5a 36  ._J+.L=&._J+.tZ6
000040: ff 5f 4a 2b ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b  ._J+.L=&._J+._J+
000050: ff 74 5a 36 ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b  .tZ6.L=&._J+._J+
000060: ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b ff 5f 4a 2b  .L=&._J+._J+._J+
000070: ff 00 4c 3d 26 ff b8 94 5f ff b8 94 5f ff b8 94  ..L=&..._..._...
000080: 5f ff b8 94 5f ff af 8f 55 ff b8 94 5f ff af 8f  _..._...U..._...
000090: 55 ff c2 9d 62 ff af 8f 55 ff af 8f 55 ff b8 94  U...b...U...U...
0000a0: 5f ff b8 94 5f ff b8 94 5f ff b8 94 5f ff 4c 3d  _..._..._..._.L=
0000b0: 26 ff 00 5f 4a 2b ff b8 94 5f ff 7e 62 37 ff 7e  &.._J+..._.~b7.~
0000c0: 62 37 ff 96 74 41 ff 96 74 41 ff 9f 84 4d ff 96  b7..tA..tA...M..
0000d0: 74 41 ff 9f 84 4d ff 9f 84 4d ff 9f 84 4d ff 9f  tA...M...M...M..
0000e0: 84 4d ff 96 74 41 ff 96 74 41 ff c2 9d 62 ff 5f  .M..tA..tA...b._
0000f0: 4a 2b ff 00 5f 4a 2b ff b8 94 5f ff 7e 62 37 ff  J+.._J+..._.~b7.
000100: c2 9d 62 ff c2 9d 62 ff b8 94 5f ff b8 94 5f ff  ..b...b..._..._.
000110: b8 94 5f ff b8 94 5f ff af 8f 55 ff b8 94 5f ff  .._..._...U..._.
000120: b8 94 5f ff b8 94 5f ff 96 74 41 ff b8 94 5f ff  .._..._..tA..._.
000130: 5f 4a 2b ff 00 5f 4a 2b ff af 8f 55 ff 96 74 41  _J+.._J+...U..tA
000140: ff b8 94 5f ff af 8f 55 ff af 8f 55 ff af 8f 55  ..._...U...U...U
000150: ff af 8f 55 ff af 8f 55 ff af 8f 55 ff af 8f 55  ...U...U...U...U
000160: ff af 8f 55 ff b8 94 5f ff 9f 84 4d ff af 8f 55  ...U..._...M...U
000170: ff 5f 4a 2b ff 00 4c 3d 26 ff b8 94 5f ff 9f 84  ._J+..L=&..._...
000180: 4d ff b8 94 5f ff af 8f 55 ff 96 74 41 ff 96 74  M..._...U..tA..t
000190: 41 ff 9f 84 4d ff 9f 84 4d ff 96 74 41 ff 96 74  A...M...M..tA..t
0001a0: 41 ff af 8f 55 ff b8 94 5f ff 96 74 41 ff af 8f  A...U..._..tA...
0001b0: 55 ff 4c 3d 26 ff 00 5f 4a 2b ff af 8f 55 ff 96  U.L=&.._J+...U..
0001c0: 74 41 ff af 8f 55 ff af 8f 55 ff 7e 62 37 ff af  tA...U...U.~b7..
0001d0: 8f 55 ff af 8f 55 ff af 8f 55 ff af 8f 55 ff 96  .U...U...U...U..
0001e0: 74 41 ff af 8f 55 ff af 8f 55 ff 96 74 41 ff b8  tA...U...U..tA..
0001f0: 94 5f ff 5f 4a 2b ff 00 5f 4a 2b ff af 8f 55 ff  ._._J+.._J+...U.
000200: 9f 84 4d ff b8 94 5f ff af 8f 55 ff 96 74 41 ff  ..M..._...U..tA.
000210: af 8f 55 ff 9f 84 4d ff 9f 84 4d ff af 8f 55 ff  ..U...M...M...U.
000220: 9f 84 4d ff af 8f 55 ff b8 94 5f ff 9f 84 4d ff  ..M...U..._...M.
000230: af 8f 55 ff 5f 4a 2b ff 00 5f 4a 2b ff af 8f 55  ..U._J+.._J+...U
000240: ff 9f 84 4d ff b8 94 5f ff af 8f 55 ff 96 74 41  ...M..._...U..tA
000250: ff af 8f 55 ff 9f 84 4d ff 96 74 41 ff af 8f 55  ...U...M..tA...U
000260: ff 96 74 41 ff af 8f 55 ff b8 94 5f ff 96 74 41  ..tA...U..._..tA
000270: ff af 8f 55 ff 5f 4a 2b ff 00 4c 3d 26 ff af 8f  ...U._J+..L=&...
000280: 55 ff 9f 84 4d ff b8 94 5f ff af 8f 55 ff 96 74  U...M..._...U..t
000290: 41 ff af 8f 55 ff af 8f 55 ff af 8f 55 ff af 8f  A...U...U...U...
0002a0: 55 ff 9f 84 4d ff af 8f 55 ff af 8f 55 ff 96 74  U...M...U...U..t
0002b0: 41 ff af 8f 55 ff 4c 3d 26 ff 00 5f 4a 2b ff af  A...U.L=&.._J+..
0002c0: 8f 55 ff 96 74 41 ff b8 94 5f ff af 8f 55 ff 96  .U..tA..._...U..
0002d0: 74 41 ff 96 74 41 ff 7e 62 37 ff 96 74 41 ff 9f  tA..tA.~b7..tA..
0002e0: 84 4d ff 9f 84 4d ff af 8f 55 ff b8 94 5f ff 9f  .M...M...U..._..
0002f0: 84 4d ff af 8f 55 ff 5f 4a 2b ff 00 5f 4a 2b ff  .M...U._J+.._J+.
000300: af 8f 55 ff 9f 84 4d ff b8 94 5f ff af 8f 55 ff  ..U...M..._...U.
000310: af 8f 55 ff af 8f 55 ff af 8f 55 ff af 8f 55 ff  ..U...U...U...U.
000320: af 8f 55 ff af 8f 55 ff af 8f 55 ff b8 94 5f ff  ..U...U...U..._.
000330: 96 74 41 ff af 8f 55 ff 5f 4a 2b ff 00 4c 3d 26  .tA...U._J+..L=&
000340: ff b8 94 5f ff 9f 84 4d ff b8 94 5f ff b8 94 5f  ..._...M..._..._
000350: ff af 8f 55 ff b8 94 5f ff b8 94 5f ff b8 94 5f  ...U..._..._..._
000360: ff b8 94 5f ff b8 94 5f ff c2 9d 62 ff b8 94 5f  ..._..._...b..._
000370: ff 96 74 41 ff af 8f 55 ff 4c 3d 26 ff 00 5f 4a  ..tA...U.L=&.._J
000380: 2b ff b8 94 5f ff 9f 84 4d ff 96 74 41 ff 9f 84  +..._...M..tA...
000390: 4d ff 96 74 41 ff 96 74 41 ff 96 74 41 ff 96 74  M..tA..tA..tA..t
0003a0: 41 ff 96 74 41 ff 96 74 41 ff 96 74 41 ff 96 74  A..tA..tA..tA..t
0003b0: 41 ff 96 74 41 ff b8 94 5f ff 5f 4a 2b ff 00 5f  A..tA..._._J+.._
0003c0: 4a 2b ff b8 94 5f ff b8 94 5f ff b8 94 5f ff af  J+..._..._..._..
0003d0: 8f 55 ff af 8f 55 ff b8 94 5f ff af 8f 55 ff af  .U...U..._...U..
0003e0: 8f 55 ff c2 9d 62 ff af 8f 55 ff af 8f 55 ff b8  .U...b...U...U..
0003f0: 94 5f ff b8 94 5f ff b8 94 5f ff 5f 4a 2b ff 00  ._..._..._._J+..
000400: 5f 4a 2b ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b ff  _J+.L=&._J+._J+.
000410: 5f 4a 2b ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b ff  _J+.L=&._J+._J+.
000420: 74 5a 36 ff 4c 3d 26 ff 5f 4a 2b ff 5f 4a 2b ff  tZ6.L=&._J+._J+.
000430: 4c 3d 26 ff 74 5a 36 ff 5f 4a 2b ff 5f 4a 2b ff  L=&.tZ6._J+._J+.
000440: d4 ae 59 38 10 03 e6 4f 00 00 00 00 49 45 4e 44  ..Y8...O....IEND
000450: ae 42 60 82                                      .B`.
```

### Hexdump of oak_log_top.png (BTYPE = 10)
Generated with `pngcrush -force -m 1 -l 9 oak_log_top.png oak_log_uncompressed.png`
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 10 00 00 00 10 08 06 00 00 00 1f f3 ff  ................
000020: 61 00 00 00 d8 49 44 41 54 38 cb 8d 53 21 0e 02  a....IDAT8..S!..
000030: 31 10 ec 6b 30 78 50 08 2c 39 24 06 73 c9 bd e1  1..k0xP.,9$.s...
000040: 34 0a 8f b9 e0 08 4f c0 a2 f9 58 c9 34 99 cd 74  4.....O...X.4..t
000050: db 52 c4 64 b7 db 76 ba b3 dd 0d e3 61 1d 87 dd  .R.d..v.....a...
000060: 2a c2 ce e7 4d d4 35 63 ba 56 1f 08 08 bc 97 31  *...M.5c.V.....1
000070: c3 eb 76 32 fb 79 4c c9 32 a6 c0 dd 00 16 2c 2e  ..v2.yL.2.....,.
000080: d3 36 e1 3e ef 13 9e d7 a3 59 05 f7 41 9c 32 50  .6.>.....Y..A.2P
000090: 02 04 81 56 46 04 08 60 8d 00 07 18 64 ba 2d e0  ...VF..`....d.-.
0000a0: 0c 32 81 9f d5 00 41 12 a8 0c 4d 9b 04 f4 ad 06  .2....A...M.....
0000b0: bc c4 57 20 c7 bf ac fb 55 09 3e 03 c6 98 2e 6d  ..W ....U.>....m
0000c0: 21 a1 47 40 bf 26 c1 6a d0 22 50 f0 55 dd 2f 6a  !.G@.&.j."P.U./j
0000d0: e0 8b c8 be 50 29 7f 49 e8 7d 63 21 41 bf b1 d6  ....P).I.}c!A...
0000e0: 38 04 9b ac 90 40 02 6d e1 1e b2 6f f4 6d ab c3  8....@.m...o.m..
0000f0: f4 6b a0 8c c0 8f 68 6f 9c 39 f6 c0 17 d4 ae 59  .k....ho.9.....Y
000100: 38 37 c9 53 1f 00 00 00 00 49 45 4e 44 ae 42 60  87.S.....IEND.B`
000110: 82                                               .
```

### Hexdump of oak_leaves.png
```
000000: 89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52  .PNG........IHDR
000010: 00 00 00 10 00 00 00 10 08 06 00 00 00 1f f3 ff  ................
000020: 61 00 00 00 c7 49 44 41 54 38 cb 85 93 51 0e c3  a....IDAT8...Q..
000030: 20 0c 43 39 ec 3e 7a 81 a9 57 e9 8e 52 69 77 63   .C9.>z..W..Riwc
000040: 33 9a d1 c3 4d 35 a4 08 08 81 d8 4e 68 db 63 eb  3...M5.....Nh.c.
000050: b2 f6 1d e7 fb 1c f3 f1 3a ba 4c eb fd b9 77 99  ........:.L...w.
000060: 62 ec 73 ac fc 8d 4e 0d 3f a8 43 9f 69 e6 5e 97  b.s...N.?.C.i.^.
000070: b5 76 c2 c6 4c 44 a0 3d 51 31 bb ad 31 43 d2 71  .v..LD.=Q1..1C.q
000080: 16 fb 38 2e 97 3d 3b e0 02 f1 47 8f fa b4 84 9b  ..8..=;...G.....
000090: 02 19 11 11 a4 3e 0b 77 1d 12 7a 56 a2 d4 a7 e2  .....>.w..zV....
0000a0: 4e 81 aa aa 18 e1 d8 33 93 83 ed 23 82 2c e5 a4  N......3...#.,..
0000b0: 5d 09 75 d7 0b 7c 94 89 97 ce 33 95 7f bd 30 4b  ].u..|....3...0K
0000c0: 99 8a 67 00 cb b9 a8 cf 0e 24 82 ac 73 25 a6 7d  ..g......$..s%.}
0000d0: 63 66 86 3b 34 a4 c5 0a 4c 0a 09 97 08 d8 03 e5  cf.;4...L.......
0000e0: 47 a2 aa cb 07 29 2a 92 b4 74 f7 03 ce 5d ce 0f  G....)*..t...]..
0000f0: b9 97 71 38 00 00 00 00 49 45 4e 44 ae 42 60 82  ..q8....IEND.B`.
```



<style>img {
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}</style>