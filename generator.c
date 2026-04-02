#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t bfType;      // "BM" (0x4D42)
    uint32_t bfSize;      // File size in bytes
    uint16_t bfReserved1; // 0
    uint16_t bfReserved2; // 0
    uint32_t bfOffBits;   // Offset to pixel data
} BITMAPFILEHEADER;

typedef struct {
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t d;
	uint32_t e;
} ENDPOINTS;

typedef struct {
    uint32_t bV5Size;          // 40
    int32_t  bV5Width;         // Width in pixels
    int32_t  bV5Height;        // Height in pixels
    uint16_t bV5Planes;        // Must be 1
    uint16_t bV5BitCount;      // Bits per pixel (24, 32, etc.)
    uint32_t bV5Compression;   // 0 = BI_RGB
    uint32_t bV5SizeImage;     // Image data size (can be 0 for BI_RGB)
    int32_t  bV5XPelsPerMeter; // Usually 0
    int32_t  bV5YPelsPerMeter; // Usually 0
    uint32_t bV5ClrUsed;
    uint32_t bV5ClrImportant;  // 0
	uint32_t bV5RedMask;
	uint32_t bV5GreenMask;
	uint32_t bV5BlueMask;
	uint32_t bV5AlphaMask;
	uint32_t bV5CSType;
	ENDPOINTS bV5Endpoints;
	uint32_t bV5GammaRed;
	uint32_t bV5GammaGreen;
	uint32_t bV5GammaBlue;
	uint32_t bV5Intent;
	uint32_t bV5ProfileData;
	uint32_t bV5ProfileSize;
	uint32_t bV5Reserved;
} BITMAPV5HEADER;

#pragma pack(pop)

BITMAPFILEHEADER fileHeader = {
	.bfType = 0x4D42,
	.bfSize = 0,
	.bfReserved1 = 0,
	.bfReserved2 = 0,
	.bfOffBits = 138
};

BITMAPV5HEADER v5Header = {
	.bV5Size = 124,
    .bV5Width = 0,
    .bV5Height = 9,
    .bV5Planes = 1,
    .bV5BitCount = 32,
    .bV5Compression = 3,
    .bV5SizeImage = 0,
    .bV5XPelsPerMeter = 0,
    .bV5YPelsPerMeter = 0,
    .bV5ClrUsed = 0,
    .bV5ClrImportant = 0,
	.bV5RedMask = 0xFF000000,
	.bV5GreenMask = 0x00FF0000,
	.bV5BlueMask = 0x0000FF00,
	.bV5AlphaMask = 0x000000FF,
	.bV5CSType = 0x73524742,
	.bV5Endpoints = {0, 0, 0, 0, 0},
	.bV5GammaRed = 0,
	.bV5GammaGreen = 0,
	.bV5GammaBlue = 0,
	.bV5Intent = 4,
	.bV5ProfileData = 0,
	.bV5ProfileSize = 0,
	.bV5Reserved = 0
};

// argv: {'generator', text color, bg color, text to be generated}
int main(int argc, char *argv[]) {
	if (argc != 5) {
		return 1;
	}
	
	char mode = argv[1][0];
	uint32_t fgColor = strtoul(argv[2], NULL, 16);
	uint32_t bgColor = strtoul(argv[3], NULL, 16);
	// go through all characters in the input text
	int rowSize = strlen(argv[4])*6; // in pixels
	uint32_t pixels[9][rowSize];
	char prevChar = '_'; // first character should be uppercase, so we need a fake space before.
	for (int charID = 0; charID < strlen(argv[4]); charID++) {
		// makes the path to the character file that has the info for the bitmap, opens it.
		char characterPath[23] = "characters_";
		// if the last character is a space, make this character uppercase to achieve titlecase
		if (prevChar == '_' || mode == 'C') {
			strcat(characterPath, "upper/ .txt");
		} else {
			strcat(characterPath, "lower/ .txt");
		}
		prevChar = argv[4][charID];
		characterPath[17] = argv[4][charID];
		FILE *characterFile = fopen(characterPath, "r");
		
		// build pixel array for current character.
		for (int row = 0; row < 9; row++) {
			for (int pixel = 0; pixel < 6; pixel++) {
				int currentPixel = fgetc(characterFile);
				if (currentPixel == '0') {
					pixels[row][(charID*6) + pixel] = bgColor;
				} else {
					pixels[row][(charID*6) + pixel] = fgColor;
				}
			}
			fgetc(characterFile); // remove the newline
		}
		fclose(characterFile);
	}
	
	// finish header
	v5Header.bV5Width = rowSize;
	fileHeader.bfSize = fileHeader.bfOffBits + ((v5Header.bV5Width * v5Header.bV5Height) * 4);
	
	// Form filename with same name as input text
	char outputFilename[strlen(argv[4])+15];
	strcpy(outputFilename, "generated/");
	strcat(outputFilename, argv[4]);
	strcat(outputFilename, ".bmp");
	
	// Write to file
	FILE *bitmap = fopen(outputFilename, "wb");
	fwrite(&fileHeader, sizeof(fileHeader), 1, bitmap);
	fwrite(&v5Header, sizeof(v5Header), 1, bitmap);
	fwrite(pixels, sizeof(uint32_t), sizeof(pixels)/sizeof(uint32_t), bitmap);
	fclose(bitmap);
	
	return 0;
}