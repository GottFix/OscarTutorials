#include <c64/memmap.h>
#include <c64/vic.h>
#include <gfx/bitmap.h>
#include <math.h>
#include <fixmath.h>
#include <string.h>

#pragma region( main, 0x0880, 0x8c00, , , {code, data, bss, heap, stack } )

// Display buffer
char * const Colors[] = {(char *)0xd000, (char *)0x8c00};
char * const Hires[] = {(char *)0xe000, (char *)0xa000};

// Bitmap for display screen
Bitmap		Screen[2];

// Init display
void init(void)
{
	// Prepare IRQ trampoline, so we can use the RAM under the kernal
	mmap_trampoline();

	// Switch to RAM to init bitmap and screen
	mmap_set(MMAP_RAM);

	memset(Colors[0], 0x01, 1000);
	memset(Hires[0], 0x00, 8000);
	memset(Colors[1], 0x01, 1000);
	memset(Hires[1], 0x00, 8000);

	mmap_set(MMAP_NO_ROM);

	// Set hires bitmap mode
	vic_setmode(VICM_HIRES, Colors[1], Hires[1]);

	vic.color_border = VCOL_WHITE;

	bm_init(&Screen[0], Hires[0], 40, 25);	
	bm_init(&Screen[1], Hires[1], 40, 25);	
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
void drawCorners(Bitmap * screen, const Point * c, int n)
{
	char j = n - 1;
	for(char i=0; i<n; i++)
	{
		bm_line(screen, &cr, c[j].x, c[j].y, c[i].x, c[i].y, 0xff, LINOP_OR);
		j = i;
	}
}

void clearCorners(Bitmap * screen, const Point * c, int n)
{
	char j = n - 1;
	for(char i=0; i<n; i++)
	{
		bm_line(screen, &cr, c[j].x, c[j].y, c[i].x, c[i].y, 0xff, LINOP_AND);
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

	// Five star uses 10 vertices, we need two of them for the delay
	Point	c[2][10];
	bool	clear = false;

	for(;;)
	{
		for(int i=0; i<256; i++)
		{
			if (clear)
			{
				// Clear old vertices
				clearCorners(Screen + (i & 1), c[i & 1], 10);
			}

			// Calculate vertices
			calcStar(c[i & 1], 10, i, i);

			// Draw new vertices
			drawCorners(Screen + (i & 1), c[i & 1], 10);

			vic_waitBottom();
			vic_setmode(VICM_HIRES, Colors[i & 1], Hires[i & 1]);

			clear = true;
		}
	}
}
