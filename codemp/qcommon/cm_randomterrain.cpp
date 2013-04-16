//Anything above this #include will be ignored by the compiler
#define NOGDI
#include "qcommon/exe_headers.h"

#include "cm_local.h"
#include "cm_patch.h"
#include "cm_landscape.h"
#include "qcommon/GenericParser2.h"
#include "cm_randomterrain.h"


#define NOISE_SIZE			256
#define NOISE_MASK			(NOISE_SIZE - 1)

static float					noiseTable[NOISE_SIZE];
static int						noisePerm[NOISE_SIZE];

#if 0
static void CM_NoiseInit( CCMLandScape *landscape )
{
	int		i;

	for ( i = 0; i < NOISE_SIZE; i++ )
	{
		noiseTable[i] = landscape->flrand(-1.0f, 1.0f);
		noisePerm[i] = (byte)landscape->irand(0, 255);
	}
}
#endif

#define VAL( a ) noisePerm[ ( a ) & ( NOISE_MASK )]
#define INDEX( x, y, z, t ) VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )

#define LERP( a, b, w ) ( a * ( 1.0f - w ) + b * w )

static float GetNoiseValue( int x, int y, int z, int t )
{
	int index = INDEX( ( int ) x, ( int ) y, ( int ) z, ( int ) t );

	return noiseTable[index];
}

#if 0
static float GetNoiseTime( int t )
{
	int index = VAL( t );

	return (1 + noiseTable[index]);
}
#endif

static float CM_NoiseGet4f( float x, float y, float z, float t )
{
	int i;
	int ix, iy, iz, it;
	float fx, fy, fz, ft;
	float front[4];
	float back[4];
	float fvalue, bvalue, value[2], finalvalue;

	ix = ( int ) floor( x );
	fx = x - ix;
	iy = ( int ) floor( y );
	fy = y - iy;
	iz = ( int ) floor( z );
	fz = z - iz;
	it = ( int ) floor( t );
	ft = t - it;

	for ( i = 0; i < 2; i++ )
	{
		front[0] = GetNoiseValue( ix, iy, iz, it + i );
		front[1] = GetNoiseValue( ix+1, iy, iz, it + i );
		front[2] = GetNoiseValue( ix, iy+1, iz, it + i );
		front[3] = GetNoiseValue( ix+1, iy+1, iz, it + i );

		back[0] = GetNoiseValue( ix, iy, iz + 1, it + i );
		back[1] = GetNoiseValue( ix+1, iy, iz + 1, it + i );
		back[2] = GetNoiseValue( ix, iy+1, iz + 1, it + i );
		back[3] = GetNoiseValue( ix+1, iy+1, iz + 1, it + i );

		fvalue = LERP( LERP( front[0], front[1], fx ), LERP( front[2], front[3], fx ), fy );
		bvalue = LERP( LERP( back[0], back[1], fx ), LERP( back[2], back[3], fx ), fy );

		value[i] = LERP( fvalue, bvalue, fz );
	}

	finalvalue = LERP( value[0], value[1], ft );
	return finalvalue;
}








/****** lincrv.c ******/
/* Ken Shoemake, 1994 */

/* Perform a generic vector unary operation. */
#define V_Op(vdst,gets,vsrc,n) {register int V_i;\
    for(V_i=(n)-1;V_i>=0;V_i--) (vdst)[V_i] gets ((vsrc)[V_i]);}

static void lerp(float t, float a0, float a1, vec4_t p0, vec4_t p1, int m, vec4_t p)
{
    register float t0=(a1-t)/(a1-a0), t1=1-t0;
    register int i;
    for (i=m-1; i>=0; i--) p[i] = t0*p0[i] + t1*p1[i];
}

/* DialASpline(t,a,p,m,n,work,Cn,interp,val) computes a point val at parameter
    t on a spline with knot values a and control points p. The curve will have
    Cn continuity, and if interp is TRUE it will interpolate the control points.
    Possibilities include Langrange interpolants, Bezier curves, Catmull-Rom
    interpolating splines, and B-spline curves. Points have m coordinates, and
    n+1 of them are provided. The work array must have room for n+1 points.
 */
static int DialASpline(float t, float a[], vec4_t p[], int m, int n, vec4_t work[],
                    unsigned int Cn, bool interp, vec4_t val)
{
    register int i, j, k, h, lo, hi;

    if (Cn>n-1) Cn = n-1;       /* Anything greater gives one polynomial */
    for (k=0; t> a[k]; k++);    /* Find enclosing knot interval */
    for (h=k; t==a[k]; k++);    /* May want to use fewer legs */
    if (k>n) {k = n; if (h>k) h = k;}
    h = 1+Cn - (k-h); k--;
    lo = k-Cn; hi = k+1+Cn;

    if (interp) {               /* Lagrange interpolation steps */
        int drop=0;
        if (lo<0) {lo = 0; drop += Cn-k;
                   if (hi-lo<Cn) {drop += Cn-hi; hi = Cn;}}
        if (hi>n) {hi = n; drop += k+1+Cn-n;
                   if (hi-lo<Cn) {drop += lo-(n-Cn); lo = n-Cn;}}
        for (i=lo; i<=hi; i++) V_Op(work[i],=,p[i],m);
        for (j=1; j<=Cn; j++) {
            for (i=lo; i<=hi-j; i++) {
                lerp(t,a[i],a[i+j],work[i],work[i+1],m,work[i]);
            }
        }
        h = 1+Cn-drop;
    } else {                    /* Prepare for B-spline steps */
        if (lo<0) {h += lo; lo = 0;}
        for (i=lo; i<=lo+h; i++) V_Op(work[i],=,p[i],m);
        if (h<0) h = 0;
    }
    for (j=0; j<h; j++) {
        int tmp = 1+Cn-j;
        for (i=h-1; i>=j; i--) {
            lerp(t,a[lo+i],a[lo+i+tmp],work[lo+i],work[lo+i+1],m,work[lo+i+1]);
        }
    }
    V_Op(val,=,work[lo+h],m);
    return (k);
}

#define BIG (1.0e12)

static vec_t Vector2Normalize( vec2_t v ) 
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1];
	length = sqrt (length);

	if ( length ) 
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
	}
		
	return length;
}

CPathInfo::CPathInfo(CCMLandScape *landscape, int numPoints, float bx, float by, float ex, float ey, 
					 float minWidth, float maxWidth, float depth, float deviation, float breadth,
					 CPathInfo *Connected, unsigned CreationFlags) :
	mNumPoints(numPoints),
	mMinWidth(minWidth),
	mMaxWidth(maxWidth),
	mDepth(depth),
	mDeviation(deviation),
	mBreadth(breadth)
{
	int		i, numConnected, index;
	float	position, goal, deltaGoal;
//	float	random, delta;
	bool	horizontal;
	float	*point;
	float	currentWidth;
	float	currentPosition;
	vec2_t	testPoint, percPoint, diffPoint, normalizedPath;
	float	distance, length;
	
	CreateCircle();

	numConnected = -1;
	if (Connected)
	{	// we are connecting to an existing spline
		numConnected = Connected->GetNumPoints();
		if (numConnected >= SPLINE_MERGE_SIZE)
		{	// plenty of points to choose from
			mNumPoints += SPLINE_MERGE_SIZE;
		}
		else
		{	// the existing spline doesn't have enough points
			mNumPoints += numConnected;
		}
	}

	mPoints = (vec4_t *)malloc(sizeof(vec4_t) * mNumPoints);
	mWork = (vec4_t *)malloc(sizeof(vec4_t) * (mNumPoints+1));
	mWeights = (vec_t *)malloc(sizeof(vec_t) * (mNumPoints+1));

	length = sqrt((ex-bx)*(ex-bx) + (ey-by)*(ey-by));
	if (fabs(ex - bx) >= fabs(ey - by))
	{	// this appears to be a horizontal path
		mInc = 1.0 / fabs(ex - bx);
		horizontal = true;
		position = by;
		goal = ey;
		deltaGoal = (ey-by) / (numPoints-1);
	}
	else
	{	// this appears to be a vertical path
		mInc = 1.0 / fabs(ey - by);
		horizontal = false;
		position = bx;
		goal = ex;
		deltaGoal = (ex-bx) / (numPoints-1);
	}
	normalizedPath[0] = (ex-bx);
	normalizedPath[1] = (ey-by);
	Vector2Normalize(normalizedPath);
	// approx calculate how much we need to iterate through the spline to hit every point
	mInc /= 16;

	currentWidth = landscape->flrand(minWidth, maxWidth);
	currentPosition = 0.0;

	for(i=0;i<mNumPoints;i++)
	{
		// weights are evenly distributed
		mWeights[i] = (float)i / (mNumPoints-1);

		if (i < numConnected && i < SPLINE_MERGE_SIZE)
		{	// we are connecting to an existing spline, so copy over the first few points
			if (CreationFlags & PATH_CREATION_CONNECT_FRONT)
			{	// copy from the front
				index = i;
			}
			else
			{	// copy from the end
				index = numConnected-SPLINE_MERGE_SIZE+i;
			}
			point = Connected->GetPoint(index);
			mPoints[i][0] = point[0];
			mPoints[i][1] = point[1];
			mPoints[i][3] = point[3];
		}
		else
		{
			if (horizontal)
			{	// we appear to be going horizontal, so spread the randomness across the vertical
				mPoints[i][0] = ((ex - bx) * currentPosition) + bx;
				mPoints[i][1] = position;
			}
			else
			{	// we appear to be going vertical, so spread the randomness across the horizontal
				mPoints[i][0] = position;
				mPoints[i][1] = ((ey - by) * currentPosition) + by;
			}
			currentPosition += 1.0 / (numPoints-1);

			// set the width of the spline
			mPoints[i][3] = currentWidth;
			currentWidth += landscape->flrand(-0.10, 0.10);
			if (currentWidth < minWidth)
			{
				currentWidth = minWidth;
			}
			else if (currentWidth > maxWidth)
			{
				currentWidth = maxWidth;
			}

			// see how far we are from the goal
/*			delta = (goal - position) * currentPosition;
			// calculate the randomness we are allowed at this place
			random = landscape->flrand(-mDeviation/1.0, mDeviation/1.0) * (1.0 - currentPosition);
			position += delta + random;*/

			if (i == mNumPoints-2)
			{	// -2 because we are calculating for the next point
				position = goal;
			}
			else
			{
				if (i == 0)
				{
					position += deltaGoal + landscape->flrand(-mDeviation/10.0, mDeviation/10.0);
				}
				else
				{
					position += deltaGoal + landscape->flrand(-mDeviation*1.5, mDeviation*1.5);
				}
			}


			if (position > 0.9)
			{	// too far over, so move back a bit
				position = 0.9 - landscape->flrand(0.02, 0.1);
			}
			if (position < 0.1)
			{	// too near, so move bakc a bit
				position = 0.1 + landscape->flrand(0.02, 0.1);
			}

			// check our deviation from the straight line to the end
			if (horizontal)
			{
				testPoint[0] = ((ex - bx) * currentPosition) + bx;
				testPoint[1] = position;
			}
			else
			{
				testPoint[0] = position;
				testPoint[1] = ((ey - by) * currentPosition) + by;
			}
			// dot product of the normal of the path to the point we are at
			distance = ((testPoint[0]-bx)*normalizedPath[0]) + ((testPoint[1]-by)*normalizedPath[1]);
			// find the perpendicular place that is intersected by the point and the path
			percPoint[0] = (distance * normalizedPath[0]) + bx;
			percPoint[1] = (distance * normalizedPath[1]) + by;
			// calculate the difference between the perpendicular point and the test point
			diffPoint[0] = testPoint[0] - percPoint[0];
			diffPoint[1] = testPoint[1] - percPoint[1];
			// calculate the distance
			distance = sqrt((diffPoint[0]*diffPoint[0]) + (diffPoint[1]*diffPoint[1]));
			if (distance > mDeviation)
			{	// we are beyond our allowed deviation, so head back
				if (horizontal)
				{
					position = (ey-by) * currentPosition + by;
				}
				else
				{
					position = (ex-bx) * currentPosition + bx;
				}
				position += landscape->flrand(-mDeviation/2.0, mDeviation/2.0);
			}
		}
	}
	mWeights[mNumPoints] = BIG;
}

CPathInfo::~CPathInfo(void)
{
	free(mWeights);
	free(mWork);
	free(mPoints);
}

void CPathInfo::CreateCircle(void)
{
	int		x, y;
	float	r, d;

	memset(mCircleStamp, 0, sizeof(mCircleStamp));
	r = CIRCLE_STAMP_SIZE;
	for(x=0;x<CIRCLE_STAMP_SIZE;x++)
	{
		for(y=0;y<CIRCLE_STAMP_SIZE;y++)
		{
			d = sqrt((float)(x*x + y*y));
			if (d > r)
			{
				mCircleStamp[y][x] = 255;
			}
			else
			{
				mCircleStamp[y][x] = pow(sin(d / r * M_PI / 2), mBreadth) * 255;
			}
		}
	}
}

void CPathInfo::Stamp(int x, int y, int size, int depth, unsigned char *Data, int DataWidth, int DataHeight)
{
//	int xPos;
//	float yPos;
	int		dx, dy, fx, fy;
	float	offset;
	byte	value;
	byte	invDepth;

	offset = (float)(CIRCLE_STAMP_SIZE-1) / size;
	invDepth = 255-depth;

	for(dx = -size; dx <= size; dx++)
	{
		for ( dy = -size; dy <= size; dy ++ )
		{
			float d;

			d = dx * dx + dy * dy ;
			if ( d > size * size )
			{
				continue;
			}

			fx = x + dx;
			if (fx < 2 || fx > DataWidth-2)
			{
				continue;
			}
			
			fy = y + dy;
			if (fy < 2 || fy > DataHeight-2)
			{
				continue;
			}

			value = pow ( sin ( d / (size * size) * M_PI / 2), mBreadth ) * invDepth + depth;
			if (value < Data[(fy * DataWidth) + fx])
			{
				Data[(fy * DataWidth) + fx] = value;
			}
		}
	}
/*

		fx = x + dx;
		if (fx < 2 || fx > DataWidth-2)
		{
			continue;
		}
		xPos = abs((int)(dx*offset));
		yPos = offset*size + offset;
		for(dy = -size; dy < 0; dy++)
		{
			yPos -= offset;
			fy = y + dy;
			if (fy < 2 || fy > DataHeight-2)
			{
				continue;
			}

			value = (invDepth * mCircleStamp[(int)yPos][xPos] / 256) + depth;
			if (value < Data[(fy * DataWidth) + fx])
			{
				Data[(fy * DataWidth) + fx] = value;
			}
		}

		yPos = -offset;
		for(; dy <= size; dy++)
		{
			yPos += offset;

			fy = y + dy;
			if (fy < 2 || fy > DataHeight-2)
			{
				continue;
			}

			value = (invDepth * mCircleStamp[(int)yPos][xPos] / 256) + depth;
			if (value < Data[(fy * DataWidth) + fx])
			{
				Data[(fy * DataWidth) + fx] = value;
			}
		}
	}
*/
}

void CPathInfo::GetInfo(float PercentInto, vec4_t Coord, vec4_t Vector)
{
	vec4_t	before, after;
	float	testPercent;

	DialASpline(PercentInto, mWeights, mPoints, sizeof(vec4_t) / sizeof(vec_t), mNumPoints-1, mWork, 2, true, Coord);

	testPercent = PercentInto - 0.01;
	if (testPercent < 0)
	{
		testPercent = 0;
	}
	DialASpline(testPercent, mWeights, mPoints, sizeof(vec4_t) / sizeof(vec_t), mNumPoints-1, mWork, 2, true, before);

	testPercent = PercentInto + 0.01;
	if (testPercent > 1.0)
	{
		testPercent = 1.0;
	}
	DialASpline(testPercent, mWeights, mPoints, sizeof(vec4_t) / sizeof(vec_t), mNumPoints-1, mWork, 2, true, after);

	Coord[2] = mDepth;

	Vector[0] = after[0] - before[0];
	Vector[1] = after[1] - before[1];
}

void CPathInfo::DrawPath(unsigned char *Data, int DataWidth, int DataHeight )
{
	float			t;
	vec4_t			val, vector;//, perp;
	int				size;
	float			inc;
	int				x, y, lastX, lastY;
	float			depth;

	inc = mInc / DataWidth;

	lastX = lastY = -999;

	for (t=0.0; t<=1.0; t+=inc) 
	{
		GetInfo(t, val, vector);

/*		perp[0] = -vector[1];
		perp[1] = vector[0];

		if (fabs(perp[0]) > fabs(perp[1]))
		{
			perp[1] /= fabs(perp[0]);
			perp[0] /= fabs(perp[0]);
		}
		else
		{
			perp[0] /= fabs(perp[1]);
			perp[1] /= fabs(perp[1]);
		}
*/
		x = val[0] * DataWidth;
		y = val[1] * DataHeight;

		if (x == lastX && y == lastY)
		{
			continue;
		}

		lastX = x;
		lastY = y;

		size = val[3] * DataWidth;

		depth = mDepth * 255.0f;

		Stamp(x, y, size, (int)depth, Data, DataWidth, DataHeight);
	}
}







CRandomTerrain::CRandomTerrain(void)
{
	memset(mPaths, 0, sizeof(mPaths));
}

CRandomTerrain::~CRandomTerrain(void)
{
	Shutdown();
}

void CRandomTerrain::Init(CCMLandScape *landscape, byte *grid, int width, int height)
{
	Shutdown();
	mLandScape = landscape;
	mWidth = width;
	mHeight = height;
	mArea = mWidth * mHeight;
	mBorder = (width + height) >> 6;
	mGrid = grid;
}

void CRandomTerrain::ClearPaths(void)
{
	int	i;

	for(i=0;i<MAX_RANDOM_PATHS;i++)
	{
		if (mPaths[i])
		{
			delete mPaths[i];
			mPaths[i] = 0;
		}
	}

	memset(mPaths, 0, sizeof(mPaths));
}

void CRandomTerrain::Shutdown(void)
{
	ClearPaths ( );
}

bool CRandomTerrain::CreatePath(int PathID, int ConnectedID, unsigned CreationFlags, int numPoints, 
					float bx, float by, float ex, float ey, 
					float minWidth, float maxWidth, float depth, float deviation, float breadth )
{
	CPathInfo	*connected = 0;

	if (PathID < 0 || PathID >= MAX_RANDOM_PATHS || mPaths[PathID])
	{
		return false;
	}

	if (ConnectedID >= 0 && ConnectedID < MAX_RANDOM_PATHS)
	{
		connected = mPaths[ConnectedID];
	}

	mPaths[PathID] = new CPathInfo(mLandScape, numPoints, bx, by, ex, ey, 
		minWidth, maxWidth, depth, deviation, breadth,
		connected, CreationFlags );

	return true;
}

bool CRandomTerrain::GetPathInfo(int PathNum, float PercentInto, vec4_t Coord, vec4_t Vector)
{
	if (PathNum < 0 || PathNum >= MAX_RANDOM_PATHS || !mPaths[PathNum])
	{
		return false;
	}

	mPaths[PathNum]->GetInfo(PercentInto, Coord, Vector);

	return true;
}

void CRandomTerrain::ParseGenerate(const char *GenerateFile)
{
}

void CRandomTerrain::Smooth ( void )
{
	// Scale down to 1/4 size then back up to smooth out the terrain
	byte	*temp;
	int		x, y, o;

	temp = mLandScape->GetFlattenMap ( );

	// Copy over anything in the flatten map
	for ( o = 0; o < mHeight * mWidth; o++)
	{
		if ( temp[o] > 0 )
		{
			mGrid[o] = (byte)temp[o] & 0x7F;
		}
	}

	temp = (byte *)Z_Malloc(mWidth * mHeight, TAG_RESAMPLE);
#if 1
	unsigned	total, count;
	for(x=1;x<mWidth-1;x++)
	{
		for(y=1;y<mHeight-1;y++)
		{
			total = 0;
			count = 2;

			// Left
			total += mGrid[((y)*mWidth)+(x-1)];
			count++;

			// Right
			total += mGrid[((y)*mWidth)+(x+1)];
			count++;

			// Up
			total += mGrid[((y-1)*mWidth)+(x)];
			count++;

			// Down
			total += mGrid[((y+1)*mWidth)+(x)];
			count++;

			// Up-Left
			total += mGrid[((y-1)*mWidth)+(x-1)];
			count++;

			// Down-Left
			total += mGrid[((y+1)*mWidth)+(x-1)];
			count++;

			// Up-Right
			total += mGrid[((y-1)*mWidth)+(x+1)];
			count++;

			// Down-Right
			total += mGrid[((y+1)*mWidth)+(x+1)];
			count++;

			total += (unsigned)mGrid[((y)*mWidth)+(x)] * 2;

			temp[((y)*mWidth)+(x)] = total / count;
		}
	}

	memcpy(mGrid, temp, mWidth * mHeight);

#else
	float	smoothKernel[FILTER_SIZE][FILTER_SIZE];
	int		xx, yy, dx, dy;
	float	total, num;

	re.Resample(mGrid, mWidth, mHeight, temp, mWidth >> 1, mHeight >> 1, 1);
	re.Resample(temp, mWidth >> 1, mHeight >> 1, mGrid, mWidth, mHeight, 1);

	// now lets filter it.
	memcpy(temp, mGrid, mWidth * mHeight);

	for (dy = -KERNEL_SIZE; dy <= KERNEL_SIZE; dy++)
	{
		for (dx = -KERNEL_SIZE; dx <= KERNEL_SIZE; dx++)
		{
			smoothKernel[dy + KERNEL_SIZE][dx + KERNEL_SIZE] =
				1.0f / (1.0f + fabs(float(dx) * float(dx) * float(dx)) + fabs(float(dy) * float(dy) * float(dy)));
		}
	}

	for (y = 0; y < mHeight; y++)
	{
		for (x = 0; x < mWidth; x++)
		{
			total = 0.0f;
			num = 0.0f;
			for (dy = -KERNEL_SIZE; dy <= KERNEL_SIZE; dy++)
			{
				for (dx = -KERNEL_SIZE; dx <= KERNEL_SIZE; dx++)
				{
					xx = x + dx;
					if (xx >= 0 && xx < mWidth)
					{
						yy = y + dy;
						if (yy >= 0 && yy < mHeight)
						{
							total += smoothKernel[dy + KERNEL_SIZE][dx + KERNEL_SIZE] * (float)temp[yy * mWidth + xx];
							num += smoothKernel[dy + KERNEL_SIZE][dx + KERNEL_SIZE];
						}
					}
				}
			}
			total /= num;
			mGrid[y * mWidth + x] = (byte)Com_Clamp(0, 255, (int)Round(total));
		}
	}
#endif

	Z_Free(temp);

/* Uncomment to see the symmetry line on the map

	for ( x = 0; x < mWidth; x ++ )
	{
		mGrid[x * mWidth + x] = 255;
	}
*/
}

void CRandomTerrain::Generate(int symmetric)
{
	int	i,j;
	int x, y;

	// Clear out all existing data
	memset(mGrid, 255, mArea);

	// make landscape a little bumpy
	float t1 = mLandScape->flrand(0, 2);
#if 0
	float t2 = mLandScape->flrand(0, 2);
	float t3 = mLandScape->flrand(0, 2);
#endif

/*
	CM_NoiseInit(mLandScape);

	for (y = 0; y < mHeight; y++)
		for (x = 0; x < mWidth; x++)
		{
			i = x + y*mWidth;
			byte val = (byte)Com_Clamp(0, 255, (int)(220 + (CM_NoiseGet4f( x*0.25, y*0.25, 0, t3 ) * 20)) + (CM_NoiseGet4f( x*0.5, y*0.5, 0, t2 ) * 15));
			mGrid[i] = val;
		}
*/

	for ( i = 0; mPaths[i] != 0; i ++ )
	{
		mPaths[i]->DrawPath(mGrid, mWidth, mHeight);
	}

	for (y = 0; y < mHeight; y++)
		for (x = 0; x < mWidth; x++)
		{
			i = x + y*mWidth;
			byte val = (byte)Com_Clamp(0, 255, (int)(mGrid[i] + (CM_NoiseGet4f( x, y, 0, t1 ) * 5))); 
			mGrid[i] = val;
		}

	// if symmetric, do this now
	if (symmetric)
	{
		assert (mWidth == mHeight); // must be square

		for (y = 0; y < mHeight; y++)
			for (x = 0; x < (mWidth-y); x++)
			{
				i = x + y*mWidth;
				j = (mWidth-1 - x) + (mHeight-1 - y)*mWidth;
				byte val = mGrid[i] < mGrid[j] ? mGrid[i] : mGrid[j];
				mGrid[i] = mGrid[j] = val;
			}
	}
}

typedef enum
{
	CP_NONE					= -1,
	CP_CONSONANT,
	CP_COMPLEX_CONSONANT,
	CP_VOWEL,
	CP_COMPLEX_VOWEL,
	CP_ENDING,

	CP_NUM_PIECES,
} ECPType;

typedef struct SCharacterPiece
{
	char	*mPiece;
	int		mCommonality;
} TCharacterPiece;

static TCharacterPiece	Consonants[] = 
{
	{	"b", 6 },
	{	"c", 8 },
	{	"d", 6 },
	{	"f", 5 },
	{	"g", 4 },
	{	"h", 5 },
	{	"j", 2 },
	{	"k", 4 },
	{	"l", 4 },
	{	"m", 7 },
	{	"n", 7 },
	{	"r", 6 },
	{	"s", 10 },
	{	"t", 10 },
	{	"v", 1 },
	{	"w", 2 },
	{	"x", 1 },
	{	"z", 1 },

	{	0, 0 }
};

static TCharacterPiece	ComplexConsonants[] = 
{
	{	"st", 10 },
	{	"ck", 10 },
	{	"ss", 10 },
	{	"tt", 7 },
	{	"ll", 8 },
	{	"nd", 10 },
	{	"rn", 6 },
	{	"nc", 6 },
	{	"mp", 4 },
	{	"sc", 10 }, 
	{	"sl", 10 }, 
	{	"tch", 6 }, 
	{	"th", 4 }, 
	{	"rn", 5 }, 
	{	"cl", 10 }, 
	{	"sp", 10 }, 
	{	"st", 10 }, 
	{	"fl", 4 }, 
	{	"sh", 7 }, 
	{	"ng", 4 }, 
//	{	"" },

	{	0, 0 }
};

static TCharacterPiece	Vowels[] = 
{
	{	"a", 10 },
	{	"e", 10 },
	{	"i", 10 },
	{	"o", 10 },
	{	"u", 2 },
//	{	"" },

	{	0, 0 }
};

static TCharacterPiece	ComplexVowels[] = 
{
	{	"ea", 10  },
	{	"ue", 3 },
	{	"oi", 10 },
	{	"ai", 8 },
	{	"oo", 10 },
	{	"io", 10 },
	{	"oe", 10 },
	{	"au", 3 },
	{	"ee", 7 },
	{	"ei", 7 },
	{	"ou", 7 }, 
	{	"ia", 4 }, 
//	{	"" },

	{	0, 0 }
};

static TCharacterPiece	Endings[] = 
{
	{	"ing", 10 },
	{	"ed", 10 },
	{	"ute", 10 },
	{	"ance", 10 },
	{	"ey", 10 },
	{	"ation", 10 },
	{	"ous", 10 },
	{	"ent", 10 },
	{	"ate", 10 },
	{	"ible", 10 },
	{	"age", 10 },
	{	"ity", 10 },
	{	"ist", 10 },
	{	"ism", 10 },
	{	"ime", 10 },
	{	"ic", 10 },
	{	"ant", 10 },
	{	"etry", 10 },
	{	"ious", 10 },
	{	"ative", 10 },
	{	"er", 10 },
	{	"ize", 10 }, 
	{	"able", 10 }, 
	{	"itude", 10 }, 
//	{	"" },

	{	0, 0 }
};

static void FindPiece(ECPType type, char *&pos)
{
	TCharacterPiece		*search, *start;
	int					count = 0;

	switch(type)
	{
		case CP_CONSONANT:
		default:
			start = Consonants;
			break;

		case CP_COMPLEX_CONSONANT:
			start = ComplexConsonants;
			break;

		case CP_VOWEL:
			start = Vowels;
			break;

		case CP_COMPLEX_VOWEL:
			start = ComplexVowels;
			break;

		case CP_ENDING:
			start = Endings;
			break;
	}

	search = start;
	while(search->mPiece)
	{
		count += search->mCommonality;
		search++;
	}

	count = irand(0, count-1);
	search = start;
	while(count > search->mCommonality)
	{
		count -= search->mCommonality;
		search++;
	}

	strcpy(pos, search->mPiece);
	pos += strlen(search->mPiece);
}

unsigned RMG_CreateSeed(char *TextSeed)
{
	int			Length;
	char		Ending[256], *pos;
	int			ComplexVowelChance, ComplexConsonantChance;
	ECPType		LookingFor;
	unsigned	SeedValue = 0, high;

	Length = irand(4, 9);

	if (irand(0, 100) < 20)
	{ 
		LookingFor = CP_VOWEL;
	}
	else
	{
		LookingFor = CP_CONSONANT;
	}

	Ending[0] = 0;

	if (irand(0, 100) < 55)
	{
		pos = Ending;
		FindPiece(CP_ENDING, pos);
		Length -= (pos - Ending);
	}

	pos = TextSeed;
	*pos = 0;

	ComplexVowelChance = -1;
	ComplexConsonantChance	= -1;

	while((pos - TextSeed) < Length || LookingFor == CP_CONSONANT)
	{
		if (LookingFor == CP_VOWEL)
		{
			if (irand(0, 100) < ComplexVowelChance)
			{
				ComplexVowelChance = -1;
				LookingFor = CP_COMPLEX_VOWEL;
			}
			else
			{
				ComplexVowelChance += 10;
			}

			FindPiece(LookingFor, pos);
			LookingFor = CP_CONSONANT;
		}
		else
		{
			if (irand(0, 100) < ComplexConsonantChance)
			{
				ComplexConsonantChance = -1;
				LookingFor = CP_COMPLEX_CONSONANT;
			}
			else
			{
				ComplexConsonantChance += 45;
			}

			FindPiece(LookingFor, pos);
			LookingFor = CP_VOWEL;
		}
	}

	if (Ending[0])
	{
		strcpy(pos, Ending);
	}

	pos = TextSeed;
	while(*pos)
	{
		high = SeedValue >> 28;
		SeedValue ^= (SeedValue << 4) + ((*pos)-'a');
		SeedValue ^= high;
		pos++;
	}

	return SeedValue;
}
