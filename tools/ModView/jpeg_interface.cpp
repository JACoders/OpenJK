// Filename:-	tr_jpeg_interace.cpp
//
#include "stdafx.h"
#include "includes.h"
#include "r_common.h"
//#include "system.h"
//#include "oddbits.h"
//#include "tr_local.h"
#include "jpeg_interface.h"

#include "jpeg6\jpeglib.h"

void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height ) 
{	
	*pic = NULL;

//	static char sTEMP[]="Q:\\quake\\baseq3\\menu\\common\\cursor.jpg";
//	filename = sTEMP;

	/* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  unsigned char *out;
  byte	*fbuffer = NULL;
  byte  *bbuf;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

//  LoadFile (filename, (void **)&fbuffer, false);
  int iBytesRead = ri.FS_ReadFile ( ( char * ) filename, (void **)&fbuffer);
  if (!fbuffer) 
	return;

  // this JPG loader is a bit crap, and often causes read-violations because of reading an extra 4k from the input
  //	buffer that isn't actually there, causes problems on NT4 and NT2000, so allocate 4K more and use that instead.
  //
  byte *fbuffer2 = (byte*) ri.Malloc( iBytesRead+4096 );
  if (!fbuffer2)
  {
	  free(fbuffer);
	  return;
  }

  memcpy(fbuffer2,fbuffer,iBytesRead);
  free(fbuffer);
  fbuffer = fbuffer2;  

  try
  {

	  /* Step 1: allocate and initialize JPEG decompression object */

	  /* We have to set up the error handler first, in case the initialization
	   * step fails.  (Unlikely, but it could happen if you are out of memory.)
	   * This routine fills in the contents of struct jerr, and returns jerr's
	   * address which we place into the link field in cinfo.
	   */
	  cinfo.err = jpeg_std_error(&jerr);

	  /* Now we can initialize the JPEG decompression object. */
	  jpeg_create_decompress(&cinfo);

	  /* Step 2: specify data source (eg, a file) */

	  jpeg_stdio_src(&cinfo, fbuffer);

	  /* Step 3: read file parameters with jpeg_read_header() */

	  (void) jpeg_read_header(&cinfo, TRUE);
	  /* We can ignore the return value from jpeg_read_header since
	   *   (a) suspension is not possible with the stdio data source, and
	   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	   * See libjpeg.doc for more info.
	   */

	  /* Step 4: set parameters for decompression */

	  /* In this example, we don't need to change any of the defaults set by
	   * jpeg_read_header(), so we do nothing here.
	   */

	  /* Step 5: Start decompressor */

	  (void) jpeg_start_decompress(&cinfo);
	  /* We can ignore the return value since suspension is not possible
	   * with the stdio data source.
	   */

	  /* We may need to do some setup of our own at this point before reading
	   * the data.  After jpeg_start_decompress() we have the correct scaled
	   * output image dimensions available, as well as the output colormap
	   * if we asked for color quantization.
	   * In this example, we need to make an output work buffer of the right size.
	   */ 
	  /* JSAMPLEs per row in output buffer */
	  row_stride = cinfo.output_width * cinfo.output_components;

	  if (cinfo.output_components!=4 && cinfo.output_components!=1 ) 
	  {
		  WarningBox(va("JPG %s is unsupported color depth (%d)", filename, cinfo.output_components));
	  }

	  out = (byte *) malloc(cinfo.output_width*cinfo.output_height*4);

	  *pic = out;
	  *width = cinfo.output_width;
	  *height = cinfo.output_height;

	  /* Step 6: while (scan lines remain to be read) */
	  /*           jpeg_read_scanlines(...); */

	  /* Here we use the library's state variable cinfo.output_scanline as the
	   * loop counter, so that we don't have to keep track ourselves.
	   */
	  while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		bbuf = ((out+(row_stride*cinfo.output_scanline)));
		buffer = &bbuf;
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
	  }

	  // if we've just loaded a greyscale, then adjust it from 8-bit to 32bit by stretch-copying it over itself...
	  //  (this also does the alpha stuff as well)
	  //
	  if (cinfo.output_components == 1)
	  {
		  byte *pbDest = (*pic + (cinfo.output_width * cinfo.output_height * 4))-1;
		  byte *pbSrc  = (*pic + (cinfo.output_width * cinfo.output_height    ))-1;
		  int  iPixels = cinfo.output_width * cinfo.output_height;

		  for (int i=0; i<iPixels; i++)
		  {
			  byte b = *pbSrc--;
			  *pbDest-- = 255;
			  *pbDest-- = b;
			  *pbDest-- = b;
			  *pbDest-- = b;
		  }
	  }
	  else	  
	  {// clear all the alphas to 255
		  int	i, j;
			byte	*buf;

			buf = *pic;

		  j = cinfo.output_width * cinfo.output_height * 4;
		  for ( i = 3 ; i < j ; i+=4 ) {
			  buf[i] = 255;
		  }
	  }

	  /* Step 7: Finish decompression */

	  (void) jpeg_finish_decompress(&cinfo);
	  /* We can ignore the return value since suspension is not possible
	   * with the stdio data source.
	   */

	  /* Step 8: Release JPEG decompression object */

	  /* This is an important step since it will release a good deal of memory. */
	  jpeg_destroy_decompress(&cinfo);
  }

	catch(LPCSTR psMessage)
	{	
		ErrorBox(va("JPEG read error: %s",psMessage));
		jpeg_destroy_decompress(&cinfo);
//		bReturn = false;
		if (*pic)
		{
			free(*pic);
			*pic = NULL;
		}
	}



  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  free(fbuffer);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
}



void JPG_ErrorThrow(LPCSTR message)
{
//	ri.Error( ERR_FATAL, "%s\n", message );
	throw (message);
}

void JPG_MessageOut(LPCSTR message)
{
//	ri.Printf(PRINT_ALL, "%s\n", message);
	OutputDebugString(va("%s\n", message));
}

void *JPG_Malloc( int iSize )
{
	//return (void *) ri.Malloc(iSize);
//	OutputDebugString(va("%d\n", iSize));
	return (void *) malloc(iSize);
}

void JPG_Free( void *pvObject)
{
	//ri.Free(pvObject);
	free(pvObject);
}



//////////////// eof ////////////

