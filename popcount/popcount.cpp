#include <cstdio>
#include <climits>
#include <cstdint>
#include <cassert>
#include <ctime>

int iterated_popcnt(uint32_t n)
{
	int count = 0;
	for(; n; n >>= 1U)
		count += n & 1U;
	return count;
}

int sparse_popcnt(uint32_t n)
{
	int count = 0;
	while(n)
	{
		++count;
		n &= n - 1;
	}
	return count;
}

int dense_popcnt(uint32_t n)
{
	uint32_t count = 32;  // sizeof(uint32_t) * CHAR_BIT;
	n ^= static_cast<uint32_t>(-1);
	while(n)
	{
		--count;
		n &= n - 1;
	}
	return count;
}

#ifdef USE_MACRO

int lookup_popcnt(uint32_t n)
{
# define BIT2(n)      n,       n+1,       n+1,       n+2
# define BIT4(n) BIT2(n), BIT2(n+1), BIT2(n+1), BIT2(n+2)
# define BIT6(n) BIT4(n), BIT4(n+1), BIT4(n+1), BIT4(n+2)
# define BIT8(n) BIT6(n), BIT6(n+1), BIT6(n+1), BIT6(n+2)

	static_assert(CHAR_BIT == 8);
	static const uint8_t table[256] = { BIT8(0) };

	return 
		table[(n    ) & 0xFF] +
		table[(n>> 8) & 0xFF] +
		table[(n>>16) & 0xFF] +
		table[(n>>24) & 0xFF];
}

#else
static constexpr size_t TBL_LEN = 1U << CHAR_BIT;
static uint8_t table[TBL_LEN] = {0};

int lookup_popcnt(uint32_t n)
{
	uint8_t *p = reinterpret_cast<uint8_t*>(&n);
	return table[p[0]] + table[p[1]] + table[p[2]] + table[p[3]];
}

#endif  // USE_MACRO

#define POW2(c)    (1u << (c))
#define MASK(c)    (UINT_MAX / (POW2(POW2(c)) + 1u))
#define COUNT(x, c) (((x) & MASK(c)) + (((x)>>POW2(c)) & MASK(c)))

int parallel_popcnt(uint32_t n)
{
	n = COUNT(n, 0);
	n = COUNT(n, 1);
	n = COUNT(n, 2);
	n = COUNT(n, 3);
	n = COUNT(n, 4);
//  n = COUNT(n, 5);  // uncomment this line for 64-bit integers
	return n;
}

#define MASK_01010101 (((unsigned int)(-1))/3)
#define MASK_00110011 (((unsigned int)(-1))/5)
#define MASK_00001111 (((unsigned int)(-1))/17)

int nifty_popcnt(uint32_t n)
{
	n = (n & MASK_01010101) + ((n>>1) & MASK_01010101);
	n = (n & MASK_00110011) + ((n>>2) & MASK_00110011);
	n = (n & MASK_00001111) + ((n>>4) & MASK_00001111);
	return n % 255;
}

int hacker_popcnt(uint32_t n)
{
	n -= (n>>1) & 0x55555555;
	n  = (n & 0x33333333) + ((n>>2) & 0x33333333);
	n  = ((n>>4) + n) & 0x0F0F0F0F;
	n += n>>8;
	n += n>>16;
	return n & 0x0000003F;
}

/*
HAKMEM Popcount

Consider a 3 bit number as being
    4a+2b+c
if we shift it right 1 bit, we have
    2a+b
subtracting this from the original gives
    2a+b+c
if we shift the original 2 bits right we get
    a
and so with another subtraction we have
    a+b+c
which is the number of bits in the original number.

Suitable masking  allows the sums of  the octal digits  in a 32 bit  number to
appear in  each octal digit.  This  isn't much help  unless we can get  all of
them summed together.   This can be done by modulo  arithmetic (sum the digits
in a number by  molulo the base of the number minus  one) the old "casting out
nines" trick  they taught  in school before  calculators were  invented.  Now,
using mod 7 wont help us, because our number will very likely have more than 7
bits set.   So add  the octal digits  together to  get base64 digits,  and use
modulo 63.   (Those of you  with 64  bit machines need  to add 3  octal digits
together to get base512 digits, and use mod 511.)

This is HACKMEM 169, as used in X11 sources.
Source: MIT AI Lab memo, late 1970's.
*/
int hakmem_popcnt(uint32_t n)
{
	uint32_t tmp = n - ((n>>1)&033333333333) - ((n>>2)&011111111111);
	return ((tmp+(tmp>>3)) & 030707070707) % 63;
}

int assembly_popcnt(uint32_t n)
{
/*
	asm("popcnt %0,%%eax"::"r"(n));  // Intel style
	__asm popcnt eax,n;              // AT&T style
	
	The two instructions above are functionally equivalent, and both will 
	generate warning "no return statement" if you enable all the warnings.
	A caveat applies here: Don't clobber your registers!

	What, unfamiliar with inline assembly code?
	It's time to get your hands dirty.
	http://msdn.microsoft.com/en-us/library/4ks26t93(v=vs.110).aspx
	http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
*/
#ifdef _MSC_VER  // use Intel style assembly
	__asm popcnt eax,n;
//	The function does return a value in EAX

#elif __GNUC__  // use AT&T style assembly
	register int result; // Hey, it's my first time to use the keyword register!
	asm("popcnt %1,%0":"=r"(result):"r"(n)); // probably generates "popcnt eax,eax"
	return result;

#else
#	error "other POPCNT implementations go here"
#endif
}

int main()
{
#if !defined(USE_MACRO)
	// generate the table algorithmically
	for(size_t i = 1; i < TBL_LEN; ++i)
		table[i] = table[i>>1] + (i&1);
#endif

	typedef int (*FUNC_POPCNT)(uint32_t);

	const struct Pair
	{
		FUNC_POPCNT pfunc;
		const char* name;
	} METHOD[] =
	{
#define ELEMENT(n) {(n), #n}
		ELEMENT(iterated_popcnt),
		ELEMENT(  sparse_popcnt),
		ELEMENT(   dense_popcnt),
		ELEMENT(  lookup_popcnt),
		ELEMENT(parallel_popcnt),
		ELEMENT(   nifty_popcnt),
		ELEMENT(  hacker_popcnt),
		ELEMENT(  hakmem_popcnt),
		ELEMENT(assembly_popcnt)
#undef ELEMENT
	};
	
	constexpr size_t methodCount = sizeof(METHOD) / sizeof(METHOD[0]);
	for(size_t i = 0; i < methodCount; ++i)
	{
		std::time_t start = std::clock();
		const uint32_t N = 0x10000000;  // 0xDEADBEAF, can be tweaked
		for(uint32_t j = 0; j < N; ++j)
			METHOD[i].pfunc(j);

		std::time_t stop = std::clock();
		double time = static_cast<double>(stop - start) / CLOCKS_PER_SEC / N;
		printf("%zu. method %15s uses %Gs\n", i, METHOD[i].name, time);
	}
	
	return 0;
}
