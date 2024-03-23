#include <stdio.h>
#include <ctype.h>
#include <iostream>

#include "png_utils.h"

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

int main() {
	// hexdumpfile("images/3x3_graya.png");

	int width, height, channels;
    uint8_t *RGBAarray = openPNG("images/6x6_dynamic_huffman.png", &width, &height, &channels);
	if (RGBAarray == NULL) return -1; // catch error if any
	
	drawRGBAarray(width, height, channels, RGBAarray);

	writePNG("test.png", width, height, channels, RGBAarray);
}