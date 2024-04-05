#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct{
	int width;
	int height;
	int depth;					//number of bits that make up one color component
	int channels;				//number of color channels for each pixel
	unsigned char *scanlines;
	unsigned char *colorarray;
} PNG_data;

typedef struct{
	uint16_t *lut;
	uint8_t lut_max_size;

	uint8_t *sizes;
	uint16_t *values;
	uint16_t num_codes;
} huff_alphabet;

typedef struct{
	uint8_t *buffer;
	uint32_t bit_buffer, nbits;
	huff_alphabet lit_alphabet, dist_alphabet;
} zlib_stream;

typedef struct{
	uint32_t length;
	uint32_t type;
} PNG_chunk_header;

#define CHUNK_TYPE(a,b,c,d) (((unsigned) a << 24) + ((unsigned) b << 16) + ((unsigned) c << 8) + (unsigned) d)

// Returns the current value that pbuffer points to and increments pbuffer
static uint8_t getbyte(uint8_t *&pbuffer) { // could be replaced with uint8_t *&pbuffer, so the function call would be getbyte(bp) instead of getbyte(bp)
	// no protection end of buffer is reached
	return *(pbuffer++); // if changed to *&pbufer, use return *(pbuffer++)
}

// Returns 4 consecutive bytes as an int, useful for getting 4 byte chunks
static uint32_t get4bytes(uint8_t *&pbuffer) {
  	return (uint32_t) (getbyte(pbuffer) << 24) + (getbyte(pbuffer) << 16) + (getbyte(pbuffer) << 8) + getbyte(pbuffer);
}

// Jump forward in the image byte buffer by n bytes
static void skipnbytes(uint8_t *&pbuffer, unsigned int n) {
	// no protection end of buffer is reached
	// (*pbuffer) += n;
	pbuffer += n;
}

// Returns n consequtive bytes as an array of unsigned chars, useful for getting chunks of data
static int getnbytes(uint8_t *&pbuffer, uint8_t *bytes_out, int n) {
	// no protection end of buffer is reached
	memcpy(bytes_out, pbuffer, n);
	pbuffer += n;
	return 1;
}

// Verifies that the first 8 bytes of the buffer corresponds to the png file signature
static int validsignature(uint8_t *&pbuffer) {
	static const uint8_t png_sig[8] = { 137,80,78,71,13,10,26,10 };

	int i = 0;
	for (i = 0; i < 8; ++i) 
		if (getbyte(pbuffer) != png_sig[i]) return 0; //TODO: Add error handling
	return 1;
}

// Returns the length and type of the current chunk from the first 8 bytes of the chunk
static void getchunkheader(uint8_t *&pbuffer, PNG_chunk_header *chunk) {
	chunk->length = get4bytes(pbuffer);
	chunk->type = get4bytes(pbuffer);
}

// Refills the bit buffer such that there are at least n bits of data in it
static void refill_bit_buffer(zlib_stream *zstream, int n) {
	do {
		zstream->bit_buffer |= getbyte(zstream->buffer) << zstream->nbits;
		zstream->nbits += 8;
	} while (zstream->nbits < n);
}

// Get last n bits(n <= 32) from the right most byte in the zstream
static uint32_t getzbits(zlib_stream *zstream, int n) {
	if (n == 0) return 0;
	// not protected if the n supplied is more then 32
	uint32_t bits;
	// replenish bits if not enough, up to n bits at least
	if (zstream->nbits < n) refill_bit_buffer(zstream, n);
	bits = zstream->bit_buffer & ((1 << n) - 1); //get the last n bits in the bit buffer
	zstream->nbits -= n;
	zstream->bit_buffer >>= n;
	return bits;
}

// Reverse 16 bits
static uint16_t rev16bits(uint16_t n) {
	n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  	n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  	n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  	n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  	return n;
}

// Get n bits in reverse order
static uint16_t revnbits(uint16_t val, int n) {
	return rev16bits(val) >> (16 - n);
}

// Peak the next n bits in the bit buffer in reverse order
static uint16_t peakzbitsrev(zlib_stream *zstream, int n) {
	// replenish bits if not enough, up to 16 bits at least since its the max size of a code
	if (zstream->nbits < 16) refill_bit_buffer(zstream, 16);
	return revnbits(zstream->bit_buffer & 0xffff, n);
}

// Check that the zstream header is valid
static int valid_zlib_header(uint8_t *&zbuffer) {
	uint8_t cmf = getbyte(zbuffer);
	uint8_t cm = cmf & 15; //get compression method from the 4 last bits in the cmf byte
	if (cm != 8) return 0; //only 8 is supported //TODO: Add error handling
	uint8_t flg = getbyte(zbuffer);
	if (flg & 32) return 0; //dict is not allowed //TODO: Add error handling
	if ((cmf*256+flg) % 31 != 0) return 0; //make sure FCHECK is valid //TODO: Add error handling
	return 1;
}

const int HUFF_LUT_MAX = 9; // to include all the lengths in the default alphabet + keep the LUT small, max = 12, to allow 4 bits for size

// Generate the alphabets from a list of sizes
static int gen_prefix_codes(huff_alphabet *alphabet, const uint8_t *code_sizes, int num_codes) {
	int i, max_size = 0;
	for (i = 0; i < num_codes; i++) {
		// getting the max size of the current code
		if (max_size < code_sizes[i]) max_size = code_sizes[i];
	}
	if (max_size > 16) return 0; // 1-16 max num of bits in a code is 16
	
	int size_count[max_size + 1];
	memset(size_count, 0, sizeof(size_count));
	for (i = 0; i < num_codes; i++) {
		// get the code_count, how many times a code of a certain size occurs
		size_count[code_sizes[i]]++;
	}
	size_count[0] = 0;
	for (i = 1; i <= max_size; i++) {
		if (size_count[i] > (1 << i)) return 0; // count shouldn't exceed the value i number of bits can represent
	}
	
	// set the starting codes for each code size
	uint16_t next_code[max_size + 1];
	int code = 0;
	for (i = 1; i <= max_size; i++) {
		code = (code + size_count[i-1]) << 1;
		if (size_count[i] > 0) next_code[i] = code;
	}

	alphabet->lut_max_size = max_size < HUFF_LUT_MAX ? max_size : HUFF_LUT_MAX; // max size of codes in the lut, capped at 9
	alphabet->lut = NULL;
	alphabet->lut = (uint16_t *) realloc(alphabet->lut, sizeof(uint16_t) * (1 << alphabet->lut_max_size));
	memset(alphabet->lut, 0, sizeof(alphabet->lut));

	alphabet->num_codes = num_codes;
	alphabet->values = NULL;
	alphabet->values = (uint16_t *) realloc(alphabet->values, sizeof(uint16_t) * num_codes);
	memset(alphabet->values, 0, sizeof(alphabet->values));
	alphabet->sizes = NULL;
	alphabet->sizes = (uint8_t *) realloc(alphabet->sizes, num_codes);
	memset(alphabet->sizes, 0, num_codes);

	for (i = 0; i < num_codes; i++) { // i is the current symbol we are coding for
		int size = code_sizes[i];
		if (size) {
			// non LUT stuff
			alphabet->values[i] = next_code[size];
			alphabet->sizes[i] = size;

			// LUT stuff
			if (size <= HUFF_LUT_MAX) {
				int current_prefix = revnbits(next_code[size], size);
				uint16_t value = (uint16_t) ((size << HUFF_LUT_MAX) | i); // pack size into the leftmost 7 bits of symbol, for decoding later
				while (current_prefix < (1 << alphabet->lut_max_size)) {
					alphabet->lut[current_prefix] = value;
					current_prefix += (1 << size);
				}
			}
			next_code[size]++;
		}
	}
	return 1;
}

// Decode the current code against an alphabet if it exceeds the max size to be in the LUT
static int decode_from_alphabet_non_lut(zlib_stream *zstream, huff_alphabet *alphabet) {
	if (zstream->nbits < 16) refill_bit_buffer(zstream, 16); // already done in parent function, but include incase not called from parent
	int i, size;
	// test each symbol in the alphabet to find the value the code refers to
	for (i = 0; i < alphabet->num_codes; i++) {
		size = alphabet->sizes[i];
		if (size) {
			uint16_t code = peakzbitsrev(zstream, size);
			if (alphabet->values[i] == code) {
				zstream->nbits -= size;
				zstream->bit_buffer >>= size;
				return i;
			}
		}
	}
	return -1; // cause and error, since we're not expecting a -ve value
}

// Decode the current code against an alphabet using an LUT
static int decode_from_alphabet(zlib_stream *zstream, huff_alphabet *alphabet) {
	if (zstream->nbits < 16) refill_bit_buffer(zstream, 16);
	int value, size;

	value = alphabet->lut[zstream->bit_buffer & ((1 << alphabet->lut_max_size) - 1)];
	if (value) { // any codes longer then 9 will have symbol = 0
		size = value >> HUFF_LUT_MAX; // size is stored as the left most 7 bits of symbol in the lut
		zstream->nbits -= size;
		zstream->bit_buffer >>= size;
		return value & ((1 << HUFF_LUT_MAX) -1);
	}
	return decode_from_alphabet_non_lut(zstream, alphabet);
}

static const uint8_t fixed_lit_sizes[288] = 
{
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};

static const uint8_t fixed_distance_sizes[32] = 
{
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

// Calculate the literal and distance alphabets for zstream with dynamic huffman codes
static int calculate_huffman_code(zlib_stream *zstream) {
	int hlit  = getzbits(zstream, 5) + 257;
   	int hdist = getzbits(zstream, 5) + 1;
   	int hclen = getzbits(zstream, 4) + 4;
	
	// 1st phase, generate the codelength_alphabet that decodes codelengths for the literal and distances trees
	static const uint8_t length_order[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
	uint8_t codelength_alphabet_sizes[19];
	memset(codelength_alphabet_sizes, 0, 19);
	
	int i;
	for (i = 0; i < hclen; i++) {
		codelength_alphabet_sizes[length_order[i]] = (uint8_t) getzbits(zstream, 3);
	}
	
	huff_alphabet codelength_alphabet;
	if (!gen_prefix_codes(&codelength_alphabet, codelength_alphabet_sizes, 19)) return 0;
	// uint8_t zalphabet_sizes[288+32+138]; // max n of lit + max n of dist + max repitition
	// uint8_t zalphabet_sizes[288+32]; // max n of lit + max n of dist 
	// memset(zalphabet_sizes, 0, 288 + 32);
	uint8_t zalphabet_sizes[hlit+hdist]; // keep static or be dynamic?

	int n_to_parse = hlit + hdist;
	int n = 0;

	// 2nd phase, using the codelength_alphabet to decode codelengths
	while (n < n_to_parse) {
		int code = decode_from_alphabet(zstream, &codelength_alphabet);
		if (code < 0 || code > 18) return 0; // out of range of the codelength alphabet
		if (code < 16) {
			zalphabet_sizes[n] = (uint8_t) code;
			n++;
		} else {
			int copy_len;
			uint8_t copy_val = 0;
			if (code == 16) {
				if (n == 0) return 0; // cannot copy previous codelength if its the first
				copy_len = getzbits(zstream, 2) + 3; // 3-6
				copy_val = zalphabet_sizes[n-1];
			} else if (code == 17) {
				copy_len = getzbits(zstream, 3) + 3; // 3-10
			} else if (code == 18) {
				copy_len = getzbits(zstream, 7) + 11; // 11-138
			} else return 0; // technically redundant since we check if > 18 above
			
			memset(zalphabet_sizes + n, copy_val, copy_len);
			n += copy_len;
		}
	}
	if (n != n_to_parse) return 0; // hlit or hdist was invalid, decoded sizes became larger than expected

	// 3rd phase, generating the literal and distance alphabets for the image data
	if (!gen_prefix_codes(&zstream->lit_alphabet, zalphabet_sizes, hlit)) return 0; // generate literal alphabet
	if (!gen_prefix_codes(&zstream->dist_alphabet, zalphabet_sizes + hlit, hdist)) return 0; // generate literal alphabet
	return 1;
}

static const int z_len_min[31] = 
{ 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,0,0 };

static const uint8_t z_len_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int z_dist_min[32] = 
{ 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0 };

static const int z_dist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

// Inflate the huffman block in the zstream using the literal and distance alphabets
static int inflate_huffman_block(zlib_stream *zstream, uint8_t *scanlines) {
	uint8_t *pout = scanlines;
	for (;;) {
		int code = decode_from_alphabet(zstream, &zstream->lit_alphabet);
		if (code < 0 || code > 285) return 0; // 286 and 287 are invalid code
		if (code == 256) {
			return 1;
		} else if (code < 256) {
			*pout++ = (uint8_t) code;
		} else {
			code -= 257;
			// get length as well as any extra bits as necessary
			int copy_len = z_len_min[code] + getzbits(zstream, z_len_extra[code]);
			
			// get distance as well as any extra bits as necessary
			code = decode_from_alphabet(zstream, &zstream->dist_alphabet);
			if (code < 0 || code >> 30) return 0; // 31 and 32 are invalid codes
			int dist = z_dist_min[code] + getzbits(zstream, z_dist_extra[code]);

			if (pout - scanlines < dist) return 0; // distance goes beyond the start of the data

			uint8_t *copy_val = (uint8_t *) (pout - dist);
			if (dist == 1) { // in PNG a single byte is usually copied multiple times
				while (copy_len--) { *pout++ = *copy_val; }
			} else {
				// dist < length works here as well, as it will just begin copying itself
				while (copy_len--) { *pout++ = *copy_val++; }
			}
		}
	}
	return 1;
}

// Computes the scanlines from the zstream
static int decode_zlib_stream(zlib_stream *zstream, uint8_t *scanlines) {
	if (!valid_zlib_header(zstream->buffer)) return 0;

	zstream->nbits = 0;
	zstream->bit_buffer = 0;
	int bfinal, btype;
	uint8_t *out = scanlines;

	//could be multiple data blocks, bfinal = 1 indicates the last block
	do {
		bfinal = getzbits(zstream, 1);
		btype = getzbits(zstream, 2);
		if (btype == 3) {
			return 0; //invalid btype
		} else if (btype == 0) {
			// parse the uncompressed data block
			getzbits(zstream, zstream->nbits & 7); //skip the rest of the byte
			int len = getzbits(zstream, 16);
			int nlen = getzbits(zstream, 16);
			
			if (nlen != (len ^ 0xffff)) return 0; //validate that nlen is the 1's complement of len, ~len would work if they were unsigned ints
			
			memcpy(out, zstream->buffer, len);
			out += len;
			// std::cout <<  std::hex << len <<std::endl;
			// std::cout <<  std::hex << nlen <<std::endl;
			// // std::cout <<  std::hex << zstream->nbits <<std::endl;

			// std::cout <<  std::hex << get4bytes(&zstream->buffer) <<std::endl;
		} else {
			if (btype == 1) {
				// generate default literal and distance codes
				if (!gen_prefix_codes(&zstream->lit_alphabet, fixed_lit_sizes, 288)) return 0; // generate literal alphabet
				if (!gen_prefix_codes(&zstream->dist_alphabet, fixed_distance_sizes, 32)) return 0; // generate literal alphabet

			} else {
				//parse dynamic huffman to generate literal and distance codes
				if (!calculate_huffman_code(zstream)) return 0;
			}
			// parse the huffman blocks using the generated literal and distance codes
			if (!inflate_huffman_block(zstream, out)) return 0;
		}


	} while (!bfinal);
	
	return 1;
}

// Paeth predictor implemented with ternary operators
static int paeth(int a, int b, int c) {
	//TODO: find a more efficient algorithm?
	int p = a + b - c;

	int pa = p-a < 0 ? a-p : p-a;
	int pb = p-b < 0 ? b-p : p-b;
	int pc = p-c < 0 ? c-p : p-c;

	int res = ((pa <= pb) & (pa <= pc)) ? a : (pb <= pc) ? b: c;
	return res;
}

// Writes the de-filtered scanlines to the image->colorarray
static int createRGBAarray(PNG_data *image, uint8_t *scanlines) {
	// only support depth = 8
	// int bytes = (image->depth == 16 ? 2 : 1);//bytes per components
	int bpl = image->channels * image->width;
	int sep = image->channels;

	uint8_t *scan_buff = (uint8_t *) malloc(2 * bpl);
	uint8_t first_line_filter[5] = { 0, 1, 0, 5, 1 }; // b and c = 0 for the first row, choose a more optimized filter

	int y, i;
	for (y = 0; y < image->height; y++) {
		uint8_t *cur_line = scan_buff + (y & 1) * bpl;
		uint8_t *prev_line = scan_buff + (~y & 1) * bpl;

		int filter = *scanlines++;

		if (filter > 4) return 0; //invalid value for filter
		if (y == 0) filter = first_line_filter[filter];

		// x: scanline[i], a: cur_line[i-sep], b = prev_line[i], c = prev_line[i-sep]
		switch (filter) {
			case 0: //NONE x
				memcpy(cur_line, scanlines, bpl);
				break;
			case 1: //SUB x + a
				memcpy(cur_line, scanlines, bpl); // x + 0
				for(i = sep; i < bpl; i++) { // x + a
					cur_line[i] = (scanlines[i] + cur_line[i-sep]) & 255; // the &255 ensures that the result doesn't exceed 255/8bits
				}
				break;
			case 2: //UP x + b
				for(i = 0; i < bpl; i++) { // x + b
					cur_line[i] = (scanlines[i] + prev_line[i]) & 255;
					}
				break;
			case 3: //AVG x + floor((a+b)/2)
				for (i = 0; i < sep; i++ ) { // x + floor((0+b)/2)
					cur_line[i] = (scanlines[i] + (prev_line[i] >> 1)) & 255; // >>1 acts as the floor(x/2) required for avg
				}
				for (i = sep; i < bpl; i++) { // x + floor((a+b)/2)
					cur_line[i] = (scanlines[i] + ((cur_line[i-sep] + prev_line[i]) >> 1)) & 255;
				}
				break;
			case 4: //PAETH x + paeth(a, b, c)
				for(i = 0; i < sep; i++) { // paeth(0,b,0) = b
					cur_line[i] = (scanlines[i] + prev_line[i]) & 255;
				}
				for (i = sep; i < bpl; i++) {
					cur_line[i] = (scanlines[i] + paeth(cur_line[i-sep], prev_line[i], prev_line[i-sep])) & 255;
				}
				break;
			case 5: //AVG_FIRST x = floor((a+0)/2)
				memcpy(cur_line, scanlines, bpl); // First pixel is untouched
				for (i = sep; i < bpl; i++) {
					cur_line[i] = (scanlines[i] + (cur_line[i-sep] >> 1)) & 255;
				}
				break;
		}
		scanlines += bpl;

		memcpy(image->colorarray + bpl * y, cur_line, bpl);
	}
	free(scan_buff);
	return 1;
}

// Parse the bytes in buffer into an image
static int parsePNG(uint8_t *buffer, PNG_data *image) {
    uint8_t *bp = buffer;
	int first = 1, color = 0, interlace = 0;
	uint32_t dataoffset = 0;

    if (!validsignature(bp)) return 0;

	zlib_stream zstream;
	zstream.buffer = NULL;

	PNG_chunk_header chunk;
	
    for (;;) {
		getchunkheader(bp, &chunk);
		switch(chunk.type) {
			case CHUNK_TYPE('I','H','D','R'):
				if (!first) return 0; //TODO: Add error handling
				first = 0;
				if (chunk.length != 13) return 0; //TODO: Add error handling
				image->width = get4bytes(bp);
				image->height = get4bytes(bp);
				if (image->width == 0 || image->height == 0) return 0; // 0 width or height is invalid //TODO: Add error handling
				image->depth = getbyte(bp); if (image->depth != 8 && image->depth != 16) return 0; // only support 8 for now, explore 16 later //TODO: Add error handling
				color = getbyte(bp); if (color != 0 && color != 2 && color != 4 && color != 6) return 0; //only RGB and RGBA are supported for this simple decoder //TODO: Add error handling
				int comp, filter;
				comp = getbyte(bp); if (comp) return 0; //comp has to be 0 //TODO: Add error handling
				filter = getbyte(bp); if (filter) return 0; //filter has to be 0 //TODO: Add error handling
				interlace = getbyte(bp); if (interlace) return 0; //only 0 and 1 are valid interlaces, but only support 0 //TODO: Add error handling
				image->channels = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
				//* color	channels	1st ternary	2nd ternary
				//*	0		1 gray		1			0
				//*	2		3 RGB		3			0
				//*	3		1 index		1			0
				//*	4		2 grayA		1			1
				//*	6		4 RGBA		3			1
				
				// if ((1 << 30) / data->width / data->channels < data->height) return 0; //image would be too large //TODO: Add error handling

				skipnbytes(bp, 4); //Skip over the CRC bytes
          		break;

			case CHUNK_TYPE('I','D','A','T'):
				if (first) return 0; //TODO: Add error handling
				if (chunk.length > (1u << 31) - 1) return 0; //chunk length cannot exceed 2^31-1 //TODO: Add error handling
		
				uint8_t *pdata;
				pdata = (uint8_t *) realloc(zstream.buffer, dataoffset + chunk.length); //Resize the rawdata block to be able to recieve the data from the IDAT chunk
				zstream.buffer = pdata;

				if (!getnbytes(bp, zstream.buffer + dataoffset, chunk.length)) return 0; //TODO: Add error handling
				dataoffset += chunk.length;
				
				skipnbytes(bp, 4); //Skip over the CRC bytes
				break;

			case CHUNK_TYPE('I','E','N','D'):
				if (first) return 0; //TODO: Add error handling
	 			if (zstream.buffer == NULL) return 0; //TODO: Add error handling
				int bpl, imglen;
				bpl = 1 + (image->width * image->depth * image->channels / 8);
				imglen = bpl * image->height;
				
				image->scanlines = NULL;
				image->scanlines = (uint8_t *) realloc(image->scanlines, imglen);
				
				if (!decode_zlib_stream(&zstream, image->scanlines)) return 0;

				image->colorarray = NULL;
				image->colorarray = (uint8_t *) realloc(image->colorarray, image->width * image->height * image->channels);
				
				if (!createRGBAarray(image, image->scanlines)) return 0; //TODO: Add error handling

				free(image->scanlines);

				return 1;

			default:
				//incase the file contains png headers that are not supported, such as PLTE and tRNS
				if (first) return 0; //TODO: Add error handling
		}
	}
    
    return 1;
}

static void setbyte(uint8_t *&pbuffer, uint8_t value) {
	*(pbuffer++) = value;
}

static void set4bytes(uint8_t *&pbuffer, int value) {
	setbyte(pbuffer, (uint8_t) (value >> 24));
	setbyte(pbuffer, (uint8_t) (value >> 16));
	setbyte(pbuffer, (uint8_t) (value >> 8));
	setbyte(pbuffer, (uint8_t) (value));
}

static void setnbytes(uint8_t *&pbuffer, uint8_t *bytes, int n) {
	memcpy(pbuffer, bytes, n);
	pbuffer += n;
	// (*pbuffer) += n;
}

// Generated as per https://www.w3.org/TR/png-3/#D-CRCAppendix
static const uint32_t crc_table[256] = 
{
    0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
	0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
	0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
	0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
	0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
	0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
	0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
	0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
	0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
	0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
	0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
	0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
	0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
	0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
	0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
	0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
	0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
	0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
	0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
	0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
	0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
	0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
	0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
	0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
	0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
	0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
	0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
	0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
	0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
	0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
	0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
	0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
};

// Sets the CRC bytes based on the previous n bytes
static void setCRC(uint8_t *&pbuffer, int n) {
	uint8_t *chunk = pbuffer - n;
	uint32_t crc = ~0;

	int i;
	for (i = 0; i < n; i++) {
		crc = crc_table[chunk[i] ^ (crc & 0xff)] ^ (crc >> 8);
	}

	set4bytes(pbuffer, ~crc);
}

// Apply filter to the current line for filters != 0
static void filterline(uint8_t *out_line, uint8_t *cur_line, int filter, int bpl, int sep) {
	int i;
	switch (filter) {
		// Should be protected
		// case 0: //NONE x 
		// 	memcpy(out_line, cur_line, bpl);
		case 1: //SUB x - a
			memcpy(out_line, cur_line, sep); // x + 0
			for(i = sep; i < bpl; i++) { // x + a
				out_line[i] = (cur_line[i] - cur_line[i-sep]) & 255; // the &255 ensures that the result doesn't exceed 255/8bits
			}
			return;
		case 2: //UP x - b
			for(i = 0; i < bpl; i++) { // x + b
				out_line[i] = (cur_line[i] - cur_line[i-bpl]) & 255;
				}
			return;
		case 3: //AVG x - floor((a+b)/2)
			for (i = 0; i < sep; i++ ) { // x + floor((0+b)/2)
				out_line[i] = (cur_line[i] - (cur_line[i-bpl] >> 1)) & 255; // >>1 acts as the floor(x/2) required for avg
			}
			for (i = sep; i < bpl; i++) { // x + floor((a+b)/2)
				out_line[i] = (cur_line[i] - ((cur_line[i-sep] + cur_line[i-bpl]) >> 1)) & 255;
			}
			return;
		case 4: //PAETH x - paeth(a, b, c)
			for(i = 0; i < sep; i++) { // paeth(0,b,0) = b
				out_line[i] = (cur_line[i] - (cur_line-bpl)[i]) & 255;
			}
			for (i = sep; i < bpl; i++) {
				out_line[i] = (cur_line[i] - paeth(cur_line[i-sep], cur_line[i-bpl], cur_line[i-bpl-sep])) & 255;
			}
			return;
		case 5: //AVG_FIRST x = floor((a+0)/2)
			memcpy(out_line, cur_line, sep); // First pixel is untouched
			for (i = sep; i < bpl; i++) {
				out_line[i] = (cur_line[i] - (cur_line[i-sep] >> 1)) & 255;
			}
			return;
	}
}

// Determine the best filter for each scanline and apply it, returning a buffer of the filtered scanlines
static void applyfilter(uint8_t *out_buff, uint8_t *pixels, int width, int height, int channels) {
	int bpl = channels * width;
	int sep = channels;
	int best_filt, filter;

	uint8_t first_line_filter[5] = { 0, 1, 0, 5, 1 };
	uint8_t filter_order[5] = { 4, 3, 2, 0, 1 }; // Start at higher filter type, since 0 and 1 are more common
	uint8_t *filtered_line = (uint8_t *) malloc(bpl);
	uint8_t *out = out_buff;

	int y, f, i;
	for (y = 0; y < height; y++) {
		unsigned int ent, best_ent = ~0u;

		for (f = 0; f < 5; f++) {
			filter = y == 0 ? first_line_filter[f] : f;
			
			if (filter == 0 ) memcpy(filtered_line, pixels + (y * bpl), bpl);
			else filterline(filtered_line, pixels + (y * bpl), filter, bpl, sep);

			// Calculate entropy for current filter type, we want the lowest ent
			ent = 0;
			for (i = 0; i < bpl; i++) ent += filtered_line[i];
			// ent /= width;
			if (ent < best_ent) {
				best_ent = ent;
				best_filt = f;
			}
		}

		if (filter != best_filt) {
			filter = y == 0 ? first_line_filter[best_filt] : best_filt;
			if (filter == 0 ) memcpy(filtered_line, pixels + (y * bpl), bpl);
			else filterline(filtered_line, pixels + (y * bpl), filter, bpl, sep);
		}	
	
		*out++ = best_filt;
		memcpy(out, filtered_line, bpl);
		out += bpl;
	}
	free(filtered_line);
}

#define LSB2BYTES(x) ((x & 0xff00) >> 8) | ((x & 0xff) << 8)

static uint8_t *uncompressed_data(uint8_t *scanlines, int scan_len, int *comp_len) {
	if (!scan_len) return NULL;
	
	*comp_len = scan_len + 7;
	uint8_t *uncomp_data = (uint8_t *) malloc(*comp_len);
	uint8_t *p_uncomp = uncomp_data; // dynamic ptr

	setbyte(p_uncomp, 8);
	setbyte(p_uncomp, 0x1d);
	setbyte(p_uncomp, 1);

	uint16_t rawlen = LSB2BYTES(scan_len);
	set4bytes(p_uncomp, (rawlen << 16) + rawlen ^ 0xffff);
	setnbytes(p_uncomp, scanlines, scan_len);
	
	return uncomp_data;
}

// Use fixed/dynamic huffman encoding to compress the data, depending on which is more efficient
static uint8_t *deflate_scanlines(uint8_t *scanlines, int scan_len, int *comp_len) {
	
	return 0;
}

// #define UNCOMPRESSED_MODE // Use this flag to tell the PNG writer to use the uncompressed mode

// Generate the PNG buffer from a pixel buffer and characteristics
uint8_t *genPNG(uint8_t *pixels, int width, int height, int channels, int *len) {
    uint8_t png_sig[8] = { 137,80,78,71,13,10,26,10 };
	uint8_t colors[4] = { 0, 4, 2, 6 };

	int scan_len = width * height * channels;
	if (!scan_len) return NULL; // if any one of the inputs are 0, there will be no valid output
	scan_len += height;
	int comp_len;

	uint8_t *scanlines = (uint8_t *) malloc(scan_len);
	applyfilter(scanlines, pixels, width, height, channels);

	uint8_t *comp_buff;
#ifdef UNCOMPRESSED_MODE
	comp_buff = uncompressed_data(scanlines, scan_len, &comp_len); // option for writing as uncompressed image
#else
	comp_buff = deflate_scanlines(scanlines, scan_len, &comp_len);
#endif
	//zlib compression
	free(scanlines);

	*len = 8 + 12+13 + 12+comp_len +12;
	uint8_t *png_buff = (uint8_t *) malloc(*len);
	uint8_t *bp = png_buff;

	// PNG Signature
	setnbytes(bp, png_sig, 8);
	// IHDR chunk
	set4bytes(bp, 13); // Length
	set4bytes(bp, CHUNK_TYPE('I', 'H', 'D', 'R'));
	set4bytes(bp, width);
	set4bytes(bp, height);
	setbyte(bp, 8); // Depth
	setbyte(bp, colors[channels-1]);
	setbyte(bp, 0); // Comp
	setbyte(bp, 0); // Filter
	setbyte(bp, 0); // Interlace
	setCRC(bp, 13+4);
	// IDAT chunk
	set4bytes(bp, comp_len); // Length
	set4bytes(bp, CHUNK_TYPE('I', 'D', 'A', 'T'));
	setnbytes(bp, comp_buff, comp_len);
	setCRC(bp, comp_len+4);
	// IEND chunk
	set4bytes(bp, 0); // Length
	set4bytes(bp, CHUNK_TYPE('I', 'E', 'N', 'D'));
	set4bytes(bp, 0xae426082); // Standard CRC no point calculating
	// setCRC(bp,0+4);

	printf("{ ");
	for (int i=0; i<(*len); i++) {
		printf("%02x, ", png_buff[i]);
		// if (i == 40 || i== 40+comp_len) printf("|");
	}
	printf("}\n");
	
	free(comp_buff);
	return png_buff;
}

// Read the png
uint8_t *openPNG(const char *filename, int *width, int *height, int *channels) {
    FILE *fp = fopen(filename, "rb");
	// TODO: catch error if fopen is unable to open a file, keeps seg faulting for wtv reason

    fseek(fp, 0, SEEK_END);
    int len = (int) ftell(fp);
    rewind(fp);

	uint8_t buff[len];
	
    fread(buff, 1, len, fp);
    fclose(fp);

	PNG_data image;

	if (!parsePNG(buff, &image)) {
		free(image.scanlines);
		free(image.colorarray);
		image.colorarray = NULL;
	}
	*width = image.width;
	*height = image.height;
	*channels = image.channels;
	
	return image.colorarray;
}

int writePNG(const char *filename, int width, int height, int channels, void *pixels) {
	int len;
	uint8_t *png_buff = genPNG((uint8_t *) pixels, width, height, channels, &len);
	return 0; // Pause
	if (png_buff == NULL) return 0;

	FILE *fp = fopen(filename, "wb");
	if (!fp) return 0;
	
    fwrite(png_buff, 1, len, fp);
    fclose(fp);

	free(png_buff);
	
	return 1;
}
// generatePNG
// applyfilter

// generatecodelengths