#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <complex>
#include <vector>

#include <png.h>

//#define USE_DOUBLE_PRECISION

#ifdef USE_DOUBLE_PRECISION
	typedef double real;
#else
	typedef float real;
#endif /* USE_DOUBLE_PRECISION */


/**
 * @brief Save image data to disk, using cstdio, libpng.
 * 
 * @param filename The PNG file to be saved
 * @param width Width of PNG file
 * @param height Height of PNG file
 * @param data Hold image data
 * @return True if save PNG image successfully, otherwise false.
 */
bool savePNGImage(const char* filename, int width, int height, void* data)
{
	FILE* file = fopen(filename, "wb");
	if(!file)
	{
		printf("cannot open file %s for writing.\n", filename);
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
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	/* Optional significant bit (sBIT) chunk */
	png_color_8 sig_bit;
	sig_bit.red = bit_depth;
	sig_bit.green = bit_depth;
	sig_bit.blue = bit_depth;
	sig_bit.alpha = bit_depth;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	
	// Optionally write comments into the image
	{
		png_text text_ptr[3];
		memset(&text_ptr, 0, sizeof(text_ptr));
		
		text_ptr[0].key = (png_charp)"Genre";
		text_ptr[0].text = (png_charp)"Math";
		text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;

		text_ptr[1].key = (png_charp)"Author";
		text_ptr[1].text = (png_charp)"KAlO2";
		text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;

		text_ptr[2].key = (png_charp)"Description";
		text_ptr[2].text = (png_charp)"Mandelbrot set, beauty from the fractal world.";
		text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;

		png_set_text(png_ptr, info_ptr, text_ptr, 3);
	}

	// Write the file header information.
	png_write_info(png_ptr, info_ptr);
	
	if((png_uint_32)height > PNG_UINT_32_MAX/(sizeof(png_byte*)))
		png_error(png_ptr, "Image is too tall to process in memory");
		
	png_byte** row_ptrs = (png_byte**)malloc(height*sizeof(png_byte*));
	if(!row_ptrs)
		printf("malloc failed for row_ptrs.\n");
	else
	{
		/* Set the individual row_ptrs to point at the correct offsets of image 
		 * data, note that it's upside down to keep consistent with the screen
		 * coordinate system.
		 */
		for(int row = 0; row < height; ++row)
			row_ptrs[row] = (png_byte*)data + row*width*4; // bytes_per_pixel

		png_write_image(png_ptr, row_ptrs);
		png_write_end(png_ptr, info_ptr);
	}

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	free(row_ptrs);
	
	return(row_ptrs!=NULL);
}

//  g++ mandelbrot.cpp -o mandelbrot -Wall -lstdc++ -lpng
int main(int argc, char* argv[])
{
	using std::vector;

    /*
	 * Mandelbrot set is contained in the closed disk of radius 2 around the 
	 * origin, or more precisely, in the region [-2.0, 0.6] x [-1.35, 1.35],
	 * so let's map image pixels [0, width]x[0, height] on it, you can tweak
	 * width and height to make high revolution images.
	 */
	const int N=800; // width and height set to N
	const int len=N*N;
	vector<int> image(len);
	
	const int COUNT=1000; // maximum iteration, can be tweaked
	real hist[COUNT];    // histogram about iteration numbers
	memset(&hist, 0, sizeof(hist));
//	vector<real> hist(COUNT);
	
    // diameter=4, map interval 0:side to -2:2
    vector<real> table(N);
    for(int i=0; i<N; ++i)
        table[i]=static_cast<real>(i<<2)/N-2;

    // the image of mandelbrot set is symmetric about the x-axis, no need to flip top to bottom.
	for(int y=0; y<N; ++y)
	{
        const real y0=table[y];
		for(int x=0; x<N; ++x)
		{	
            const real x0=table[x];
			std::complex<real> z0(x0, y0);
			
			int count=0;
			std::complex<real> c(0, 0);
			while((norm(c)<2*2 && count<COUNT))
			{
				c=c*c+z0;
				++count;
			}
			
			++hist[count];
			image[y*N+x]=count;	
		}
	}
	
	real total=static_cast<real>(len);
	real front=total;

	for(int i=COUNT-1; i>=0; --i)
	{
		real level=front/total;
		front=front-hist[i];
		hist[i]=level;
	}
	
	for(int i=0; i<len; ++i)
	{
		const int v=image[i];
		unsigned char value=static_cast<unsigned char>(hist[v]*255);
		image[i]=((unsigned char)255)<<24 | (value<<16) | ((256+value)<<7) | (92+value/3);
    }
	
	savePNGImage("mandelbrot.png", N/*width*/, N/*height*/, &image[0]);
	
	return 0;
}
