#ifndef IMAGE_H_
#define IMAGE_H_

typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long           LONG;

#if defined(_MSC_VER)
#  define PACKED_STRUCT(declaration)   \
	__pragma(pack(push, 1))            \
	struct declaration                 \
	__pragma(pack(pop))

#elif defined(__GNUC__)
#  define PACKED_STRUCT(declaration)   \
		struct __attribute__((packed)) \
		declaration
#endif


#ifndef _WIN32

/**
 * For information on The BITMAPFILEHEADER structure, see:
 * http://msdn.microsoft.com/en-us/library/windows/desktop/dd183374(v=vs.85).aspx
 */
typedef PACKED_STRUCT(tagBITMAPFILEHEADER {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
}) BITMAPFILEHEADER, *PBITMAPFILEHEADER;

/**
 * For information on The BITMAPINFOHEADER structure, see:
 * http://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
 */
typedef PACKED_STRUCT( tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
}) BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#endif /* _WIN32 */

/**
 * return true if snapshot successfully, otherwise false.
 */
bool snapshot_bmp(int width, int height, const char* path);

/**
 * return true if snapshot successfully, otherwise false.
 * The quality value ranges from 0(worst) to 100(best), default to 85
 */
bool snapshot_jpg(int width, int height, const char *path, int quality=85);

#ifdef HAVE_TIFF
/**
 * return true if snapshot successfully, otherwise false.
 */
bool snapshot_tiff(int width, int height, const char* path);
#endif

#ifdef HAVE_PNG
/**
 * @brief Save image data to disk, using cstdio, libpng.
 * 
 * @param path The PNG file to be saved
 * @param width Width of PNG file
 * @param height Height of PNG file
 * @return true if snapshot successfully, otherwise false.
 */
bool snapshot_png(int width, int height, const char* path);
#endif

/**
 * Call some snapshot_* function above according the given path's suffix.
 * return true if snapshot successfully, otherwise false.
 */
bool snapshot(int width, int height, const char* path);

#endif /* IMAGE_H_ */
