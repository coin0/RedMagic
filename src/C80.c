/*
 *      Red Magic 1996 - 2015
 *
 *      C80.c - 80x25 text-mode for initial bootup stage
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "C80.h"
#include "print.h"

static _u16 *video_memory = (_u16 *) 0xB8000;
static _u8 cursor_x = 0;
static _u8 cursor_y = 0;

// Updates the hardware cursor.
static void move_cursor()
{
	// The screen is 80 characters wide...
	_u16 cursorLocation = cursor_y * 80 + cursor_x;
	outb(0x3D4, 14);	// Tell the VGA board we are setting the high cursor byte.
	outb(0x3D5, cursorLocation >> 8);	// Send the high cursor byte.
	outb(0x3D4, 15);	// Tell the VGA board we are setting the low cursor byte.
	outb(0x3D5, cursorLocation);	// Send the low cursor byte.
}

// Scrolls the text on the screen up by one line.
static void scroll()
{

	// Get a space character with the default colour attributes.
	_u8 attributeByte = (0 /*black */  << 4) | (15 /*white */  & 0x0F);
	_u16 blank = 0x20 /* space */  | (attributeByte << 8);

	// Row 25 is the end, this means we need to scroll up
	if (cursor_y >= 25) {
		// Move the current text chunk that makes up the screen
		// back in the buffer by a line
		int i;
		for (i = 0 * 80; i < 24 * 80; i++) {
			video_memory[i] = video_memory[i + 80];
		}

		// The last line should now be blank. Do this by writing
		// 80 spaces to it.
		for (i = 24 * 80; i < 25 * 80; i++) {
			video_memory[i] = blank;
		}
		// The cursor should now be on the last line.
		cursor_y = 24;
	}
}

// Writes a single character out to the screen.
void c80_put(char c)
{
	// The background colour is black (0), the foreground is white (15).
	_u8 backColour = 0;
	_u8 foreColour = 15;

	// The attribute byte is made up of two nibbles - the lower being the
	// foreground colour, and the upper the background colour.
	_u8 attributeByte = (backColour << 4) | (foreColour & 0x0F);
	// The attribute byte is the top 8 bits of the word we have to send to the
	// VGA board.
	_u16 attribute = attributeByte << 8;
	_u16 *location;

	// Handle a backspace, by moving the cursor back one space
	if (c == 0x08 && cursor_x) {
		cursor_x--;
	}
	// Handle a tab by increasing the cursor's X, but only to a point
	// where it is divisible by 8.
	else if (c == 0x09) {
		cursor_x = (cursor_x + 8) & ~(8 - 1);
	}
	// Handle carriage return
	else if (c == '\r') {
		cursor_x = 0;
	}
	// Handle newline by moving cursor back to left and increasing the row
	else if (c == '\n') {
		cursor_x = 0;
		cursor_y++;
	}
	// Handle any other printable character.
	else if (c >= ' ') {
		location = video_memory + (cursor_y * 80 + cursor_x);
		*location = c | attribute;
		cursor_x++;
	}
	// Check if we need to insert a new line because we have reached the end
	// of the screen.
	if (cursor_x >= 80) {
		cursor_x = 0;
		cursor_y++;
	}
	// Scroll the screen if needed.
	scroll();
	// Move the hardware cursor.
	move_cursor();
}

// Clears the screen, by copying lots of spaces to the framebuffer.
void c80_clear()
{
	// Make an attribute byte for the default colours
	_u8 attributeByte = (0 /*black */  << 4) | (15 /*white */  & 0x0F);
	_u16 blank = 0x20 /* space */  | (attributeByte << 8);

	int i;
	for (i = 0; i < 80 * 25; i++) {
		video_memory[i] = blank;
	}

	// Move the hardware cursor back to the start.
	cursor_x = 0;
	cursor_y = 0;
	move_cursor();
}

// Outputs a null-terminated ASCII string to the monitor.
void c80_write(char *c)
{
	int i = 0;
	while (c[i]) {
		c80_put(c[i++]);
	}
}

void c80_putc_color(char c, real_color_t back, real_color_t fore)
{
	_u8 back_color = (_u8) back;
	_u8 fore_color = (_u8) fore;

	_u8 attribute_byte = (back_color << 4) | (fore_color & 0x0F);
	_u16 attribute = attribute_byte << 8;

	if (c == 0x08 && cursor_x) {
		cursor_x--;
	} else if (c == 0x09) {
		cursor_x = (cursor_x + 8) & ~(8 - 1);
	} else if (c == '\r') {
		cursor_x = 0;
	} else if (c == '\n') {
		cursor_x = 0;
		cursor_y++;
	} else if (c >= ' ') {
		video_memory[cursor_y * 80 + cursor_x] = c | attribute;
		cursor_x++;
	}

	if (cursor_x >= 80) {
		cursor_x = 0;
		cursor_y++;
	}

	scroll();
	move_cursor();
}

void c80_write_color(char *cstr, real_color_t back, real_color_t fore)
{
	while (*cstr) {
		c80_putc_color(*cstr++, back, fore);
	}
}

void c80_write_hex(_u32 n)
{
	// TODO
}

void c80_write_dec(_u32 n)
{
	// TODO
}
