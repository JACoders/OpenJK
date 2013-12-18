// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "tr_common.h"

void R_InvertImage(byte *data, int width, int height, int depth)
{
	byte	*newData;
	byte	*oldData;
	byte	*saveData;
	int		y, stride;

	stride = width * depth;

	oldData = data + ((height - 1) * stride);
	newData = (byte *)ri.Z_Malloc(height * stride, TAG_TEMP_WORKSPACE, qfalse, 4 );
	saveData = newData;

	for(y = 0; y < height; y++)
	{
		memcpy(newData, oldData, stride);
		newData += stride;
		oldData -= stride;
	}
	memcpy(data, saveData, height * stride);
	ri.Z_Free(saveData);
}

// Lanczos3 image resampling. Better than bicubic, based on sin(x)/x algorithm

#define	LANCZOS3	(3.0f)
#define M_PI_OVER_3	(M_PI / 3.0f)

typedef struct contrib_s {
	int		pixel;
	float	weight;
} contrib_t;

typedef struct contrib_list_s {
	int			n;		// number of contributors
	contrib_t	*p;		// pointer to list of contributions
} contrib_list_t;

// sin(x)/x * sin(x/3)/(x/3)

float Lanczos3(float t)
{
	if(!t)
	{
		return(1.0f);
	}
	t = (float)fabs(t);
	if(t < 3.0f)
	{
		return(sinf(t * M_PI) * sinf(t * M_PI_OVER_3) / (t * M_PI * t * M_PI_OVER_3));
	}
	return(0.0f);
}

void R_Resample(byte *source, int swidth, int sheight, byte *dest, int dwidth, int dheight, int components)
{
	int				i, j, k, l, count, left, right, num;
	int				pixel;
	byte			*raster;
	float			center, weight, scale, width, height;
	contrib_list_t	*contributors;

	byte *work = (byte *)ri.Z_Malloc(dwidth * sheight * components, TAG_TEMP_WORKSPACE, qfalse, 4);

	// Pre calculate filter contributions for rows
	contributors = (contrib_list_t *)ri.Z_Malloc(sizeof(contrib_list_t) * dwidth, TAG_TEMP_WORKSPACE, qfalse, 4);

	float xscale = (float)dwidth / (float)swidth;

	if(xscale < 1.0f)
	{
		width = ceilf(LANCZOS3 / xscale);
		scale = xscale;
	}
	else
	{
		width = LANCZOS3;
		scale = 1.0f;
	}
	num = ((int)width * 2) + 1;

	for(i = 0; i < dwidth; i++)
	{
		contributors[i].n = 0;
		contributors[i].p = (contrib_t *)ri.Z_Malloc(num * sizeof(contrib_t), TAG_TEMP_WORKSPACE, qfalse, 4);

		center = (float)i / xscale;
		left = (int)ceilf(center - width);
		right = (int)floorf(center + width);

		for(j = left; j <= right; j++)
		{
			weight = Lanczos3((center - (float)j) * scale) * scale;
			if(j < 0)
			{
				pixel = -j;
			}
			else if(j >= swidth)
			{
				pixel = (swidth - j) + swidth - 1;
			}
			else
			{
				pixel = j;
			}
			count = contributors[i].n++;
			contributors[i].p[count].pixel = pixel;
			contributors[i].p[count].weight = weight;
		}
	}
	// Apply filters to zoom horizontally from source to work
	for(k = 0; k < sheight; k++)
	{
		raster = source + (k * swidth * components);
		for(i = 0; i < dwidth; i++)
		{
			for(l = 0; l < components; l++)
			{
				weight = 0.0f;
				for(j = 0; j < contributors[i].n; j++)
				{
					weight += raster[(contributors[i].p[j].pixel * components) + l] * contributors[i].p[j].weight;
				}
				pixel = (byte)Com_Clamp(0.0f, 255.0f, weight);
				work[(k * dwidth * components) + (i * components) + l] = pixel;
			}
		}
	}
	// Clean up
	for(i = 0; i < dwidth; i++)
	{
		ri.Z_Free(contributors[i].p);
	}
	ri.Z_Free(contributors);

	// Columns
	contributors = (contrib_list_t *)ri.Z_Malloc(sizeof(contrib_list_t) * dheight, TAG_TEMP_WORKSPACE, qfalse, 4);

	float yscale = (float)dheight / (float)sheight;
	if(yscale < 1.0f)
	{
		height = ceilf(LANCZOS3 / yscale);
		scale = yscale;
	}
	else
	{
		height = LANCZOS3;
		scale = 1.0f;
	}
	num = ((int)height * 2) + 1;

	for(i = 0; i < dheight; i++)
	{
		contributors[i].n = 0;
		contributors[i].p = (contrib_t *)ri.Z_Malloc(num * sizeof(contrib_t), TAG_TEMP_WORKSPACE, qfalse, 4);

		center = (float)i / yscale;
		left = (int)ceilf(center - height);
		right = (int)floorf(center + height);

		for(j = left; j <= right; j++)
		{
			weight = Lanczos3((center - (float)j) * scale) * scale;
			if(j < 0)
			{
				pixel = -j;
			}
			else if(j >= sheight)
			{
				pixel = (sheight - j) + sheight - 1;
			}
			else
			{
				pixel = j;
			}
			count = contributors[i].n++;
			contributors[i].p[count].pixel = pixel;
			contributors[i].p[count].weight = weight;
		}
	}
	// Apply filter to columns
	for(k = 0; k < dwidth; k++)
	{
		for(l = 0; l < components; l++)
		{
			for(i = 0; i < dheight; i++)
			{
				weight = 0.0f;
				for(j = 0; j < contributors[i].n; j++)
				{
					weight += work[(contributors[i].p[j].pixel * dwidth * components) + (k * components) + l] * contributors[i].p[j].weight;
				}
				pixel = (byte)Com_Clamp(0.0f, 255.0f, weight);
				dest[(i * dwidth * components) + (k * components) + l] = pixel;
			}
		}
	}
	// Clean up
	for(i = 0; i < dheight; i++)
	{
		ri.Z_Free(contributors[i].p);
	}
	ri.Z_Free(contributors);
	ri.Z_Free(work);
}

