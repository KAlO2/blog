#include <cassert>
#include <cstring>
#include <ctime>
#include <iostream>
#include <cmath>

#include "Sudoku.h"

/*
	Use this command to compile, your compiler needs to support C++11 syntax:
		gcc Sudoku.cpp SudokuSolver.cpp -o sudoku -O3 -Wall -lstdc++
	
	Here are some useful links for further reading.
	http://en.wikipedia.org/wiki/Exact_cover_problem
	http://www.sudokuessentials.com/
	https://metacpan.org/pod/release/WYANT/Games-Sudoku-General-0.010/lib/Games/Sudoku/General.pm
	https://github.com/dimitri/sudoku
*/

// 0: input data comes from commandline. use it in release mode.
// 1: some sudoku examples, input data hard coded in program. use it in debug mode.
#define COMMANDLINE_INPUT 0

#if COMMANDLINE_INPUT

void usage()
{
	const char* PROGRAM = "sudoku";
	
	std::cout << "usage: " << PROGRAM << " rank state [block] [placeholder]" << R"(
  rank :       the sudoku's size, usually it's 9.
  state:       initial state, row-major matrix, ranges from 1 to rank, unfilled cell will be 0 if no placeholder is set.
  block:       sudoku's block partition, row-major matrix, ranges from 1 to rank, optional for regular sudoku.
  placeholder: unfilled cell's character.
)";
}

/*
 * std::sqrt takes a float value as input, output a float value. and libm must linked to the final
 * program. That's why we need an integer version of square root.
 *
 * @see https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_(base_2)
 */
static int32_t isqrt(int32_t n)
{
	assert(n >= 0);
	
	int32_t res = 0;
	int32_t bit = 1 << 30;

	// "bit" starts at the highest power of four <= the argument.
	while (bit > n)
		bit >>= 2;
		
	while(bit != 0)
	{
		if(n >= res + bit)
		{
			n -= res + bit;
			res = (res >> 1) + bit;
		}
		else
			res >>= 1;
		bit >>= 2;
	}
	return res;
}

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		usage();
		return 0;
	}

	int8_t rank = static_cast<int8_t>(std::stoi(argv[1]));
	if(rank <= 1)
	{
		std::cerr << "invalid rank size: " << rank << '\n';
		return -1;
	}
	
	const char* state = argv[2];
	int32_t stateLength = std::strlen(state);
	const int32_t rankSquared = rank * rank;
	if(stateLength < rankSquared)
	{
		std::cerr << "invalid state length: " << stateLength << ", needs " << rankSquared << '\n';
		return -2;
	}
	const char* block;
	char placeholder = argc > 5 ? argv[5][0] : '0';
	
	std::vector<char> blockPartition;
	if(argc > 4)
	{
		block = argv[3];
		int32_t blockLength = std::strlen(block);
		if(blockLength < rankSquared)
		{
			std::cerr << "invalid block length: " << blockLength << ", needs " << rankSquared << '\n';
			return -2;
		}
	}
	else
	{
		int32_t blockSize = isqrt(rank);
		if(blockSize * blockSize != rank)
		{
			std::cout << "not a regular sudoku, needs a block partition table" << '\n';
			return -3;
		}
		
		blockPartition.resize(rankSquared);
		for(int32_t position = 0; position < rankSquared; ++position)
		{
			int32_t row = position / rank;
			int32_t column = position % rank;
			int32_t index = 1 + row / blockSize * blockSize + column / blockSize;
			blockPartition[position] = Sudoku::letter(index);
		}
		block = blockPartition.data();
	}
	
#else

int main()
{

	constexpr int8_t rank = 9;
	// 9 digits a row group, I put 9 row groups on one line to make it more compact.
	const char* state =
/*
		"005004000" "000060090" "300000007" "000040000" "008000400" "541000009" "200000003" "007400000" "000003000";
		"927000100" "000090003" "060800070" "604901000" "103020805" "000503906" "080006090" "400070000" "009000627";
*/
		// http://www.sudokuessentials.com/x-wing.html
		// http://www.sudokuessentials.com/support-files/sudoku-very-hard-1.pdf
		"030480609" "000027000" "800300000" "019000000" "780002093" "000004870" "000005006" "000130000" "902048010";
/*
		// http://www.sudokuessentials.com/support-files/sudoku-very-hard-5.pdf
		"050006007" "004000005" "000490610" "007004001" "082000760" "500800900" "096038000" "300000100" "700500030";

		// http://www.sudokusnake.com/finnedxwings.php
		"506080007" "004500860" "180006040" "050800900" "200000008" "008002070" "070200056" "062005300" "905630700";

		// A Sudoku with 17 clues.
		"000000010" "000002003" "000400000" "000000500" "401600000" "007100000" "050000200" "000080040" "030910000";
	
		// A Sudoku with 17 clues and diagonal symmetry
		"000000001" "000000023" "004005000" "000100000" "000030600" "007000580" "000067000" "010004000" "520000000";
		
		// A Sudoku with 18 clues and orthogonal symmetry
		"000000000" "090010030" "006020700" "000304000" "210000098" "000000000" "002506400" "080000010" "000000000";

		// https://en.wikipedia.org/wiki/Mathematics_of_Sudoku
		// A 24-clue automorphic Sudoku with translational symmetry.
		"100200300" "200300400" "300400500" "400500600" "000000000" "003004005" "004005006" "005006007" "006007008";
*/
	const char* block = 
		"111222333" "111222333" "111222333" "444555666" "444555666" "444555666" "777888999" "777888999" "777888999";

/*
	// https://www.wechall.net/challenge/sudoku1/index.php
	constexpr int8_t rank = 13;  // 123456789abcd
	const char* state = 
		"0a00d30070090"
		"4000000c07b00"
		"600d00c030800"
		"0bc2004000000"
		"0040000000000"
		"0705803000000"
		"05b0002000000"
		"0300000502008"
		"0000a00045000"
		"0c0000000d030"
		"b000090000000"
		"040002d080000"
		"000056a000000";
	
	const char* block = 
		"1122223333444"
		"1112223334444"
		"1111222344444"
		"1112223334555"
		"6166773835555"
		"6667778885555"
		"6667778885995"
		"6667788889999"
		"6aab7788ddd99"
		"aaabb7ccddd99"
		"aabbbbccccdd9"
		"aaabbbcccdd99"
		"aaabbbccccddd";
*/
	char placeholder = '0';

#endif

	try
	{
		Sudoku sudoku(rank, state, block, placeholder);
		std::cout << "initial state:" << '\n'
				<< sudoku.toString() << '\n';
		
		std::time_t start = std::clock();
		sudoku.solve();  // sudoku.backtrack(); is not recommended since it's time-consuming.
		std::time_t stop = std::clock();
		double elapsedTime = static_cast<double>(stop - start) / CLOCKS_PER_SEC;
		std::cout << "solver uses " << elapsedTime << 's' << '\n';
		
		std::cout << "final state:" << '\n'
				<< sudoku.toString() << '\n';
		std::cout << "answer: " << sudoku.toString(false/* lineByLine */) << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}
	
	return 0;
}
