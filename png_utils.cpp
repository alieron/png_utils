#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>


// Testing
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

// Draws the colorarray in the terminal via ANSI ESC codes, works with grayscale, grayscaleA, RGB and RGBA
void drawRGBAarray(int width, int height, int nchannels, unsigned char *img_RGBA){
    for (int h=0; h<height; h++){
        for (int w=0; w<width; w++){
			int index = nchannels*(h*width+w);
			
            if ((~nchannels & 1) && !(img_RGBA[index+(nchannels-1)])) {
                printf("\x1b[0m  ");
            } else {
 				printf("\x1b[48;2;%i;%i;%im  ", (int)img_RGBA[index], (int)img_RGBA[index+(nchannels/3)], (int)img_RGBA[index+2*(nchannels/3)]);
			}
        }
        printf("\x1b[0m\n");
    }
}

// Returns the current value that pbuffer points to and increments pbuffer
static uint8_t getbyte(uint8_t **pbuffer) {
	// no protection end of buffer is reached
	return *((*pbuffer)++);
}

// Returns 4 consecutive bytes as an int, useful for getting 4 byte chunks
static uint32_t get4bytes(uint8_t **pbuffer) {
  	return (uint32_t) (getbyte(pbuffer) << 24) + (getbyte(pbuffer) << 16) + (getbyte(pbuffer) << 8) + getbyte(pbuffer);
}

// Jump forward in the image byte buffer by n bytes
static void skipnbytes(uint8_t **pbuffer, unsigned int n) {
	// no protection end of buffer is reached
	(*pbuffer) += n;
}

// Returns n consequtive bytes as an array of unsigned chars, useful for getting chunks of data
static int getnbytes(uint8_t **pbuffer, uint8_t *bytes_out, int n) {
	// no protection end of buffer is reached
	memcpy(bytes_out, *pbuffer, n);
	(*pbuffer) += n;
	return 1;
}

// Verifies that the first 8 bytes of the buffer corresponds to the png file signature
static int validsignature(uint8_t **pbuffer) {
	static const uint8_t png_sig[8] = { 137,80,78,71,13,10,26,10 };

	int i = 0;
	for (i = 0; i < 8; ++i) 
		if (getbyte(pbuffer) != png_sig[i]) return 0; //TODO: Add error handling
	return 1;
}

// Returns the length and type of the current chunk from the first 8 bytes of the chunk
static void getchunkheader(uint8_t **pbuffer, PNG_chunk_header *chunk) {
	chunk->length = get4bytes(pbuffer);
	chunk->type = get4bytes(pbuffer);
}

// Refills the bit buffer such that there are at least n bits of data in it
static void refill_bit_buffer(zlib_stream *zstream, int n) {
	do {
		zstream->bit_buffer |= getbyte(&zstream->buffer) << zstream->nbits;
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
static int valid_zlib_header(uint8_t **zbuffer) {
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

static const uint8_t fixed_lit_sizes[288] = {
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

static const uint8_t fixed_distance_sizes[32] ={
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

static const int z_len_min[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static const uint8_t z_len_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int z_dist_min[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int z_dist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

// Deflate the huffman block in the zstream using the literal and distance alphabets
static int deflate_huffman_block(zlib_stream *zstream, uint8_t *scanlines) {
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
	if (!valid_zlib_header(&zstream->buffer)) return 0;

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
			if (!deflate_huffman_block(zstream, out)) return 0;
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
	return 1;
}

// Parse the bytes in buffer into an image
static int parsePNG(uint8_t *buffer, PNG_data *image) {
    uint8_t *bp = buffer;
	int first = 1, color = 0, interlace = 0;
	uint32_t dataoffset = 0;

    if (!validsignature(&bp)) return 0;

	zlib_stream zstream;
	zstream.buffer = NULL;

	PNG_chunk_header chunk;
	
    for (;;) {
		getchunkheader(&bp, &chunk);
		switch(chunk.type) {
			case CHUNK_TYPE('I','H','D','R'):
				if (!first) return 0; //TODO: Add error handling
				first = 0;
				if (chunk.length != 13) return 0; //TODO: Add error handling
				image->width = get4bytes(&bp);
				image->height = get4bytes(&bp);
				if (image->width == 0 || image->height == 0) return 0; // 0 width or height is invalid //TODO: Add error handling
				image->depth = getbyte(&bp); if (/*image->depth != 1 && image->depth != 2 && image->depth != 4 &&*/ image->depth != 8 && image->depth != 16) return 0; // only support 8 for now, explore 16 later //TODO: Add error handling
				color = getbyte(&bp); if (color != 0 && color != 2 && color != 4 && color != 6) return 0; //only RGB and RGBA are supported for this simple decoder //TODO: Add error handling
				int comp, filter;
				comp = getbyte(&bp); if (comp) return 0; //comp has to be 0 //TODO: Add error handling
				filter = getbyte(&bp); if (filter) return 0; //filter has to be 0 //TODO: Add error handling
				interlace = getbyte(&bp); if (interlace) return 0; //only 0 and 1 are valid interlaces, but only support 0 //TODO: Add error handling
				image->channels = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
				//* color	channels	1st ternary	2nd ternary
				//*	0		1 gray		1			0
				//*	2		3 RGB		3			0
				//*	3		1 index		1			0
				//*	4		2 grayA		1			1
				//*	6		4 RGBA		3			1
				
				// if ((1 << 30) / data->width / data->channels < data->height) return 0; //image would be too large //TODO: Add error handling

				skipnbytes(&bp, 4); //Skip over the CRC bytes
          		break;

			case CHUNK_TYPE('I','D','A','T'):
				if (first) return 0; //TODO: Add error handling
				if (chunk.length > (1u << 31) - 1) return 0; //chunk length cannot exceed 2^31-1 //TODO: Add error handling
		
				uint8_t *pdata;
				pdata = (uint8_t *) realloc(zstream.buffer, dataoffset + chunk.length); //Resize the rawdata block to be able to recieve the data from the IDAT chunk
				zstream.buffer = pdata;

				if (!getnbytes(&bp, zstream.buffer + dataoffset, chunk.length)) return 0; //TODO: Add error handling
				dataoffset += chunk.length;
				
				skipnbytes(&bp, 4); //Skip over the CRC bytes
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

				return 1;

			default:
				//incase the file contains png headers that are not supported, such as PLTE and tRNS
				if (first) return 0; //TODO: Add error handling
		}
	}
    
    return 1;
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
		image.colorarray = NULL;
	}
	*width = image.width;
	*height = image.height;
	*channels = image.channels;
	
	return image.colorarray;
}


int main() {
	// hexdumpfile("images/3x3_graya.png");

	int width, height, channels;
    uint8_t *RGBAarray = openPNG("images/3x3_gray.png", &width, &height, &channels);
	if (RGBAarray == NULL) return -1; // catch error if any
	
	drawRGBAarray(width, height, channels, RGBAarray);   
}