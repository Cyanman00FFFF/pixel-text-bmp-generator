#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

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
	ERR_HELP,
	ERR_NOARGS,
	ERR_INVALID_FLAG,
	ERR_MARGIN_SETTINGS,
	ERR_COLOR_VALUE,
} ErrorType;

typedef struct {
	ErrorType type;
	char *excerpt;
} ErrorInfo;


// Define constants

const int MARGIN_CAP = 1024; // How many pixels of margin are allowed in any one direction. Must be greater than 0 and less than INT_MAX.
const int CHARACTER_PATH_LEN = 30; // Will break if realpathlen > this. This is simply the length of the char array.



ErrorInfo applyMargins(int *margins, char *marginInfo) {
	ErrorInfo errInfo = (ErrorInfo) {ERR_NONE, ""};
	
	char *pMarginInfo = &marginInfo[0]; // pointer for use with strtol
	
	for (int i = 0; i < 4; i++) {
		char *end; // pointer to next character after last converted by strtol
		const long marginCheck = strtol(pMarginInfo, &end, 10);
		
		// there needs to be four margins. if pMarginInfo is at end, then there was a number that was undetected.
		if (pMarginInfo == end || marginCheck < 0 || marginCheck > MARGIN_CAP) {
			errInfo = (ErrorInfo) {ERR_MARGIN_SETTINGS, marginInfo};
			return errInfo;
		}
		
		pMarginInfo = end + 1; // + 1 to ignore ':'
		margins[i] = marginCheck;
	}
	
	return errInfo;
}

ErrorInfo applyColor(uint32_t *color, char *colorInfo) {
	ErrorInfo errInfo = (ErrorInfo) {ERR_NONE, ""};
	
	if (strlen(colorInfo) != 8) {
		errInfo = (ErrorInfo) {ERR_COLOR_VALUE, colorInfo};
	}
	char *pColorInfo = &colorInfo[0]; // pointer for use with strtol
	
	char *end; // pointer to next character after last converted by strtol
	const unsigned long colorCheck = strtoul(pColorInfo, &end, 16);
	
	// there needs to be four margins. if pColorInfo is at end, then there was a number that was undetected.
	if (pColorInfo == end || colorCheck < 0 || colorCheck > 0xFFFFFFFF) {
		errInfo = (ErrorInfo) {ERR_COLOR_VALUE, colorInfo};
		return errInfo;
	}
	
	*color = colorCheck;
	
	return errInfo;
}

// convert a character from its input state to a string that is allowed for the filename
// uses html names when possible
char *charToFilename(char input) {
	switch (input) {
		case ' ': return "space";
		case '"': return "quot";
		case '\'': return "apos";
		case '*': return "ast";
		case '/': return "sol";
		case ':': return "colon";
		case '<': return "lt";
		case '>': return "gt";
		case '?': return "quest";
		case '\\': return "bsol";
		case '|': return "verbar";
	}
}

void printerr(ErrorInfo info) {
	switch (info.type) {
		case ERR_HELP: {
			fprintf(stderr,
				"usage: generator [-m <margin-settings>] [--fg <color>] [--bg <color>] \"<text>\"\n\n"
				"  -m <margin-settings>  specify how many pixels to put on each side of the text.\n"
				"                        format: top:bot:left:right (all must be unsigned integers 0-%d)\n"
				"  --fg <color>          specify the foreground (text) color as an 8-digit hexadecimal unsigned integer.\n"
				"  --bg <color>          specify the background color as an 8-digit hexadecimal unsigned integer.\n"
				"  <text>                the text string to be converted into an image.\n",
				MARGIN_CAP
			);
			break;
		}
		case ERR_NOARGS: {
			fprintf(stderr, 
				"\x1b[1m\x1b[91merror: No arguments specified (expected at least 1)\n\x1b[0m\x1b[37m"
				"usage: generator [-m <margin-settings>] [--fg <color>] [--bg <color>] \"<text>\"\n"
				"see 'generator -h' for full help.\n"
			);
			break;
		}
		case ERR_INVALID_FLAG: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: Flag '%s' did not match any valid flag\n\x1b[0m\x1b[37m", info.excerpt);
			break;
		}
		case ERR_MARGIN_SETTINGS: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: Margin (-m) input '%s' invalid. expected: top:bot:left:right (all must be unsigned integers 0-%d)\n\x1b[0m\x1b[37m", info.excerpt, MARGIN_CAP);
			break;
		} case ERR_COLOR_VALUE: {
			fprintf(stderr, "\x1b[1m\x1b[91merror: color (--fg or --bg) input '%s' invalid. expected: 8-digit hexadecimal unsigned integer\n\x1b[0m\x1b[37mex: 202020FF\n", info.excerpt);
			break;
		}
	}
}


int main(int argc, char *argv[]) {
	// user specified no arguments, print usage
	if (argc == 1) {
		printerr((ErrorInfo) {ERR_NOARGS, ""});
	}
	
	// set defaults
	int margins[] = {0, 0, 0, 0};
	uint32_t fgColor = 0xFFFFFFFF;
	uint32_t bgColor = 0x000000FF;
	
	ErrorInfo errInfo;
	
	
	for (int i = 1; i < argc; i++) {
		
		if (argv[i][0] == '-') {
			// flag handling
			
			
			if (strcmp(argv[i], "-h") == 0) {
				
				errInfo = (ErrorInfo) {ERR_HELP, ""};
				
			} else if (strcmp(argv[i], "-m") == 0) {
				
				if (i == argc - 1) { // requires argument after
					errInfo = (ErrorInfo) {ERR_MARGIN_SETTINGS, ""};
				} else {
					errInfo = applyMargins(margins, argv[i + 1]);
				}
				
			} else if (strcmp(argv[i], "--fg") == 0) {
				
				if (i == argc - 1) { // requires argument after
					errInfo = (ErrorInfo) {ERR_COLOR_VALUE, ""};
				} else {
					errInfo = applyColor(&fgColor, argv[i + 1]);
				}
				
			} else if (strcmp(argv[i], "--bg") == 0) {
				
				if (i == argc - 1) { // requires argument after
					errInfo = (ErrorInfo) {ERR_COLOR_VALUE, ""};
				} else {
					errInfo = applyColor(&bgColor, argv[i + 1]);
				}
				
			} else {
				// failed to match a flag
				errInfo = (ErrorInfo) {ERR_INVALID_FLAG, argv[i]};
			}
			
			if (errInfo.type != ERR_NONE) {
				printerr(errInfo);
				return 1;
			}
			
			
		} else {
			// text input handling
			
			
			//              top + charHeight + bot       left + allCharsWidth + right-1 (compensates for extra pixel after last char)
			uint32_t pixels[margins[0] + 9 + margins[1]][margins[2] + strlen(argv[i])*6 + margins[3]-1];
			
			// initialize the pixel array to the background color
			for (int row = 0; row < sizeof(pixels)/sizeof(pixels[0]); row++) {
				for (int col = 0; col < sizeof(pixels[0])/sizeof(pixels[0][0]); col++) {
					pixels[row][col] = bgColor;
				}
			}
			
			// insert fg color where character files indicate
			for (int charID = 0; charID < strlen(argv[i]); charID++) {
				char characterPath[CHARACTER_PATH_LEN];
				strncpy(characterPath, "characters_", CHARACTER_PATH_LEN);
				// subtract current characterPath length (with null) from total to get available space for the folder and file name
				char folder[CHARACTER_PATH_LEN - strlen(characterPath) - 1];
				
				if (argv[i][charID] >= 'A' && argv[i][charID] <= 'Z') {
					// this character is uppercase, so get the data from the uppercase folder
					sprintf(folder, "upper/%c", argv[i][charID]);
					strncat(characterPath, folder, CHARACTER_PATH_LEN - strlen(characterPath) - 1);
				} else if (argv[i][charID] >= 'a' && argv[i][charID] <= 'z') {
					// this character is lowercase, so get the data from the lowercase folder
					sprintf(folder, "lower/%c", argv[i][charID]);
					strncat(characterPath, folder, CHARACTER_PATH_LEN - strlen(characterPath) - 1);
				} else {
					// this character is a symbol, so get the data from the 'other' folder
					sprintf(folder, "other/%s", charToFilename(argv[i][charID]));
					strncat(characterPath, folder, CHARACTER_PATH_LEN - strlen(characterPath) - 1);
				}
			}
		}
	}
	
	
	return 0;
}