#include "image.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strrchr

#include "jpeglib.h" // libjpeg
#ifdef HAVE_TIFF
#include "tiffio.h"  // libtiff
#endif
#ifdef HAVE_PNG
#include "png.h"   // libpng
#endif

#include <GL/gl.h>   // for glPixelStore & glReadPixels
#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif

#ifdef _WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif
bool snapshot_bmp(int width, int height, const char* path)
{
	const size_t pitch=(width*3+3)&~3; // DWORD aligned line
	const size_t len=pitch*height*sizeof(GLbyte);
	GLbyte* buffer = (GLbyte*)malloc(len);
	if(!buffer)
	{
		fprintf(stderr, "buffer alloc failed!\n");
		return false;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer);

	FILE *file = fopen(path, "wb");
	if (!file)
	{
		fprintf(stderr, "can't open %s\n", path);
		free(buffer);
		return false;
	}

	BITMAPFILEHEADER file_header;
	BITMAPINFOHEADER info_header;

	file_header.bfType = (WORD)('B'|'M'<<8); // Windows BMP file tag
	file_header.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+len;
	file_header.bfReserved1 = 0;
	file_header.bfReserved2 = 0;
	file_header.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

	info_header.biSize = sizeof(BITMAPINFOHEADER);
	info_header.biWidth = width;
	info_header.biHeight = height;
	info_header.biPlanes = 1;
	info_header.biBitCount = 24;
	info_header.biCompression = 0; // BI_RGB, An uncompressed format.
	info_header.biSizeImage = len; // size in bytes. This may be set to zero for BI_RGB bitmaps
	info_header.biXPelsPerMeter = 0;
	info_header.biYPelsPerMeter = 0;
	info_header.biClrUsed = 0;
	info_header.biClrImportant = 0;

	fwrite(&file_header, sizeof(file_header), 1, file);
	fwrite(&info_header, sizeof(info_header), 1, file);
	fwrite(buffer, len, 1, file);
	fclose(file);

	free(buffer);
	return true;
}


bool snapshot_jpg(int width, int height, const char *path, int quality)
{
	struct jpeg_compress_struct cinfo; // a JPEG object
	struct jpeg_error_mgr jerr;        // a JPEG error handler
	unsigned char *row_pointer[1];     // pointer to JSAMPLE row[s]
	
	// Step 1: allocate and initialize JPEG compression object
	cinfo.err=jpeg_std_error(&jerr); // set up the error handler first
	jpeg_create_compress(&cinfo);    // compression object
	
	// Step 2: specify data destination
	FILE *file=fopen(path, "wb");
	if(!file)
	{
		fprintf(stderr, "can't open %s\n", path);
		return false;
	}
	
	jpeg_stdio_dest(&cinfo, file);   // tie stdio object to JPEG object
	
	const size_t len=width*height*3*sizeof(GLubyte);
	GLubyte *pixels=(GLubyte*)malloc(len);

	if (!pixels)
	{
		fprintf(stderr, "buffer alloc failed!\n");
		fclose(file);
		return false;
	}
	
	// save the screen shot into the buffer
	// glReadBuffer(GL_FRONT_LEFT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// give some specifications about the image to save to libjpeg
	// Step 3: set parameters for compression
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3; // color components per pixel, 3 for RGB
	cinfo.in_color_space = JCS_RGB; // colorspace of input image

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, true/*force_baseline*/);
	
	// Step 4: Start compressor
	jpeg_start_compress(&cinfo, true/*write_all_tables*/);
	
	// Step 5: while (scan lines remain to be written)
	const int row_stride=width*3; // physical row width in image buffer
	const int last_line=len-row_stride;
	while(cinfo.next_scanline < cinfo.image_height)
	{
		// OpenGL writes from bottom to top while libjpeg goes from top to bottom.
		row_pointer[0]=&pixels[last_line - cinfo.next_scanline*row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	// Step 6: Finish compression
	jpeg_finish_compress(&cinfo);
	fclose(file);
	
	// Step 7: release JPEG compression object 
	jpeg_destroy_compress(&cinfo);
	
	return true;
}

#ifdef HAVE_TIFF
bool snapshot_tiff(int width, int height, const char* path)
{
	TIFF *file=TIFFOpen(path, "wb");
	if(!file)
	{
		fprintf(stderr, "can't open %s\n", path);
		return false;
	}
	
	const size_t line=sizeof(GLubyte)*3*width;
	GLubyte *image = (GLubyte*)malloc(line*height);

	// Make sure the rows are packed as tight as possible (no row padding),
	// set the pack alignment to 1.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
	
	TIFFSetField(file, TIFFTAG_IMAGEWIDTH, (uint32) width);
	TIFFSetField(file, TIFFTAG_IMAGELENGTH, (uint32) height);
	TIFFSetField(file, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(file, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
	TIFFSetField(file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(file, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(file, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(file, TIFFTAG_IMAGEDESCRIPTION, "OpenGL snapshot");
	
	GLubyte *p = image;
	for(int i=height-1; i>=0; --i)
	{
		if(TIFFWriteScanline(file, p, i, 0)<0)
		{
			free(image);
			TIFFClose(file);
			return false;
		}
		p+=line;
	}
	
	TIFFClose(file);
	return true;
}
#endif /* HAVE_TIFF */

#ifdef HAVE_PNG
bool snapshot_png(int width, int height, const char* path)
{
	FILE *file = fopen(path, "wb");
	if(!file)
	{
		printf("can't open file %s for writing\n", path);
		return false;
	}

	png_struct* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL/*user_error_ptr*/, NULL/*user_error_fn*/, NULL/*user_warning_fn*/);
	if(!png_ptr)
	{
		printf("error: png_create_read_struct returned 0.\n");
		fclose(file);
		return false;
	}
	
	png_info* info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr || setjmp(png_jmpbuf(png_ptr)))
	{
		printf("error: png_create_read_struct returned 0 or set error handling failed.\n");
		png_destroy_write_struct(&png_ptr, (info_ptr==NULL)?NULL:&info_ptr);
		fclose(file);
		return false;
	}
    
    png_init_io(png_ptr, file);
    
	/* Set the image information here. Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	 * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	 * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	 * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	 * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	 */
	const int bit_depth = 8;
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	/* Optional significant bit (sBIT) chunk */
	png_color_8 sig_bit;
	sig_bit.red = bit_depth;
	sig_bit.green = bit_depth;
	sig_bit.blue = bit_depth;
	sig_bit.alpha = 0; // no alpha channel
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

	// Write the file header information.
	png_write_info(png_ptr, info_ptr);
	
	if((png_uint_32)height > PNG_UINT_32_MAX/(sizeof(png_byte*)))
		png_error(png_ptr, "Image is too tall to process in memory");
	
	png_byte** row_ptrs = (png_byte**)malloc(height * sizeof(png_byte*));
	if(!row_ptrs)
		printf("malloc failed for row_ptrs.\n");
	else
	{
		const size_t line = sizeof(GLubyte) * width * 3; // RGB components
		GLubyte *data = (GLubyte*)malloc(line * height);

		if(data)
		{
			/* Make sure the rows are packed as tight as possible (no row padding),
			 * set the pack alignment to 1.
			 */
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

			/* Set the individual row_ptrs to point at the correct offsets of image 
			 * data, note that it's upside down to keep consistent with the screen
			 * coordinate system.
			 */
			for(int row = 0; row < height; ++row)
				row_ptrs[row] = (png_byte*)data + line * row;

			png_write_image(png_ptr, row_ptrs);
			png_write_end(png_ptr, info_ptr);

			free(data);
		}
	}

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	free(row_ptrs);
	
	return(row_ptrs != NULL);
}
#endif

bool snapshot(int width, int height, const char* path)
{
	const char* pos=strrchr(path, '.');
	if(!pos)
	{
		fprintf(stderr, "path [%s] has no suffix.\n", path);
		return false;
	}
	
	++pos; // skip current dot to suffix
	if(!strcasecmp(pos, "bmp"))
		return snapshot_bmp(width, height, path);
	else if(!strcasecmp(pos, "jpg") || !strcasecmp(pos, "jpeg"))
		return snapshot_jpg(width, height, path);
#ifdef HAVE_TIFF
	else if(!strcasecmp(pos, "tiff") || !strcasecmp(pos, "tif"))
		return snapshot_tiff(width, height, path);
#endif /* HAVE_TIFF */
#ifdef HAVE_PNG
	else if(!strcasecmp(pos, "png"))
		return snapshot_png(width, height, path);
#endif /* HAVE_PNG */
	else
	{
		fprintf(stderr, "unimplemented picture format [%s].\n", pos);
		return false;
	}
}
