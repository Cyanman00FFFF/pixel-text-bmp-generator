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

/*
============ Update plan ============

- Remove magic numbers
	- Hardcoded string sizes mainly
- Fully custom capitalization
- Include all (printable) ASCII
	- '_' can't be used as spaces
- Styling: Margins and sizing
- Make character files use 0x0 and 0x1, not '0' and '1'
- Better error handling
	- Auto build generated/ if it doesn't exist
	- Usage message

*/

typedef enum {
	ERR_NONE,
	ERR_INVALID_FLAG,
	ERR_MARGIN_SETTINGS,
	ERR_FOREGROUND_COLOR,
	ERR_BACKGROUND_COLOR,
} ErrorType;

typedef struct {
	ErrorType type;
	char *excerpt;
} ErrorInfo;

const int MARGIN_CAP = 1024; // How many pixels of margin are allowed in any one direction. Must be greater than 0 and less than INT_MAX.


ErrorType applyMargins(int *margins, char *marginInfo) {
	// store the margin info in a new variable to avoid overwriting argv
	char tempMargin[strlen(marginInfo)]; 
	strcpy(tempMargin, marginInfo);
	char *pTempMargin = &tempMargin[0]; // pointer for use with strtol
	
	for (int i = 0; i < 4; i++) {
		char *end; // pointer to next character after last converted by strtol
		const long marginCheck = strtol(pTempMargin, &end, 10);
		
		// there needs to be four margins. if pTempMargin is at end, then there was a number that was undetected.
		if (pTempMargin == end) {
			return ERR_MARGIN_SETTINGS;
		}
		
		if (marginCheck < 0 || marginCheck > MARGIN_CAP) {
			return ERR_MARGIN_SETTINGS;
		}
		
		pTempMargin = end + 1; // + 1 to ignore ':'
		margins[i] = marginCheck;
	}
	
	return ERR_NONE;
}

void printerr(ErrorInfo info) {
	switch (info.type) {
		case ERR_INVALID_FLAG: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: Flag '%s' did not match any valid flag\n\x1b[0m\x1b[37m", info.excerpt);
			break;
		}
		case ERR_MARGIN_SETTINGS: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: Margin (-m) input '%s' invalid. expected: top:bot:left:right (all must be unsigned integers 0-%d)\n\x1b[0m\x1b[37m", info.excerpt, MARGIN_CAP);
			break;
		} case ERR_FOREGROUND_COLOR: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: foreground color input '%s' invalid. expected: 8-digit hexadecimal unsigned integer\n  ex: 202020FF", info.excerpt);
			break;
		} case ERR_BACKGROUND_COLOR: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: background color input '%s' invalid. expected: 8-digit hexadecimal unsigned integer\n  ex: 202020FF", info.excerpt);
			break;
		}
	}
}


int main(int argc, char *argv[]) {
	// set defaults
	int margins[] = {0, 0, 0, 0};
	uint32_t fgColor = 0xFFFFFFFF;
	uint32_t bgColor = 0x000000FF;
	
	ErrorInfo errInfo = {.type = ERR_NONE, .excerpt = char [256]};
	
	for (int i = 1; i < argc; i++) {
		
		// flag handling
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "-m") == 0) {
				
				if (i == argc - 1) { // requires argument after
					err = ERR_MARGIN_SETTINGS;
				} else {
					err = applyMargins(margins, argv[i + 1]);
				}
				
			} else if (strcmp(argv[i], "--fg") == 0) {
				
				if (i == argc - 1) { // requires argument after
					err = ERR_FOREGROUND_COLOR;
				} else {
					err = applyColor(&fgColor, argv[i + 1]);
				}
				
			} else if (strcmp(argv[i], "--bg") == 0) {
				
				if (i == argc - 1) { // requires argument after
					err = ERR_BACKGROUND_COLOR;
				} else {
					err = applyColor(&bgColor, argv[i + 1]);
				}
				
			} else {
				// failed to match a flag
				err = ERR_INVALID_FLAG;
			}
			
			if (err != ERR_NONE) {
				printerr(errInfo);
				return 1;
			}
		}
		
	}
	
	
	return 0;
}