# Pixel Text BMP generator
This program generates pixelated text from inputted text. Made for use in a terminal.

Usage:
<code>generator c|C fg_color bg_color text</code>

Parameters:
* 'c' means that the output will be in Title Case.
* 'C' means that the output will be in ALL CAPS
* 'fg_color' is hex 32-bit color (8 each including alpha)
* 'bg_color' is hex 32-bit color as well.
* 'text' is the text that is to be outputted. Underscores are outputted as spaces.

### Example
<code>generator c FFFFFFFF 202020FF this_is_an_example</code>

Will output white text on a dark grey background in this format: This Is An Example
