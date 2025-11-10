#include <c64/memmap.h>
#include <c64/vic.h>
#include <gfx/bitmap.h>
#include <math.h>
#include <fixmath.h>
#include <string.h>

// Display buffer
char * const Color = (char *)0xd000;
char * const Hires = (char *)0xe000;

// Bitmap for display screen
Bitmap		Screen;

// Init display
void init(void)
{
	// Prepare IRQ trampoline, so we can use the RAM under the kernal
	mmap_trampoline();

	// Switch to RAM to init bitmap and screen
	mmap_set(MMAP_RAM);

	memset(Color, 0x01, 1000);
	memset(Hires, 0x00, 8000);

	mmap_set(MMAP_NO_ROM);

	// Set hires bitmap mode
	vic_setmode(VICM_HIRES, Color, Hires);

	vic.color_border = VCOL_WHITE;

	bm_init(&Screen, Hires, 40, 25);	
}

// Clipping rectangle for screen
ClipRect	cr = {0, 0, 320, 200};

// A single point in 2D space
struct Point
{
	int x, y;	
};

// Precomputed sine table for 8.8bit precision
__striped static const int SinTab[256] = {
	#for(i, 256) int(sin(i * PI / 128) * 256),
};

// Draw corners of a polygon
void drawCorners(const Point * c, int n)
{
	char j = n - 1;
	for(char i=0; i<n; i++)
	{
		bm_line(&Screen, &cr, c[j].x, c[j].y, c[i].x, c[i].y, 0xff, LINOP_XOR);
		j = i;
	}
}

// Calc border corners of a star
void calcStar(Point * c, int n, char o, char s)
{
	for(char i=0; i<n; i++)
	{
		// Scale sine and cosine by s
		int x = lmul8f8s(SinTab[o], s);
		int y = lmul8f8s(SinTab[(o + 64) & 255], s);

		// Check for smaller corners of star
		if (i & 1)
		{
			x >>= 1;
			y >>= 1;
		}

		// Center on screen
		c[i].x = x + 160;
		c[i].y = y + 100;

		// Next angle
		o = (o + 26) & 255;
	}
}

int main(void)
{
	init();

	// Five star uses 10 vertices
	Point	c[10];
	for(;;)
	{
		for(int i=0; i<256; i++)
		{
			// Calculate vertices
			calcStar(c, 10, i, i);

			// Draw new vertices
			drawCorners(c, 10);

			// Wait a frame
			vic_waitFrame();

			// Clear vertices
			drawCorners(c, 10);		
		}
	}
}
