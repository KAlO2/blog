#include "image.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strrchr

#include <jpeglib.h> // libjpeg
#include <tiffio.h>  // libtiff
#include <png.h>     // libpng

#include <GL/gl.h>   // for glPixelStore & glReadPixels

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
	else
	{
		fprintf(stderr, "unimplemented picture format [%s].\n", pos);
		return false;
	}
}
