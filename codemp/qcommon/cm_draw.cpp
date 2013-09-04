///////////////////////////////////////////////////////////////////////////////
// CDraw32 Class Implementation
//
// Basic drawing routines for 32-bit buffer
///////////////////////////////////////////////////////////////////////////////
//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"


#include "cm_local.h"
#include "cm_draw.h"


///////////// statics for CDraw32 //////////////////////////////////
// Used by all drawing routines as the "current" drawing context
CPixel32* CDraw32::buffer = NULL; 	// pointer to 32-bit deep pixel buffer
long	CDraw32::buf_width=0;		// width of buffer in pixels
long	CDraw32::buf_height=0;		// height of buffer in pixels
long	CDraw32::stride = 0;		// stride in pixels
long	CDraw32::clip_min_x=0;		// clip bounds
long	CDraw32::clip_min_y=0;		// clip bounds
long	CDraw32::clip_max_x=0;		// clip bounds
long	CDraw32::clip_max_y=0;		// clip bounds
long*	CDraw32::row_off = NULL;	// Table for quick Y calculations

CDraw32::CDraw32() 
//USE:	constructor
{
}

CDraw32::~CDraw32()
//USE:	Destructor
{
}

int	imgKernel[5][5] = 
{
	{-1,-1,-1,-1, 0},
	{-1,-1,-1, 0, 1},
	{-1,-1, 0, 1, 1},
	{-1, 0, 1, 1, 1},
	{ 0, 1, 1, 1, 1}
};

const int KWIDTH = 2;

void CDraw32::Emboss(long dstX, long dstY, long width, long height, 
					 CPixel32* clrImage, long clrX, long clrY, long clrStride)
{
	CPixel32 	*dst;
	CPixel32 	*clr;
	int			x,y,i,j;
	int			dstNextLine;
	int			clrNextLine;

	assert(buffer != NULL);

	BlitClip(dstX, dstY, width, height, clrX, clrY);

	if (width < 1 || height < 1)
		return;

	dst = &buffer[PIXPOS(dstX,dstY,stride)];
	clr = &clrImage[PIXPOS(clrX,clrY,clrStride)];

	dstNextLine = (stride - width);
	clrNextLine = (clrStride - width);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int accum = 0;
			for (j = -KWIDTH; j<=KWIDTH; j++)
				for (i = -KWIDTH; i<=KWIDTH; i++)
				{
					int xk = CLAMP(x + i, clrX, clrX+width-1);
					int yk = CLAMP(y + j, clrY, clrY+height-1);
					accum += clrImage[PIXPOS(xk,yk,clrStride)].a * imgKernel[j+KWIDTH][i+KWIDTH];
				}
			*dst = LIGHT_PIX(*clr, accum);
			dst->a = 255;
			++dst;
			++clr;
		}
		dst += dstNextLine;
		clr += clrNextLine;
	}


}

bool CDraw32::SetBufferSize(long width,long height,long stride_len)
//USE:	setup for a particular size drawing buffer
//			(do not re-setup if buffer size has not changed)
//IN:		width,height	- size of buffer
//			stride_len		- distance to next line
//OUT:	true if everything goes OK, otherwise false
{
	long		i;

	assert(width!=0);
	assert(height!=0);
	assert(stride_len!=0);

	if (buf_width != width || buf_height != height ||
		  stride_len != stride)
	{	// need to re-create row_off table
		buf_width  = width;
		buf_height = height;
		stride = stride_len;

		if (row_off)
  			delete [] row_off;

		// row offsets used for quick pixel address calcs
		row_off = new long[height];

		assert(row_off != NULL);
		if (row_off == NULL)
			return false;

		// table for quick pixel lookups
		for (i=0; i<height; i++)
			row_off[i] = i * stride;
	}
	// set default clip bounds
	SetClip(0, 0, width-1, height-1);

	return true;
}

void CDraw32::ClearLines(CPixel32 color,long start,long end)
//USE:	clear screen buffer to color provided for lines
//IN:		color	 				- 32-bit color value
//			start through end		- line numbers
//OUT:	none
{
	CPixel32 	*dest;
	int			line,i,next_line;

	assert(buffer!=NULL);
	assert(start>=0);
	assert(end<buf_height);

	dest = &buffer[row_off[start]];

	next_line = stride - buf_width;
	line = end - start + 1;

	while (line-- != 0)
	{	 // very simple-minded fill loop
		i = buf_width;
		while (i--)
			*dest++ = color;
		dest += next_line;
	}
}

void CDraw32::SetAlphaLines(byte alpha,long start,long end)
//USE:	set the alpha value only
//IN:		alpha	 				- 8-bit alpha value
//			start through end		- line numbers
//OUT:	none
{
	CPixel32 	*dest;
	int			line,i,next_line;

	assert(buffer!=NULL);
	assert(start>=0);
	assert(end<buf_height);

	dest = &buffer[row_off[start]];

	next_line = stride - buf_width;
	line = end - start + 1;

	while (line-- != 0)
	{	 // very simple-minded fill loop
		i = buf_width;
		while (i--)
		{
			dest->a = alpha;
			++dest;
		}
		dest += next_line;
	}
}

#define LEFT   1 // code bits
#define RIGHT  2
#define TOP    4
#define BOTTOM 8

static long code(long x,long y)
//USE:	determines where a point is in relation to a bounding box
//IN:		x,y		- coordinate pair
//OUT:	clipping code compaired to global clip context
{
	long c;

	c = 0;
	if (x < CDraw32::clip_min_x) c |= LEFT;
	if (x > CDraw32::clip_max_x) c |= RIGHT;
	if (y < CDraw32::clip_min_y) c |= BOTTOM;
	if (y > CDraw32::clip_max_y) c |= TOP;

	return c;
}

bool CDraw32::ClipLine(long& x1, long& y1, long& x2, long& y2)
//USE:	clip a line from (x1,y1) to (x2,y2) to clip bounds
//IN:		(x1,y1)-(x2,y2) line
//OUT:	return true if something left to draw, otherwise false
{
	long c1,c2,c,x,y,f;
	x = x1;
	y = y1;

	c1 = code(x1,y1); // find where first pt. is
	c2 = code(x2,y2); // find where second pt. is

	if ((c1 & c2) == 0)
  	{   // the line may be visible
	  	while (c1 | c2)
    	{   // where there is 2D clipping to be done
	    	if (c1 & c2)
			{
				return false;  // if both on same side, quit
			}

			c = c1;
    		if (c==0)
			{
				c = c2; // pick a point
			}

	    	if (c & TOP)
		  	{
      			f = ((clip_max_y-y1) << 15)/(y2-y1);
		      	x = x1 + (((x2-x1)*f + 16384) >> 15);
      			y = clip_max_y;
	      	}
			else if (c & BOTTOM)
      		{
	      		f = ((clip_min_y-y1) << 15)/(y2-y1);
		  		x = x1 + (((x2-x1)*f + 16384) >> 15);
      			y = clip_min_y;
      		}
	      	else if (c & LEFT)
       		{
	     		f = ((clip_min_x-x1) << 15)/(x2-x1);
    	 		y = y1 + (((y2-y1)*f + 16384) >> 15);
       			x = clip_min_x;
       		}
      		else if (c & RIGHT)
   			{
       			f = ((clip_max_x-x1) << 15)/(x2-x1);
   	   			y = y1 + (((y2-y1)*f + 16384) >> 15);
       			x = clip_max_x;
   			}
	    	if (c==c1)
		  	{
				x1=x; y1=y; c1=code(x1,y1);
			}
    		else
	      	{
				x2=x; y2=y; c2=code(x2,y2);
			}
    	} // while still needs clipping
  	}
	else
	{	// line not visible
  		return false;
	}
	return true;
}

void CDraw32::DrawLineNC(long x1, long y1, long x2, long y2, CPixel32 color)
//USE:	draw a line from (x1,y1) to (x2,y2) in color (no clip)
//IN:		(x1,y1)			- starting coordinate
//			(x2,y2)			- ending coordinate
//			color			- 32-bit color value
//OUT:	none
{
	long		d, ax, ay, sx, sy, dx, dy;
	CPixel32*	dest;

	assert(buffer != NULL);

	dx = x2-x1;
	ax = ABS(dx) << 1;
	sx = SIGN(dx);
	dy = y2-y1;
	ay = ABS(dy) << 1;
	sy = SIGN(dy);

	if (255 == color.a)
	{
		if (dy == 0)
		{ // horz line
			if (dx >= 0)
			{
				dest = &buffer[row_off[y1] + x1];
				int i = dx+1;
				while (i--)
					*dest++ = color;
			}
			else
			{
				dest = &buffer[row_off[y1] + x1 + dx];
				int i = -dx+1;
				while (i--)
					*dest++ = color;
			}
			return;
		}

		if (dx == 0)
		{ // vert line
			if (dy >= 0)
			{
				dest = &buffer[row_off[y1] + x1];
				dy++;
			}
			else
			{
				dest = &buffer[row_off[y2] + x1];
				dy = -dy + 1;
			}

			while (dy-- != 0)
			{
				*dest = color;
				dest += stride;
			}
			return;
		}
	}

	// bressenham's algorithm
	if (ax > ay)
	{
		d = ay - (ax >> 1);
		while(x1 != x2)
   		{
			PutPixAlphaNC(x1,y1,color);

	    	if (d >= 0)
    		{
		   		y1 += sy;
    			d  -= ax;
    		}
	    	x1 += sx;
			d  += ay;
   		}
	}
	else
	{
		d = ax - (ay >> 1);
		while(y1 != y2)
    	{
			PutPixAlphaNC(x1,y1,color);
			if (d >= 0)
    		{
    			x1 += sx;
    			d  -= ay;
    		}
	    	y1 += sy;
			d  += ax;
    	}
	}
	PutPixAlphaNC(x1,y1,color);
}

void CDraw32::DrawLineAveNC(long x1, long y1, long x2, long y2, CPixel32 color)
//USE:	draw a translucent line from (x1,y1) to (x2,y2) in color (no clip)
//IN:		(x1,y1)			- starting coordinate
//			(x2,y2)			- ending coordinate
//			color 			- 32-bit color value
//OUT:	none
{
	long		d, ax, ay, sx, sy, dx, dy;
	CPixel32*	dest;

	assert(buffer != NULL);

	dx = x2-x1;
	ax = ABS(dx) << 1;
	sx = SIGN(dx);
	dy = y2-y1;
	ay = ABS(dy) << 1;
	sy = SIGN(dy);

	if (dy == 0)
	{ // horz line
		if (dx >= 0)
		{
			dest = &buffer[row_off[y1] + x1];
			int i = dx+1;
			while (i--)
			{
				*dest = AVE_PIX(*dest, color);
				dest++;
			}
		}
		else
		{
			dest = &buffer[row_off[y1] + x1 + dx];
			int i = -dx+1;
			while (i--)
			{
				*dest = AVE_PIX(*dest, color);
				dest++;
			}
		}
		return;
	}

	if (dx == 0)
	{ // vert line
		if (dy >= 0)
		{
			dest = &buffer[row_off[y1] + x1];
			dy++;
		}
		else
		{
			dest = &buffer[row_off[y2] + x1];
			dy = -dy + 1;
		}

		while (dy-- != 0)
		{
			*dest = AVE_PIX(*dest, color);
			dest += stride;
		}
		return;
	}

	// bressenham's algorithm
	if (ax > ay)
	{
		d = ay - (ax >> 1);
		while(x1 != x2)
   		{
			PutPixAveNC(x1,y1,color);

	    	if (d >= 0)
    		{
		   		y1 += sy;
    			d  -= ax;
    		}
	    	x1 += sx;
			d  += ay;
   		}
	}
	else
	{
		d = ax - (ay >> 1);
		while(y1 != y2)
    	{
			PutPixAveNC(x1,y1,color);
			if (d >= 0)
    		{
    			x1 += sx;
    			d  -= ay;
    		}
	    	y1 += sy;
			d  += ax;
    	}
	}
	PutPixAveNC(x1,y1,color);
}

void CDraw32::DrawLineAANC(long x0, long y0, long x1, long y1, CPixel32 color)
// Wu antialiased line drawer.
//USE:	Function to draw an antialiased line from (x0,y0) to (x1,y1), using an
//			antialiasing approach published by Xiaolin Wu in the July 1991 issue of
//			Computer Graphics (SIGGRAPH proceedings). 
//
//IN:		(x0,y0),(x1,y1) = line to draw
//			color = 32-bit color
//OUT:	none
{
	assert(buffer != NULL);

	// Make sure the line runs top to bottom 
	if (y0 > y1) 
	{
		SWAP(y0,y1);
		SWAP(x0,x1);
	}

	long	DeltaX = x1 - x0;
	long	DeltaY = y1 - y0;
	long	XDir;

	// Draw the initial pixel, which is always exactly intersected by
 	// the line and so needs no Alpha 
	PutPixAlphaNC(x0, y0, color);

	if (DeltaX >= 0) 
	{
 		XDir = 1;
	} 
	else 
	{
 		XDir = -1;
 		DeltaX = -DeltaX; // make DeltaX positive 
	}

	// Special-case horizontal, vertical, and diagonal lines, which
 	//	require no Alpha because they go right through the center of
 	//	every pixel 
	if (DeltaY == 0) 
	{	// Horizontal line 
 		while (DeltaX-- != 0) 
		{
 			x0 += XDir;
			PutPixAlphaNC(x0, y0, color);
		}
 		return;
	}
	if (DeltaX == 0) 
	{	// Vertical line 
 		do 
		{
 			y0++;
			PutPixAlphaNC(x0, y0, color);
		} 
		while (--DeltaY != 0);
 		return;
	}
	if (DeltaX == DeltaY) 
	{	// Diagonal line 
 		do 
		{
 			x0 += XDir;
 			y0++;
			PutPixAlphaNC(x0, y0, color);
 		}
		while (--DeltaY != 0);
 		return;
	}

	// Line is not horizontal, diagonal, or vertical 
	unsigned short ErrorAcc = 0;  // initialize the line error accumulator to 0 

	// # of bits by which to shift ErrorAcc to get intensity level 
	const unsigned long	IntensityShift = 16 - 8;
	
	// Is this an X-major or Y-major line? 
	if (DeltaY > DeltaX) 
	{
 		// Y-major line; calculate 16-bit fixed-point fractional part of a
 		// pixel that X advances each time Y advances 1 pixel, truncating the
 		// result so that we won't overrun the endpoint along the X axis 
		unsigned short	ErrorAdj = (unsigned short)
				(((unsigned long) DeltaX << 16) / (unsigned long) DeltaY);

 		// Draw all pixels other than the first and last 
 		while (--DeltaY) 
		{
 			unsigned short	ErrorAccTemp = ErrorAcc;   // remember currrent accumulated error 
 			ErrorAcc += ErrorAdj;      // calculate error for next pixel 
 			if (ErrorAcc <= ErrorAccTemp) 
			{	// The error accumulator turned over, so advance the X coord 
 				x0 += XDir;
			}
 			y0++; // Y-major, so always advance Y 
 			// The IntensityBits most significant bits of ErrorAcc give us the
 			// intensity Alpha for this pixel, and the complement of the
 			// Alpha for the paired pixel 
 			unsigned long	Alpha = ErrorAcc >> IntensityShift;
			unsigned long	InvAlpha = 256-Alpha;

			PutPixAlphaNC(x0, y0, 
				ALPHA_PIX(GetPix(x0, y0), color, Alpha, InvAlpha));
			PutPixAlphaNC(x0+XDir, y0, 
				ALPHA_PIX(GetPix(x0+XDir, y0), color, InvAlpha, Alpha));
 		}
 		// Draw the final pixel, which is always exactly intersected by the line
 		// and so needs no Alpha 
		PutPixAlphaNC(x1, y1, color);
 		return;
	}
	// It's an X-major line; calculate 16-bit fixed-point fractional part of a
 	// pixel that Y advances each time X advances 1 pixel, truncating the
 	// result to avoid overrunning the endpoint along the X axis 
	unsigned short	ErrorAdj = (unsigned short)
		(((unsigned long) DeltaY << 16) / (unsigned long) DeltaX);
	// Draw all pixels other than the first and last 
	while (--DeltaX) 
	{
 		unsigned short	ErrorAccTemp = ErrorAcc;   // remember currrent accumulated error 
 		ErrorAcc += ErrorAdj;      // calculate error for next pixel 
 		if (ErrorAcc <= ErrorAccTemp) 
		{	// The error accumulator turned over, so advance the Y coord 
 			y0++;
		}
 		x0 += XDir; // X-major, so always advance X 
 		// The IntensityBits most significant bits of ErrorAcc give us the
 		// intensity Alpha for this pixel, and the complement of the
 		// Alpha for the paired pixel 
 		unsigned long	Alpha = ErrorAcc >> IntensityShift;
		unsigned long	InvAlpha = 256-Alpha;
		PutPixAlphaNC(x0, y0, 
			ALPHA_PIX(GetPix(x0, y0), color, Alpha, InvAlpha));
		PutPixAlphaNC(x0, y0+1, 
			ALPHA_PIX(GetPix(x0, y0+1), color, InvAlpha, Alpha));
	}
	// Draw the final pixel, which is always exactly intersected by the line
 	// and so needs no Alpha 
	PutPixAlphaNC(x1, y1, color);
}

void CDraw32::DrawRectNC(long ulx, long uly, long width, long height, CPixel32 color)
//USE:	draw rectangle in solid color, no clipping
//IN:		(ulx,uly)			- coordinates of upper-left corner of rect
//			width, height		- dimensions of rectangle
//			color				- color value
//OUT:	none
{

	assert(buffer != NULL);
	assert(ulx>=0);
	assert(uly>=0);
	assert(ulx+width<buf_width);
	assert(uly+height<buf_height);

	if (height < 1 || width < 1)
		return;

	while (height-- != 0)
	{
		DrawLineNC(ulx, uly, ulx+width-1, uly, color);
		uly++;
	}
}

void CDraw32::DrawRect(long ulx, long uly, long width, long height, CPixel32 color)
//USE:	draw rectangle in solid color
//IN:		(ulx,uly)			- coordinates of upper-left corner of rect
//			width, height		- dimensions of rectangle
//			color				- color value
//OUT:	none
{
	assert(buffer != NULL);

	if (height < 1 || width < 1)
		return;

	while (height-- != 0)
	{
		DrawLine(ulx, uly, ulx+width-1, uly, color);
		uly++;
	}
}

void CDraw32::DrawRectAve(long ulx, long uly, long width, long height, CPixel32 color)
//USE:	draw rectangle in solid color, translucent
//IN:		(ulx,uly)			- coordinates of upper-left corner of rect
//			width, height		- dimensions of rectangle
//			color				- color value
//OUT:	none
{
	assert(buffer != NULL);

	if (height < 1 || width < 1)
		return;

	while (height-- != 0)
	{
		DrawLineAve(ulx, uly, ulx+width-1, uly, color);
		uly++;
	}
}

void CDraw32::DrawBoxNC(long ulx, long uly, long width, long height, CPixel32 color)
//USE:	Draw an empty box, no clipping
//IN:		(ulx,uly)		- coordinates of upper-left corner of box
//			width, height	- dimensions of box
//			color			- color value
//OUT:	none
{
	assert(buffer != NULL);

	if (height < 1 || width < 1)
		return;

	DrawLineNC(ulx, uly, ulx+width-1, uly, color);
	DrawLineNC(ulx, uly+height-1, ulx+width-1, uly+height-1, color);
	DrawLineNC(ulx, uly, ulx, uly+height-1, color);
	DrawLineNC(ulx+width-1, uly, ulx+width-1, uly+height-1, color);
}

void CDraw32::DrawBox(long ulx, long uly, long width, long height, CPixel32 color)
//USE:	Draw an empty box
//IN:		(ulx,uly)		- coordinates of upper-left corner of rect
//			width, height	- dimensions of rectangle
//			color			- color value
//OUT:	none
{
	assert(buffer != NULL);

	if (height < 1 || width < 1)
		return;

	DrawLine(ulx, uly, ulx+width-1, uly, color);
	DrawLine(ulx, uly+height-1, ulx+width-1, uly+height-1, color);
	DrawLine(ulx, uly, ulx, uly+height-1, color);
	DrawLine(ulx+width-1, uly, ulx+width-1, uly+height-1, color);
}

void CDraw32::DrawBoxAve(long ulx, long uly, long width, long height, CPixel32 color)
//USE: Draw an empty box, translucent
//IN:		(ulx,uly)		- coordinates of upper-left corner of rect
//			width, height	- dimensions of rectangle
//			color			- color value
//OUT:	none
{
	assert(buffer!=NULL);

	if (height < 1 || width < 1)
		return;

	DrawLineAve(ulx, uly, ulx+width-1, uly, color);
	DrawLineAve(ulx, uly+height-1, ulx+width-1, uly+height-1, color);
	DrawLineAve(ulx, uly, ulx, uly+height-1, color);
	DrawLineAve(ulx+width-1, uly, ulx+width-1, uly+height-1, color);
}

void CDraw32::DrawCircle(long xc, long yc, long r, CPixel32 edge, CPixel32 fill)
//USE:	Draw a simple circle in current color with Bresenham's
//		circle algorithm.
// See PROCEDURAL ELEMENTS FOR COMPUTER GRAPHICS
// David F. Rogers Pg. 48.
//
//IN:   xc,yc - center
//      r     - radius
//      edge  - edge color 
//      fill  - fill color 
//
//OUT: NONE (a circle drawn in the off-screen buffer)
{
	long		x,y;
	long		limit,di,delta;
	long		last_x,last_y;

	assert(buffer != NULL);

	if (r < 1)
		return;

	// draw fill
	if (fill.a != 0)
	{
		x = 0; last_x = x;
		y = r; last_y = y;
		di = 2*(1-r);
		limit = 0;

		do
    	{
	    	if (y >= limit)
    		{
				if (di < 0)
       			{
	     			delta = 2*di + 2*y - 1;
		 			if (delta <= 0)
       				{ 	// move horizontal
     					last_x = x;
     					x++;
	     				di += (2*x + 1);
		   			}
     			else
       				{ 	// move diagonal
     					last_x = x;
     					x++;
	     				y--;
		 				di += (2*x - 2*y + 2);
       				}
       			}
    			else
				{
        			if (di > 0)
        			{
	        			delta = 2*di - 2*x -1;
		    			if (delta <= 0)
         				{ 	// move diagonal
							last_x = x;
	       					x++;
    	   					y--;
         					di += (2*x - 2*y + 2);
         				}
	        			else
		      			{ 	// move vertical
				  			y--;
          					di += (1 - 2*y);
	          			}
	    			}
		    		else /* di = 0 */
        			{  // move diagonal
			    		last_x = x;
	    	    		x++;
		    			y--;
						di += (2*x - 2*y + 2);
        			}
				}
    		}

	    	if (y != last_y)
    		{ // circle fill
		    	DrawLine(xc-last_x,yc+last_y,xc+last_x,yc+last_y,fill);
				if (last_y > limit)
				{
      				DrawLine(xc-last_x,yc-last_y,xc+last_x,yc-last_y,fill);
				}
	    		last_y = y;
    		}
    	}  while (y >= limit);
	}

	// draw edge
	if (edge.a != 0)
	{
		x = 0;
		y = r;
		limit = 0;
		di = 2*(1-r);

		do
   		{
	    	// circle edge
			PutPix(xc+x, yc+y, edge);
    		PutPix(xc-x, yc+y, edge);
	    	if (y > limit)
    		{
		   		PutPix(xc+x, yc-y, edge);
    			PutPix(xc-x, yc-y, edge);
    		}

	    	if (y >= limit)
    		{
    			if (di < 0)
     			{
	     			delta = 2*di + 2*y - 1;
		 			if (delta <= 0)
       				{ 	// move horizontal
     					x++;
     					di += (2*x + 1);
	       			}
		 			else
       				{ 	// move diagonal
     					x++;
	     				y--;
		       			di += (2*x - 2*y + 2);
       				}
       			}
	    		else
				{
		      		if (di > 0)
       				{
      					delta = 2*di - 2*x -1;
		      			if (delta <= 0)
			 			{ // move diagonal
       						x++;
       						y--;
		       				di += (2*x - 2*y + 2);
			 			}
      					else
	        			{ // move vertical
			   				y--;
       						di += (1 - 2*y);
         				}
	       			}
			  		else /* di = 0 */
	       			{  	// move diagonal
			  			x++;
      					y--;
		      			di += (2*x - 2*y + 2);
	     			}
				}
    		}
    	}  while (y >= limit);
	}
} // DrawCircle

void CDraw32::DrawCircleAve(long xc, long yc, long r, CPixel32 edge, CPixel32 fill)
//USE:	Draw a simple circle in current color with Bresenham's
//			circle algorithm.
//			a circle with fill and edge colors averaged with dest (-1 = no color)
// See PROCEDURAL ELEMENTS FOR COMPUTER GRAPHICS
// David F. Rogers Pg. 48.
//
//IN:   xc,yc - center
//      r     - radius
//      edge  - edge color 
//      fill  - fill color 
//
//OUT:	none (a circle on in the off-screen buffer)
{
	long		x,y;
	long		limit,di,delta;
	long		last_x,last_y;
	long		f;

	assert(buffer != NULL);

	if (r < 1)
		return;

	// draw fill
	if (fill.a != 0)
	{
		x = 0; last_x = x;
		y = r; last_y = y;
		di = 2*(1-r);
		limit = 0;

		do
    	{
	    	if (y >= limit)
    		{
				if (di < 0)
       			{
     				delta = 2*di + 2*y - 1;
	     			if (delta <= 0)
		   			{ 	// move horizontal
     					last_x = x;
     					x++;
     					di += (2*x + 1);
	       			}
		 			else
       				{ 	// move diagonal
	     				last_x = x;
		 				x++;
     					y--;
     					di += (2*x - 2*y + 2);
       				}
	       		}
				else
				{
					if (di > 0)
	        		{
			    		delta = 2*di - 2*x -1;
		        		if (delta <= 0)
			 			{ 	// move diagonal
		        			last_x = x;
				   			x++;
		    	   			y--;
				 			di += (2*x - 2*y + 2);
         				}
		        		else
			      		{ 	// move vertical
			          		y--;
			          		di += (1 - 2*y);
          				}
	        		}
			    	else /* di = 0 */
	        		{  // move diagonal
			        	last_x = x;
		    	    	x++;
						y--;
		        		di += (2*x - 2*y + 2);
					}
				}
    		}

	    	if (y != last_y)
    		{ // circle fill
				for (f=xc-last_x; f<=xc+last_x; f++)
					PutPixAve(f, yc+last_y, fill);
	    		if (last_y > limit)
				{
					for (f=xc-last_x; f<=xc+last_x; f++)
						PutPixAve(f, yc-last_y, fill);
				}
	    		last_y = y;
    		}
    	}  while (y >= limit);
	}

	// draw edge
	if (edge.a != 0)
	{
		x = 0;
		y = r;
		limit = 0;
		di = 2*(1-r);

		do
   		{
	    	// circle edge
			PutPixAve(xc+x, yc+y, edge);
	    	PutPixAve(xc-x, yc+y, edge);
			if (y > limit)
    		{
	   			PutPixAve(xc+x, yc-y, edge);
	    		PutPixAve(xc-x, yc-y, edge);
    		}
			if (y >= limit)
    		{
    			if (di < 0)
     			{
     				delta = 2*di + 2*y - 1;
	     			if (delta <= 0)
	       			{ 	// move horizontal
		 				x++;
     					di += (2*x + 1);
       				}
     				else
	       			{ 	// move diagonal
		 				x++;
			 			y--;
	       				di += (2*x - 2*y + 2);
		   			}
       			}
    			else
				{
		      		if (di > 0)
		   			{
	      				delta = 2*di - 2*x -1;
			  			if (delta <= 0)
	         			{ // move diagonal
			   				x++;
       						y--;
		       				di += (2*x - 2*y + 2);
			 			}
      					else
	        			{ // move vertical
			   				y--;
       						di += (1 - 2*y);
	         			}
		   			}
		      		else /* di = 0 */
       				{  	// move diagonal
		      			x++;
      					y--;
		      			di += (2*x - 2*y + 2);
	     			}
				}
    		}
    	}  while (y >= limit);
	}
} // DrawCircleAve

////////////////////////////////////////////////////////////////////////////
//Concave Polygon Scan Conversion
////////////////////////////////////////////////////////////////////////////

// concave: scan convert nvert-sided concave non-simple polygon
// with vertices at (point[i].x, point[i].y) for i in
// [0..nvert-1] within the window win by
// calling spanproc for each visible span of pixels.
//
// Polygon can be clockwise or counterclockwise.
//
// Algorithm does uniform point sampling at pixel centers.
// Inside-outside test done by even-odd rule: a point is
// considered inside if an emanating ray intersects the polygon
// an odd number of times.
//
// spanproc should fill in pixels from xl to xr inclusive on scanline y,
//
// e.g:
//	spanproc(short y, short xl, short xr)
//	{
//	    short x;
//	    for (x=xl; x<=xr; x++)
//			pixel_write(x, y, pixelvalue);
//	}

typedef struct POLYEDGE_s
{	   	        // a polygon edge
             	// these are fixed point long ints for some accuracy & speed
	long   x;		// x coordinate of edge's intersection with current scanline
	long   dx;	// change in x with respect to y
	long   i;		// edge number: edge i goes from pt[i] to
       				// pt[i+1]
} POLYEDGE;

#define INT_SHIFT 13

// global for speed
static long   	 n;				// number of vertices
static Point  	*pt;		  	// vertices

static long	    nact;	  		// number of active edges
static POLYEDGE active[256];	// active edge list:edges crossing scanline y
static long     ind[256];		// list of vertex indices, sorted by
	    		                // pt[ind[j]].y

static void del_edge(long i)
// remove edge i from active list
{
	int  j;

	for (j = 0; j < nact && active[j].i != i; j++)
		;

	// edge not in active list; happens at cliprect->top
	if (j >= nact)
	{
		return;
	}

	nact--;
	memcpy(&active[j], &active[j + 1], (nact - j) * sizeof(active[0]));
}

static void ins_edge(long i, long y)
// append edge i to end of active list
{
	int 		j;
	long    dx;
	Point   *p;
	Point   *q;

	j = i < n - 1 ? i + 1 : 0;
	if (pt[i].y < pt[j].y)
	{
		p = &pt[i];
		q = &pt[j];
	}
	else
	{
		p = &pt[j];
		q = &pt[i];
	}

	// initialize x position at intersection of edge with scanline y
	if ((q->y - p->y) != 0)
	{
		dx =  (((long)q->x - (long)p->x) * (long)(1<<INT_SHIFT));
		dx /= (((long)q->y - (long)p->y));
	}
	else
	{
		// horizontal line
		dx = 0;
	}

	active[nact].dx = dx;
	active[nact].x = (dx * (long)(y - p->y)) + ((long)p->x << INT_SHIFT);
	active[nact].i = i;
	nact++;
}

// comparison routines for shellsort
int compare_ind(long *u, long *v)
{
	return (pt[*u].y <= pt[*v].y ? -1 : 1);
}

int compare_active(POLYEDGE *u, POLYEDGE *v)
{
	return (u->x <= v->x ? -1 : 1);
}

void shell_sort(void *vec, long n, long siz,
                int (*compare)(void*,void*))
// USE:  shell sort aka heap sort. Best sort algorithm for almost sorted list.
{
	byte *a;
	byte v[128]; // temp object
	long 		i,j,h;

	a = (byte *)vec;

	// choose size of "heap"
	for (h = 1; h <= n/9; h = 3*h+1)
		;

	// divide and conq.
	for ( ; h > 0; h /= 3)
	{
		for (i = h; i < n; i++)
   		{
	    	// v = a[i];
			memcpy(v,(a+i*siz),siz);
	    	j = i;
			// j >= h && a[j-h] > v
    		while ((j >= h) && compare((void*)(a+(j-h)*siz),(void*)v) > 0)
    		{
	    		// a[j] = a[j-h]
				memcpy((a+j*siz),(a+(j-h)*siz),siz);
    			j -= h;
    		}
	    	// a[j] = v;
			memcpy((a+j*siz),v,siz);
   		}
	}
}

void CDraw32::DrawPolygon(long nvert, Point *point, CPixel32 edge, CPixel32 fill)
//USE:    Scan convert a polygon
//IN:     nvert:        Number of vertices
//        point:        Vertices of polygon
//        edge:         edge color
//        fill:         fill color
//OUT:		none
{
	long	k,
	        y0,
	        y1,
	        y,
	        i,
	        j,
	        xl,
	        xr;

	assert(buffer != NULL);

	n = nvert;

	if (n <= 0)
	{	// nothing to do
		return;
	}

	pt = point;

	if (fill.a != 0)
	{ // draw fill

		// create y-sorted array of indices ind[k] into vertex list
		for (k = 0; k < n; k++)
		{
			ind[k] = k;
		}

		// sort ind by pt[ind[k]].y
		shell_sort(ind, n, sizeof(long), (int (*)(void*,void*)) compare_ind);

		nact = 0;					   // start with empty active list
		k = 0;						   // ind[k] is next vertex to process

		// ymin of polygon
		y0 = MAX(clip_min_y-1, pt[ind[0]].y);

		// ymax of polygon
		y1 = MIN(clip_max_y+1, pt[ind[n-1]].y);

		// step through scanlines
		for (y = y0; y < y1; y++)
    	{
			// Check vertices between previous scanline
			// and current one, if any
			for (; (k < n) && (pt[ind[k]].y <= y); k++)
    		{
				i = ind[k];
				//  insert or delete edges before and after vertex i
	    		//  (i-1 to i, and i to i+1)
				//  from active list if they cross scanline y
				j = i > 0 ? i - 1 : n - 1;	// vertex previous to i
				if (pt[j].y < y)
				{
					// old edge, remove from active list
					del_edge(j);
				}
				else
				{
					if (pt[j].y > y)
					{
						// new edge, add to active list
						ins_edge(j, y);
					}
		 		}
				j = i < n - 1 ? i + 1 : 0;	// vertex next after i
				if (pt[j].y < y)
				{
					// old edge, remove from active list
					del_edge(i);
				}
				else
				{
					if (pt[j].y > y)
					{
						// new edge, add to active list
						ins_edge(i, y);
					}
				}
   			}

			// sort active edge list by active[j].x
			shell_sort(active, nact, sizeof(POLYEDGE), (int (*)(void*,void*))compare_active);

			// draw horizontal segments for scanline y
			for (j = 0; j < nact; j += 2)
    		{	// draw horizontal segments
				// span 'tween j & j+1 is inside, span tween
				// j+1 & j+2 is outside

				// left end of span
	    		// convert back from fixed point - round down
				xl = (long) (active[j].x >> INT_SHIFT);
  				if (xl < clip_min_x-1)
				{
  					xl = clip_min_x-1;
			 	}

	    		// right end of span
				// convert back from fixed point - round down
				xr = (long) (active[j + 1].x >> INT_SHIFT);
  				if (xr > clip_max_x)
				{
  					xr = clip_max_x;
				}

				if (xl < xr)
	        	{
  					// draw pixels in span
		        	DrawLine(xl+1,y,xr,y,fill);
	        	}

				// increment edge coords
				active[j].x += active[j].dx;
				active[j + 1].x += active[j + 1].dx;
				}
  			}

			// sort active edge list by active[j].x
			shell_sort(active, nact, sizeof(POLYEDGE), (int (*)(void*,void*))compare_active);

			// draw horizontal segments for scanline y
			for (j = 0; j < nact; j += 2)
    		{	// draw horizontal segments
				// span 'tween j & j+1 is inside, span tween
				// j+1 & j+2 is outside

				// left end of span
    			// convert back from fixed point - round down
		    	xl = (long) (active[j].x >> INT_SHIFT);
		  		if (xl < clip_min_x-1)
				{
  					xl = clip_min_x-1;
				}

		    	// right end of span
		    	// convert back from fixed point - round up
				xr = (long) (active[j + 1].x >> INT_SHIFT);
		  		if (xr > clip_max_x)
				{
  					xr = clip_max_x;
				}

				if (xl < xr)
				{
  					// draw pixels in span
		    		DrawLine(xl+1,y,xr,y,fill);
    			}

				// increment edge coords
				active[j].x += active[j].dx;
				active[j + 1].x += active[j + 1].dx;
			}
		}

		if (edge.a != 0)
		{ // draw edges
			for (k = 0; k < n-1; k++)
			{
    			DrawLineAA(pt[k].x,pt[k].y,pt[k+1].x,pt[k+1].y,edge);
			}

			DrawLineAA(pt[n-1].x,pt[n-1].y,pt[0].x,pt[0].y,edge);
		}

	return;
}

void CDraw32::Blit(long dstX, long dstY, long width, long height, 
				   CPixel32* srcImage, long srcX, long srcY, long srcStride)
//USE:	simple blit
//IN:	dstX, dstY				- upper left corner of where image will land in buffer
//		width, height			- width and height of image
//		srcImage				- src image buffer
//		srcX, srcY				- upper left corner in src image
//		srcStride				- number of pixels per line in src image
//OUT:	none
{
	CPixel32 	*dst;
	CPixel32 	*src;
	int			x,y;

	assert(buffer != NULL);

	BlitClip(dstX, dstY, width, height, srcX, srcY);

	if (width < 1 || height < 1)
		return;

	dst = &buffer[PIXPOS(dstX,dstY,stride)];
	src = &srcImage[PIXPOS(srcX,srcY,srcStride)];

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
//			*dst++ = *src++;
			byte alpha = src->a;
			byte dst_alpha = dst->a;
			*dst = ALPHA_PIX(*src, *dst, alpha, 256-alpha);
			dst->a = dst_alpha;
			++dst;
			++src;
		}
		dst += (stride - width);
		src += (srcStride - width);
	}
}

void CDraw32::BlitClip(long& dstX, long& dstY, 
					   long& width, long& height, 
					   long& srcX, long& srcY)
//USE:	simple blit clip
//IN:	dstX, dstY				- upper left corner of where image will land in buffer
//		width, height			- width and height of image
//		srcX, srcY				- upper left corner in src image
//OUT:	none
{

	// clip to our buffer size
	if (dstX < clip_min_x)
	{
		int dif = (clip_min_x - dstX);
		dstX += dif;
		srcX += dif;
		width -= dif;
	}

	if (dstY < clip_min_y)
	{
		int dif = (clip_min_y - dstY);
		dstY += dif;
		srcY += dif;
		height -= dif;
	}

	if (dstX+width-1 > clip_max_x)
	{
		width -= (dstX+width-1 - clip_max_x);
	}

	if (dstY+height-1 > clip_max_y)
	{
		height -= (dstY+height-1 - clip_max_y);
	}
}

void CDraw32::BlitColor(long dstX, long dstY, long width, long height, 
					    CPixel32* srcImage, long srcX, long srcY, long srcStride, CPixel32 color)
//USE:	blit using image alpha as mask
//IN:	dstX, dstY				- upper left corner of where image will land in buffer
//		width, height			- width and height of image
//		srcImage				- src image buffer
//		srcX, srcY				- upper left corner in src image
//		srcStride				- number of pixels per line in src image
//		color					- color to apply to srcImage
//OUT:	none
{
	CPixel32 	*dst;
	CPixel32 	*src;
	int			x,y;
	int			dstNextLine;
	int			srcNextLine;

	assert(buffer != NULL);

	BlitClip(dstX, dstY, width, height, srcX, srcY);

	if (width < 1 || height < 1)
		return;

	dst = &buffer[PIXPOS(dstX,dstY,stride)];
	src = &srcImage[PIXPOS(srcX,srcY,srcStride)];

	dstNextLine = (stride - width);
	srcNextLine = (srcStride - width);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			byte alpha = src->a;
			*dst = ALPHA_PIX(color, *dst, alpha, 256-alpha);
			++dst;
			++src;
		}
		dst += dstNextLine;
		src += srcNextLine;
	}
}

