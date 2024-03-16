#include <iostream>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
void hexdumpPNG(const char *filename) {
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
	uint8_t *buffer;
	uint32_t bit_buffer, nbits;
} zlib_stream;

typedef struct{
	uint32_t length;
	uint32_t type;
} PNG_chunk_header;

#define CHUNK_TYPE(a,b,c,d) (((unsigned) a << 24) + ((unsigned) b << 16) + ((unsigned) c << 8) + (unsigned) d)

// Draws the colorarray in the terminal via ANSI ESC codes, works with RGB and RGBA only
void drawRGBAarray(int width, int height, int nchannels, unsigned char *img_RGBA){
    for (int h=0; h<height; h++){
        for (int w=0; w<width; w++){
			int index = nchannels*(h*width+w);

            if (nchannels == 4 && !(img_RGBA[index+3])) {
                printf("\x1b[0m ");
            } else {
 				printf("\x1b[48;2;%i;%i;%im  ", (int)img_RGBA[index], (int)img_RGBA[index+1], (int)img_RGBA[index+2]);
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
	//TODO add a check and error handling for if the desired length exceeds the end of pbuffer
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
PNG_chunk_header getchunkheader(uint8_t **pbuffer) {
	PNG_chunk_header chunk;
	chunk.length = get4bytes(pbuffer);
	chunk.type = get4bytes(pbuffer);
	return chunk;
}

// Get last n bits from the right most byte in the zstream
static uint32_t getzbits(zlib_stream *zstream, int n) {
	// not protected if the n supplied is more then 32
	uint32_t bits;
	if (zstream->nbits < n) {
		// replenish bits if not enough, up to n bits max
		do {
			zstream->bit_buffer |= getbyte(&zstream->buffer) << zstream->nbits;
			zstream->nbits += 8;
		} while (zstream->nbits < n);
	}
	bits = zstream->bit_buffer & ((1 << n) - 1); //get the last n bits in the bit buffer
	zstream->nbits -= n;
	zstream->bit_buffer >>= n;
	return bits;
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
				// parse fixed huffman
		
			} else {
				//parse dynamic huffman
			}
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
static int createRGBAarray(PNG_data *image, uint8_t *scanlines, int interlace) {
	if (interlace) {
		//deinterlace maybe?
		 return 0; //TODO: support for interlace = 1
	}

	int bytes = (image->depth == 16 ? 2 : 1);//bytes per components
	int bpl = image->channels * bytes * image->width;
	int sep = image->channels * bytes;

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
	
    for (;;) {
		PNG_chunk_header chunk = getchunkheader(&bp);
		switch(chunk.type) {
			case CHUNK_TYPE('I','H','D','R'):
				if (!first) return 0; //TODO: Add error handling
				first = 0;
				if (chunk.length != 13) return 0; //TODO: Add error handling
				image->width = get4bytes(&bp);
				image->height = get4bytes(&bp);
				if (image->width == 0 || image->height == 0) return 0; // 0 width or height is invalid //TODO: Add error handling
				image->depth = getbyte(&bp); if (image->depth != 1 && image->depth != 2 && image->depth != 4 && image->depth != 8 && image->depth != 16) return 0; //TODO: Add error handling
				color = getbyte(&bp); if (color != 2 && color != 6) return 0; //only RGB and RGBA are supported for this simple decoder //TODO: Add error handling
				int comp, filter;
				comp = getbyte(&bp); if (comp) return 0; //comp has to be 0 //TODO: Add error handling
				filter = getbyte(&bp); if (filter) return 0; //filter has to be 0 //TODO: Add error handling
				interlace = getbyte(&bp); if (interlace) /*(interlace>1)*/ return 0; //only 0 and 1 are valid interlaces, but only support 0 for now //TODO: Add error handling
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
				image->scanlines = (uint8_t*) realloc(image->scanlines, imglen);
				
				if (!decode_zlib_stream(&zstream, image->scanlines)) return 0;

				image->colorarray = NULL;
				image->colorarray = (uint8_t*) realloc(image->colorarray, image->width * image->height * image->channels);
				
				if (!createRGBAarray(image, image->scanlines, interlace)) return 0; //TODO: Add error handling

				return 1;

			default:
				//incase the file contains png headers that are not supported, such as PLTE and tRNS
				if (first) return 0; //TODO: Add error handling
		}
	}
    
    return 1;
}

// PNG_data openPNG(const char *filename) {
uint8_t *openPNG(const char *filename, int *width, int *height, int *channels) {
    FILE *fp = fopen(filename, "rb");
    
    fseek(fp, 0, SEEK_END);
    int len = (int) ftell(fp);
    rewind(fp);

	uint8_t buff[len];
	
    fread(buff, 1, len, fp);
    fclose(fp);

	PNG_data image;

	if (!parsePNG(buff, &image)) {
		image.colorarray = nullptr;
	}
	*width = image.width;
	*height = image.height;
	*channels = image.channels;

	// hexdump(buff, sizeof(buff));
	
	// return image;
	return image.colorarray;
}


int main() {
	// hexdumpPNG("2x2_uncompressed.png");

    // PNG_data pngdata = openPNG("2x2_uncompressed.png");
	int width, height, channels;
    uint8_t *RGBAarray = openPNG("2x2_uncompressed.png", &width, &height, &channels);
    // uint8_t *RGBAarray = openPNG("oak_log_uncompressed.png", &width, &height, &channels);


	drawRGBAarray(width, height, channels, RGBAarray);

	// unsigned char* image = stbi_load("2x2_blue.png", &width, &height, &channels, 0);
	// drawRGBAarray(width, height, channels, image);
	// stbi_image_free(image);    
}