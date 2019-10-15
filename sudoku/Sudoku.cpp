#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <unordered_map>

#include "Sudoku.h"

#if __cplusplus < 201703L  // static constexpr member variable declaration implies inline since C++17
constexpr int32_t Sudoku::INVALID_POSTION;
constexpr int8_t Sudoku::INVALID_NUMBER;
#endif

const char* Sudoku::GROUP_TEXT[4] = {"none", "row", "column", "block"};

template <typename T>
static inline bool removeElement(std::vector<T>& array, const T& element)
{
	auto it = std::find(array.begin(), array.end(), element);
	if(it != array.end())
	{
		array.erase(it);
		return true;
	}
	
	return false;
}

int8_t Sudoku::number(char letter)
{
	int8_t value;
	if('0' <= letter && letter <= '9')  // '0' is the placeholder for unfilled cells.
		value = letter - '0';
	else if('a' <= letter && letter <= 'z')
		value = letter - 'a' + 10;
	else if('A' <= letter&& letter <= 'Z')
		value = letter - 'A' + 10;
	else
	{
		value = '?';
		assert(false);  // You should not be here.
	}
	
	return value;
}

char Sudoku::letter(int8_t number)
{
	if(0 <= number && number < 10)
		return '0' + number;
	else if(10 <= number && number < RANK_MAX)
		return 'a' - 10 + number;
	else
	{
		assert(false);  // You should not be here.
		return '?';
	}
}

std::vector<int8_t> Sudoku::parse(const char* letters, int32_t length)
{
	assert(letters && length >= 0);
	
	std::vector<int8_t> array(length);
	for(int32_t i = 0; i < length; ++i)
		array[i] = number(letters[i]);
	
	return array;
}

std::vector<int8_t> Sudoku::parse(const char* letters, int32_t length, char placeholder)
{
	assert(letters && length >= 0);
	assert(placeholder == '0' || placeholder == ' ' || placeholder == '*' || placeholder == '.');
	std::vector<int8_t> array(length);
	
	for(int32_t i = 0; i < length; ++i)
	{
		char letter = letters[i];
		if(letter == ' ' || letter == '*' || letter == '.')
			letter = '0';
		
		array[i] = number(letter);
	}
	
	return array;
}

Sudoku::Sudoku(int8_t rank, const char* states, const char* blocks, char placeholder/* = '0'*/) noexcept(false):
		rank(rank),
		initialState(placeholder == '0'? parse(states, rank * rank) : parse(states, rank * rank, placeholder)),
		blockIndices(placeholder == '0'? parse(blocks, rank * rank) : parse(blocks, rank * rank, placeholder)),
		field(rank * rank, INVALID_NUMBER),
		map(rank * rank, INVALID_POSTION)
{
	assert(0 < rank && rank <= RANK_MAX);
	if(rank <= 0 || rank > RANK_MAX)
		throw std::invalid_argument("invalid rank");
	
	if(!isBlockPartitionValid())
		throw std::invalid_argument("invalid block partition");
	
	// initialState and blockIndices data are initialized, go to field.
	std::copy(initialState.begin(), initialState.end(), field.begin());
	validate(field);

//	blankCandidates.reserve(blankCount);  // changed from std::unordered_map to std::map, useless.
	blankBlocks.resize(1 + rank);  // [0] is unused here.
	for(int32_t position = 0; position < rank * rank; ++position)
	{
		const int8_t& number = field[position];
		const int8_t& blockIndex = blockIndices[position];
		if(number != INVALID_NUMBER)
			setMapPosition(blockIndex, number, position);
		else
			blankBlocks[blockIndex].emplace_back(position);
	}
	
	for(int32_t position = 0; position < rank * rank; ++position)
	{
		if(field[position] != INVALID_NUMBER)
			continue;
		
		std::vector<int8_t> candidates;
		candidates.reserve(rank);  // at most rank
		for(int8_t n = 1; n <= rank; ++n)
			if(isSafe(position, n))
				candidates.emplace_back(n);
		
		blankCandidates.emplace(position, candidates);
	}
}

void Sudoku::validate(const std::vector<int8_t>& state) const noexcept(false)
{
	bool flags[RANK_MAX];
	
#define FAST_FAIL(index, groupText, groupId)               \
	const int8_t& number = state[position];                \
	if(number == INVALID_NUMBER)                           \
		continue;                                          \
	                                                       \
	if(!flags[index])                                      \
		flags[index] = true;                               \
	else                                                   \
	{                                                      \
		std::ostringstream os;                             \
		os << "found duplicate value "                     \
				<< '\'' << letter(number) << '\''          \
				<< " on " << groupText << ' ' << groupId;  \
		throw std::invalid_argument(os.str());             \
	}                                                      \
	
	// check whether ROW group conflicts
	for(int8_t r = 0; r < rank; ++r)
	{
		int32_t base = r * rank;
		std::fill(flags, flags + RANK_MAX, false);
		for(int8_t c = 0; c < rank; ++c)
		{
			int32_t position = base + c;
			FAST_FAIL(c, GROUP_TEXT[ROW], int16_t(r));
		}
	}
	
	// check whether COLUMN group conflicts
	for(int8_t c = 0; c < rank; ++c)
	{
		std::fill(flags, flags + RANK_MAX, false);
		for(int8_t r = 0; r < rank; ++r)
		{
			int32_t position = r * rank + c;
			FAST_FAIL(r, GROUP_TEXT[COLUMN], int16_t(c));
		}
	}

	// check whether BLOCK group conflicts
	for(int8_t b = 1; b <= rank; ++b)
	{
		std::fill(flags, flags + RANK_MAX, false);
		for(int32_t position = 0; position < rank * rank; ++position)
		{
			const int8_t& blockIndex = blockIndices[position];
			if(blockIndex != b)
				continue;
			
			// number start from 1.
			FAST_FAIL(state[position], GROUP_TEXT[BLOCK], int16_t(b));
		}
	}
	
#undef FAST_FAIL
}

bool Sudoku::isBlockPartitionValid() const
{
	std::vector<int8_t> accumulator(1 + rank);
	for(int32_t i = 0; i < rank * rank; ++i)
	{
		int8_t index = blockIndices[i];
		if(index <= 0 || index > rank)
			return false;
		
		++accumulator[index];
	}
	
//	assert(accumulator[0] == 0);  // blocks are one base indexed
	for(int8_t i = 1; i <= rank; ++i)
		if(accumulator[i] != rank)
			return false;
	
	return true;
}

int8_t Sudoku::getRank() const
{
	return rank;
}

int32_t Sudoku::getMapPosition(int8_t blockIndex, int8_t number) const
{
	assert(0 < blockIndex && blockIndex <= rank);
	assert(0 < number && number <= rank);
	return map[(blockIndex - 1) * rank + (number - 1)];
}

void Sudoku::setMapPosition(int8_t blockIndex, int8_t number, int32_t position)
{
	assert(0 < blockIndex && blockIndex <= rank);
	assert(0 < number && number <= rank);
	assert(0 <= position && position < rank * rank);
	map[(blockIndex - 1) * rank + (number - 1)] = position;
}

void Sudoku::setNumber(int32_t position, int8_t number)
{
	assert(0 <= position && position < rank * rank);
	assert(0 <= number && number <= rank);
	field[position] = number;
	assert(isSafe());
	
	updateNumber(position, number);
}

int8_t Sudoku::getNumber(int32_t position) const
{
	assert(0 <= position && position < rank * rank);
	return field[position];
}

void Sudoku::setNumber(int8_t row, int8_t column, int8_t number)
{
	assert(0 <= row && row < rank);
	assert(0 <= column && column < rank);
	assert(0 <= number && number <= rank);
	int32_t position = row * rank + column;
	field[position] = number;
	assert(isSafe());
	
	updateNumber(position, number);
}

int8_t Sudoku::getNumber(int8_t row, int8_t column) const
{
	assert(0 <= row && row < rank);
	assert(0 <= column && column < rank);
	return field[row * rank + column];
}

void Sudoku::updateNumber(int32_t position, int8_t number)
{
	assert(0 <= position && position < rank * rank);
	assert(0 < number && number <= rank);
	
	int8_t row    = position / rank;
	int8_t column = position % rank;
	
	// remove possibility of value in the same row
	for(int8_t r = 0; r < rank; ++r)
	{
		int32_t position = r * rank + column;
		if(r == row || field[position] != INVALID_NUMBER)
			continue;
		
		removeCandidate(position, number);
	}
	
	// remove possibility of value in the same column
	int32_t base = row * rank;
	for(int8_t c = 0; c < rank; ++c)
	{
		int32_t position = base + c;
		if(c == column || field[position] != INVALID_NUMBER)
			continue;
		
		removeCandidate(position, number);
	}
	
	// remove possibility of value in the same block.
	const int8_t& blockIndex = blockIndices[position];
	for(std::pair<const int32_t, std::vector<int8_t>>& blank: blankCandidates)
	{
		const int32_t& position = blank.first;
		if(blockIndices[position] == blockIndex)
		{
			std::vector<int8_t>& candidates = blank.second;
			removeElement(candidates, number);
		}
	}
	
	blankCandidates.erase(position);
	removeElement(blankBlocks[blockIndex], position);
	setMapPosition(blockIndices[position], number, position);
}

bool Sudoku::isSafe() const
{
	bool flags[RANK_MAX];
#define FAST_FAIL(index)                       \
	if(getNumber(position) != INVALID_NUMBER)  \
	{                                          \
		if(!flags[index])                      \
			flags[index] = true;               \
		else                                   \
			return false;                      \
	}                                          \
	
	// check whether ROW group conflicts
	for(int8_t r = 0; r < rank; ++r)
	{
		int32_t base = r * rank;
		std::fill(flags, flags + RANK_MAX, false);
		for(int8_t c = 0; c < rank; ++c)
		{
			int32_t position = base + c;
			FAST_FAIL(c);
		}
	}

	// check whether COLUMN group conflicts
	for(int8_t c = 0; c < rank; ++c)
	{
		std::fill(flags, flags + RANK_MAX, false);
		for(int8_t r = 0; r < rank; ++r)
		{
			int32_t position = r * rank + c;
			FAST_FAIL(r);
		}
	}
	
	// check whether BLOCK group conflicts
	for(int8_t b = 1; b <= rank; ++b)
	{
		std::fill(flags, flags + RANK_MAX, false);
		for(int32_t position = 0; position < rank * rank; ++position)
		{
			if(blockIndices[position] != b)
				continue;
			
			int8_t number = getNumber(position);
			if(number == INVALID_NUMBER)
				continue;
			
			FAST_FAIL(number - 1);
		}
	}
	
#undef FAST_FAIL
	return true;
}

bool Sudoku::isSafe(int8_t row, int8_t column, int8_t number) const
{
	assert(0 <= row && row < rank);
	assert(0 <= column && column < rank);
	assert(0 < number && number <= rank);
	
	int32_t position = row * rank + column;
	if(getNumber(position) != INVALID_NUMBER)  // this seat is already taken.
		return false;
	
	// check the row line
	for(int32_t c = 0; c < rank; ++c)
		if(getNumber(row, c) == number)  // getNumber(row, column) == 0, number > 0
			return false;
			
	// check the column line
	for(int32_t r = 0; r < rank; ++r)
		if(getNumber(r, column) == number)  // getNumber(row, column) == 0, number > 0
			return false;
	
	// check the block that belongs.
	int8_t blockIndex = blockIndices[position];
	if(getMapPosition(blockIndex, number) != INVALID_POSTION)
		return false;
	
	return true;
}

bool Sudoku::isSafe(int32_t position, int8_t number) const
{
	assert(0 <= position && position < rank * rank);
	assert(0 < number && number <= rank);
	
	if(getNumber(position) != INVALID_NUMBER)  // this seat is already taken.
		return false;
	
	// check the row line
	for(int32_t p = position / rank * rank, end = p + rank; p < end; ++p)
		if(getNumber(p) == number)  // getNumber(row, column) == 0, number > 0
			return false;
	
	// check each column,
	for(int32_t p = position % rank, end = rank * rank; p < end; p += rank)
		if(getNumber(p) == number)  // getNumber(row, column) == 0, number > 0
			return false;
	
	// check the block that belongs.
	int8_t blockIndex = blockIndices[position];
	if(getMapPosition(blockIndex, number) != INVALID_POSTION)
		return false;
	
	return true;
}

std::vector<std::pair<int32_t, int8_t>> Sudoku::findNakedSingle() const
{
	std::vector<std::pair<int32_t, int8_t>> steps;
	
	for(const std::pair<int32_t, std::vector<int8_t>>& blank: blankCandidates)
	{
		const std::vector<int8_t>& candidates = blank.second;
		if(candidates.size() == 1)
		{
			const int32_t& position = blank.first;
			const int8_t& number = *candidates.begin();
			steps.emplace_back(std::make_pair(position, number));
		}
	}
	
	return steps;
}

std::vector<std::pair<int32_t, int8_t>> Sudoku::findHiddenSingle() const
{
	std::vector<std::pair<int32_t, int8_t>> steps;

	int32_t positions[1 + RANK_MAX];
	int8_t histogram[1 + RANK_MAX];
	
	auto reset = [&positions, &histogram]()
	{
		std::fill(positions, positions + (1 + RANK_MAX), INVALID_POSTION);
		std::fill(histogram, histogram + (1 + RANK_MAX), 0);
	};
	
	const auto& field = this->field;
	const auto& blankCandidates = this->blankCandidates;
	auto handlePosition = [&positions, &histogram, &field, &blankCandidates](int32_t position)
	{
		if(field[position] != INVALID_NUMBER)
			return;
			
		const std::vector<int8_t>& candidates = blankCandidates.at(position);
		for(const int8_t& candidate: candidates)
		{
			++histogram[candidate];
			positions[candidate] = position;
		}
	};
	
	const auto& rank = this->rank;
	auto addSingle = [&positions, &histogram, &rank, &steps](Group group, int8_t index)
	{
		for(int8_t n = 1; n <= rank; ++n)
		{
			if(histogram[n] != 1)
				continue;
			
			const int32_t& position = positions[n];
			steps.emplace_back(std::make_pair(position, n));
			std::cout << GROUP_TEXT[group] << ' ' << int16_t(index)
					<< " has hidden single candidate " << '\'' << letter(n) << '\''
					<< " at position " << '(' << position / rank << ", " << position % rank << ')' << '\n';
		}
	};
	
	// for each row
	for(int8_t r = 0; r < rank; ++r)
	{
		reset();
		
		const int32_t base = r * rank;
		for(int8_t c = 0; c < rank; ++c)
		{
			int32_t position = base + c;
			handlePosition(position);
		}
		
		addSingle(ROW, r);
	}
	
	// for each column
	for(int8_t c = 0; c < rank; ++c)
	{
		reset();
		for(int8_t r = 0; r < rank; ++r)
		{
			int32_t position = r * rank + c;
			handlePosition(position);
		}
		
		addSingle(COLUMN, c);
	}
	
	// for each block
	for(int8_t b = 1; b <= rank; ++b)
	{
		const std::vector<int32_t>& blankBlock = blankBlocks[b];
		if(blankBlock.empty())  // this block is full filled.
			continue;
		
		reset();
		for(const int32_t position: blankBlock)
			handlePosition(position);
		addSingle(BLOCK, b);
	}

	return steps;
}

const std::map<int32_t, std::vector<int8_t>>& Sudoku::getBlankCandidates() const
{
	return blankCandidates;
}

bool Sudoku::removeCandidate(int32_t position, int8_t number)
{
	assert(0 <= position && position < rank * rank);
	assert(0 < number && number <= rank);
	assert(blankCandidates.find(position) != blankCandidates.end());
	
	std::vector<int8_t>& candidates = blankCandidates[position];
	return removeElement(candidates, number);
}

/*
 * A Naked Pair (also known as a Conjugate Pair) is a set of two candidate numbers sited in two 
 * cells that belong to one group in common, namely they reside in the same row, column or block. 
 * It is clear that the solution will contain those numbers in those two cells (we just don’t know 
 * which is which at this stage) and all other candidates with those numbers can be crossed out from
 * whatever group they have in common.
 */
void Sudoku::updateCandidateByNakedPair()
{
	std::vector<int32_t> values;  // naked pair candidates.

#if __cplusplus >= 201703L // structured binding is added in C++17
	for(const auto& [position, candidates]: blankCandidates)
	{
#else
	for(const std::pair<int32_t, std::vector<int8_t>>& blank: blankCandidates)
	{
		const int32_t& position = blank.first;
		const std::vector<int8_t>& candidates = blank.second;
#endif
		if(candidates.size() != 2)
			continue;
		
		int8_t row = position / rank;
		int8_t column = position % rank;
		assert(candidates[0] < candidates[1]);
		int32_t value = (column << 24) | (row << 16) | (candidates[1] << 8) | candidates[0];
//		assert(candidates[0] < candidates[1]);
		values.emplace_back(value);
	}

	std::unordered_map<int32_t, int32_t> keyValues;
	keyValues.reserve(values.size());
	
	// statistics about the paired candidates.
	for(const int32_t& value: values)
	{
		const int32_t key = value & 0x0000FFFF;  // (candidates[1], candidates[0]) pair.
		auto it = keyValues.find(key);
		if(it == keyValues.end())
			keyValues.emplace(std::make_pair(key, 1));
		else
		{
			int32_t& count = it->second;
			++count;
		}
	}
	
	// evict those paired candidates that only appears once.
	std::vector<int32_t> pairedValues;
	for(const int32_t& value: values)
	{
		const int32_t key = value & 0x0000FFFF;  // (candidates[1], candidates[0]) pair.
		auto it = keyValues.find(key);
		const int32_t& count = it->second;
		if(count >= 2)
			pairedValues.emplace_back(value);
	}
	values = std::move(pairedValues);
	
	keyValues.clear();
	for(const int32_t& value: values)
	{
		assert(value != 2);
		const int32_t key = value & 0x00FFFFFF;  // (row, candidates[1], candidates[0]) triple.
		auto it = keyValues.find(key);
		if(it == keyValues.end())
			keyValues.emplace(std::make_pair(key, value));
		else  // naked pair found
		{
			const int8_t* byte = reinterpret_cast<const int8_t*>(&value);
			int8_t candidate0 = byte[0];
			int8_t candidate1 = byte[1];
			int8_t row     = byte[2];
			int8_t column0 = byte[3];
			int8_t column1 = reinterpret_cast<const int8_t*>(&it->second)[3];
			assert(column0 != column1);
			
			int32_t base = row * rank;
			for(int8_t c = 0; c < rank; ++c)
			{
				if(c == column0 || c == column1)
					continue;
					
				int32_t position = base + c;
				if(field[position] != INVALID_NUMBER)
					continue;
				
				auto removeCandidateAndPrint = [&](int8_t number)
				{
					if(!removeCandidate(position, number))
						return;
					
					std::cout << "naked pair candidates "
							<< '{' << letter(candidate0) << ", " << letter(candidate1) << '}'
							<< " found in " << GROUP_TEXT[ROW] << ' ' << int16_t(row) << ' '
							<< GROUP_TEXT[ROW] << ' ' << int16_t(column0) << " and " << int16_t(column1)
							<< ", remove candidate " << '\'' << letter(number) << '\''
							<< " at position " << '(' << int16_t(row) << ", " << int16_t(c) << ')' << '\n';
				};
				
				removeCandidateAndPrint(candidate0);
				removeCandidateAndPrint(candidate1);
			};
		}
	}
	
	keyValues.clear();
	for(const int32_t& value: values)
	{
		const int32_t key = value & 0xFF00FFFF;  // (column, candidates[1], candidates[0]) triple.
		auto it = keyValues.find(key);
		if(it == keyValues.end())
			keyValues.emplace(std::make_pair(key, value));
		else  // naked pair found
		{
			const int8_t* byte = reinterpret_cast<const int8_t*>(&value);
			int8_t candidate0 = byte[0];
			int8_t candidate1 = byte[1];
			int8_t row0    = byte[2];
			int8_t row1    = reinterpret_cast<const int8_t*>(&it->second)[2];
			int8_t column  = byte[3];
			assert(row0 != row1);

			for(int8_t r = 0; r < rank; ++r)
			{
				if(r == row0 || r == row1)
					continue;
					
				int32_t position = r * rank + column;
				if(field[position] != INVALID_NUMBER)
					continue;
				
				auto removeCandidateAndPrint = [&](int8_t number)
				{
					if(!removeCandidate(position, number))
						return;
					
					std::cout << "naked pair candidates "
							<< '{' << letter(candidate0) << ", " << letter(candidate1) << '}'
							<< " found in " << GROUP_TEXT[COLUMN] << ' ' << int16_t(column) << ' '
							<< GROUP_TEXT[ROW] << ' ' << int16_t(row0) << " and " << int16_t(row1)
							<< ", remove candidate " << '\'' << letter(number) << '\''
							<< " at position " << '(' << int16_t(r) << ", " << int16_t(column) << ')' << '\n';
				};
				
				removeCandidateAndPrint(candidate0);
				removeCandidateAndPrint(candidate1);
			};
		}
	}
	
	keyValues.clear();
	for(const int32_t& value: values)  // high <- column | row | candidate1 | candidate0 -> low
	{
		int32_t row = value >> 16, column = value >> 24;
		int32_t position = row * rank + column;
		int8_t blockIndex = blockIndices[position];
		int8_t number = field[position];
		int32_t newValue = (number << 24) | (blockIndex << 16) | (value & 0x0000FFFF);
		
		const int32_t key = newValue & 0x00FFFFFF;  // (block, candidate1, candidate0) triple.
		auto it = keyValues.find(key);
		if(it == keyValues.end())
			keyValues.emplace(std::make_pair(key, newValue));
		else  // naked pair found
		{
			const int8_t* byte = reinterpret_cast<const int8_t*>(&value);
			int8_t candidate0 = byte[0];
			int8_t candidate1 = byte[1];
			int8_t blockIndex = byte[2];
			int8_t number0 = byte[3];
			int8_t number1 = reinterpret_cast<const int8_t*>(&it->second)[3];
//			assert(number0 != number1);

			const std::vector<int32_t>& blockGroup = blankBlocks[blockIndex];
			for(const int32_t& position: blockGroup)
			{
				const int8_t& number = field[position];
				if(number == INVALID_NUMBER || number == number0 || number == number1)
					continue;
				
				auto removeCandidateAndPrint = [&](int8_t number)
				{
					if(!removeCandidate(position, number))
						return;
					
					std::cout << "naked pair candidates "
							<< '{' << letter(candidate0) << ", " << letter(candidate1) << '}'
							<< " found in " << GROUP_TEXT[BLOCK] << ' ' << int16_t(blockIndex)
							<< ", remove candidate " << '\'' << letter(number) << '\''
							<< " at position " << '(' << position / rank << ", " << position % rank << ')' << '\n';
				};
				
				removeCandidateAndPrint(candidate0);
				removeCandidateAndPrint(candidate1);
			};
		}
	}
}

void Sudoku::updateCandidateByHiddenPair()
{
	// TODO:
}

/*
 * We can certainly extend Naked Pairs to Naked Triples. Any three cells in the same group that 
 * contain the same three candidate numbers will be a Naked Triple. The rest of the group can be 
 * crossed out any of those numbers.
 *
 * But a Naked Triple is much more versatile than this rule implies. In fact it is not necessary for
 * there to be three candidates in each cell. As long as there are in total three candidates in 
 * three cells. Obviously we’re not going to apply this rule to single candidate cells – since they 
 * are solved, so the possible combinations are as follows:
 * (123) (123) (123)  - {3/3/3} (in terms of candidates per cell)
 * (123) (123) (12)   - {3/3/2} (or some combination thereof)
 * (123) (12)  (23)   - {3/2/2}
 * (12)  (23)  (13)   - {2/2/2}
 */
void Sudoku::updateCandidateByNakedTriple()
{
	std::vector<int32_t> positions;

#if __cplusplus >= 201703L // structured binding is added in C++17
	for(const auto& [position, candidates]: blankCandidates)
	{
#else
	for(const std::pair<int32_t, std::vector<int8_t>>& blank: blankCandidates)
	{
		const int32_t& position = blank.first;
		const std::vector<int8_t>& candidates = blank.second;
#endif
		int8_t size = static_cast<int8_t>(candidates.size());  // at most rank
		if(size < 2 || size > 3)
			continue;
		
		positions.emplace_back(position);
	}
	
	// For three given position, judge whether they forms a group.
	const auto& rank = this->rank;
	const auto& blockIndices = this->blockIndices;
	auto getGroup = [&rank, &blockIndices](int32_t position0, int32_t position1, int32_t position2) -> int16_t
	{
		assert(position0 != position1 && position1 != position2);
		int8_t bytes[4] = {0, int8_t(position0 / rank), int8_t(position0 % rank), blockIndices[position0]};
		int8_t& r = bytes[ROW];
		int8_t& c = bytes[COLUMN];
		int8_t& b = bytes[BLOCK];
		
		Group group;
		if(position1 / rank == r && position2 / rank == r)
			group = ROW;
		else if(position1 % rank == c && position2 % rank == c)
			group = COLUMN;
		else if(blockIndices[position1] == b && blockIndices[position2] == b)
			group = BLOCK;
		else
			group = NONE;
		
		return bytes[group] << 8 | group;
	};
	
	int8_t size = static_cast<int8_t>(positions.size());
	for(int8_t i = 0;     i < size; ++i)
	for(int8_t j = i + 1; j < size; ++j)
	for(int8_t k = j + 1; k < size; ++k)
	{
		const int32_t& position0 = positions[i];
		const int32_t& position1 = positions[j];
		const int32_t& position2 = positions[k];
		
		int16_t groupInfo = getGroup(position0, position1, position2);
		Group group = static_cast<Group>(groupInfo & 0xFF);
		int8_t groupIndex = groupInfo >> 8;
		if(group == NONE)
			continue;
		
		std::set<int8_t> candidates;
//		candidates.reserve(rank);
		for(const int8_t& candidate: blankCandidates[position0])
			candidates.emplace(candidate);
		for(const int8_t& candidate: blankCandidates[position1])
			candidates.emplace(candidate);
		for(const int8_t& candidate: blankCandidates[position2])
			candidates.emplace(candidate);
		
		if(candidates.size() != 3)
			continue;
		
		int8_t numbers[3];  // naked triple found
		std::copy(candidates.begin(), candidates.end(), numbers);
//		sort(numbers);  // changed from std::unordered_set to std::set, std::set container is ordered.
		
		auto removeCandidateAndPrint = [&, this](int32_t position)
		{
			if(field[position] != INVALID_NUMBER)
				return;
			
			if(position == position0 || position == position1 || position == position2)
				return;
				
			for(int8_t m = 0; m < 3; ++m)
			{
				const int8_t& number = numbers[m];
				if(!removeCandidate(position, number))
					continue;
				
				std::cout << "naked triple " << '{'
						<< letter(numbers[0]) << ", " <<  letter(numbers[1]) << ", " << letter(numbers[2])
						<< '}' << " in " << GROUP_TEXT[group] << ' ' << int16_t(groupIndex)
						<< ", remove candidate " << '\'' << letter(number) << '\''
						<< " at position " << '(' << position / rank << ", " << position % rank << ')' << '\n';
			}
		};
		
		if(group == BLOCK)
		{
			const std::vector<int32_t>& blockGroup = blankBlocks.at(groupIndex);
			for(const int32_t& position: blockGroup)
				removeCandidateAndPrint(position);
		}
		else
		{
			for(int32_t l = 0; l < rank; ++l)
			{
				int32_t position = (group == ROW) ? (groupIndex * rank + l): (groupIndex + rank * l);
				removeCandidateAndPrint(position);
			}
		}
	}
}

/*
 * Check whether the candidate of one blank block forms one line(row or column), 
 * If so, extra restriction can be added to other blank blocks who intersects this line.
 */
void Sudoku::updateCandidateOutBlockOfLine()
{
	for(int8_t b = 1; b <= rank; ++b)
	{
		const std::vector<int32_t>& blockGroup = blankBlocks[b];
		if(blockGroup.size() < 2)  // at least two points form a line.
			continue;
		
		// looking for intersection value
		for(int8_t n = 1; n <= rank; ++n)
		{
			if(getMapPosition(b, n) >= 0)  // skip values that used.
				continue;

			int32_t row = 0, column = 0;
			bool sameRow = true, sameColumn = true, first = true;
			for(const int32_t& position: blockGroup)
			{
				assert(0 <= position && position < rank * rank);
				const std::vector<int8_t>& candidates = blankCandidates.at(position);
				auto it = std::find(candidates.begin(), candidates.end(), n);
				if(it != candidates.end())
				{
					if(first)
					{
						row    = position / rank;
						column = position % rank;
						first  = false;
					}
					else
					{
						if(sameRow && position / rank != row)
							sameRow = false;
			
						if(sameColumn && position % rank != column)
							sameColumn = false;
			
						if(!sameRow && !sameColumn)  // [[likely]], fast quit
							break;
					}
				}
			}
			
			// Apparently, they can be the same row and the same column at the same time.
			// If they never enter the for loop above, they will stay inital state (true).
			if(sameRow == sameColumn)
				continue;
			
			for(int8_t k = 0; k < rank; ++k)
			{
				int32_t position = sameRow? (row * rank + k): (k * rank + column);
				if(field[position] == INVALID_NUMBER && blockIndices[position] != b)
				{
					if(!removeCandidate(position, n))
						continue;
					
					std::cout << "in block " << int16_t(b) << ", candidate value "
							<< '\'' << letter(n) << '\'' << " happens to be in the same ";
					if(sameRow)
						std::cout << GROUP_TEXT[ROW] << ' ' << int16_t(row);
					else
						std::cout << GROUP_TEXT[COLUMN] << ' ' << int16_t(column);
					
					std::cout << ", remove candidate of "
							<< GROUP_TEXT[sameRow? COLUMN:ROW] << ' ' << int32_t(k) << '\n';
				}
			}
		}
	}
}

/*
 * Since that one row or column must fill all the numbers, and if one number's candidate are in one 
 * single block, then other blank of this block should forbid this number.
 *
 * for example, in a 9x9 size sudoku, number N must shown in the row below, Here, X means it cannot 
 * be N, + means it's possible to be N.
 * | X X X | + + + | X X X |    <------ the line that should have the number N.
 *         | + + + |            <----  now the + for that number can be crossed out.
 *         | + + + |            <----- ditto.
 */
void Sudoku::updateCandidateInBlockOutOfLine(bool horizontal)
{
	for(int8_t i = 0; i <  rank; ++i)  // i is row if horizontal else column
	for(int8_t n = 1; n <= rank; ++n)
	{
		int8_t blockIndex = 0, j = 0;
		for(; j < rank; ++j)  // j is column if horizontal else row
		{
			int32_t position = horizontal? (i * rank + j): (i + rank * j);
			if(field[position] != INVALID_NUMBER)
				continue;
			
			const std::vector<int8_t>& candidates = blankCandidates.at(position);
			auto it = std::find(candidates.begin(), candidates.end(), n);
			if(it != candidates.end())
			{
				const int8_t& index = blockIndices[position];
				if(blockIndex != index)
				{
					if(blockIndex != 0)  // when first time in, blockIndex == 0
					{
						blockIndex = 0;
						break;
					}
					blockIndex = index;
				}
			}
		}
		
		if(blockIndex > 0)  // same block index
		{
			for(const std::pair<int32_t, std::vector<int8_t>>& blank: blankCandidates)
			{
				const int32_t& position = blank.first;
				int8_t lineIndex = horizontal? position / rank : position % rank;
				if(blockIndices[position] == blockIndex && lineIndex != i)
				{
					if(!removeCandidate(position, n))
						return;
					
					std::cout << GROUP_TEXT[horizontal?ROW:COLUMN] << ' ' << int16_t(i)
							<< " must feed letter " << '\'' << letter(n) << '\''
							<< " in block " << int16_t(blockIndex) << ", so remove candidate " << '\'' << letter(n) << '\''
							<< " at position " << '(' << position / rank << ", " << position % rank << ')' << '\n';
				}
			}
		}
	}
}

/*
 * An X-Wing pattern occurs when two rows (or two columns) each contain only two cells that hold a 
 * matching candidate. This candidate must reside in both rows and share the same two columns or 
 * vice versa.
 */
void Sudoku::updateCandidateByXWing(bool horizontal)
{
	auto project = [&](int8_t line, int8_t number, bool horizontal) -> int32_t
	{
		constexpr int8_t UNINITIALIZED = -1;
		alignas(4) int8_t lines[4] = {UNINITIALIZED, UNINITIALIZED, number, line};
		// four bytes are line0, line1, number, line.
		// if horizontal, it's column0, column1, number,    row;
		//     otherwise, it's    row0,    row1, number, column.
		
		for(int8_t i = 0; i < rank; ++i)  // i is row if horizontal else column
		{
			int32_t position = horizontal? (line * rank + i): (line + rank * i);
			if(field[position] != INVALID_NUMBER)
				continue;
			
			const std::vector<int8_t>& candidates = blankCandidates.at(position);
			if(std::find(candidates.begin(), candidates.end(), number) != candidates.end())
			{
				if(lines[0] == UNINITIALIZED)
					lines[0] = i;
				else if(lines[1] == UNINITIALIZED)
					lines[1] = i;
				else
					return 0;
			}
		}
		
		if(lines[1] == UNINITIALIZED)  // less than two seats.
			return 0;

		return *reinterpret_cast<int32_t*>(lines);
	};
	
	std::vector<int32_t> values;
	values.reserve(rank * (rank + 1) / 2);
	
	for(int8_t i = 0; i < rank; ++i)  // i is row if horizontal else column
	for(int8_t n = 1; n <= rank; ++n)
	{
		int32_t value = project(i, n, horizontal);
		if(value != 0)
			values.emplace_back(value);
	}
	
	std::unordered_map<int32_t, int32_t> keyValues;
	keyValues.reserve(values.size());
	
	for(const int32_t& value: values)
	{
		const int32_t key = value & 0x00FFFFFF;  // (line0, line1, number) triple.
		auto it = keyValues.find(key);
		if(it == keyValues.end())
			keyValues.emplace(std::make_pair(key, value));
		else  // an X-Wing found
		{
			int8_t line0 = *reinterpret_cast<const int8_t*>(&key);        // column if horizontal
			int8_t line1 = *(reinterpret_cast<const int8_t*>(&key) + 1);  // column if horizontal
			int8_t number= static_cast<int8_t>(value >> 16);
			int8_t line2 = static_cast<int8_t>(it->second >> 24);    // row if horizontal
			int8_t line3 = static_cast<int8_t>(value >> 24);         // row if horizontal
			assert(line2 != line3);
			
			auto removeCandidateAndPrint = [&](int8_t line)
			{
				for(int8_t i = 0; i < rank; ++i)
				{
					if(i == line2 || i == line3)
						continue;
					
					int32_t position = horizontal? (i * rank + line): (i + rank * line);
					if(field[position] != INVALID_NUMBER)
						continue;
					
					if(removeCandidate(position, number))
						std::cout << GROUP_TEXT[horizontal ? ROW:COLUMN] << ' ' << int16_t(line2) << " and " << int16_t(line3)
								<< " forms an X-wing about letter " << '\'' << letter(number) << '\'' << " in " 
								<< GROUP_TEXT[horizontal ? COLUMN:ROW] << ' ' << int16_t(line0) << " and " << int16_t(line1)
								<< ", remove candidate " << '\'' << letter(number) << '\''
								<< " at position " << '(' << position / rank << ", " << position % rank << ')' << '\n';
				}
			};
			
			removeCandidateAndPrint(line0);
			removeCandidateAndPrint(line1);
		}
	}
}

void Sudoku::updateCandidateInOneLine(bool horizontal)
{
	// a blank group projects to one line.
	auto project = [&](int8_t blockIndex, int8_t number, bool horizontal) -> int16_t
	{
		const std::vector<int32_t>& positions = blankBlocks[blockIndex];
		
		int8_t line1 = 0;
		bool initialized = false;
		for(const int32_t& position: positions)
		{
			const std::vector<int8_t>& candidates = blankCandidates.at(position);
			if(std::find(candidates.begin(), candidates.end(), number) != candidates.end())
			{
				int8_t line = horizontal? position / rank: position % rank;
				if(initialized)
				{
					if(line1 != line)  // more than one line, fast fail.
						return 0;
				}
				else
				{
					line1 = line;
					initialized = true;
				}
			}
		}
		
		if(!initialized)  // no line.
			return 0;
		
		return (line1 << 8) | number;
	};
	
	for(int8_t b = 1; b <= rank; ++b)
	for(int8_t n = 1; n <= rank; ++n)
	{
		if(getMapPosition(b, n) >= 0)
			continue;
		
		int16_t value = project(b, n, horizontal);
		if(value != 0)
		{
			int8_t line = reinterpret_cast<int8_t*>(&value)[1];
			auto removeCandidateAndPrint = [&](int8_t line)
			{
				for(int8_t i = 0; i < rank; ++i)
				{
					int8_t& row    = horizontal? line: i;
					int8_t& column = horizontal? i: line;
					int32_t position = row * rank + column;
					if(getNumber(position) != INVALID_NUMBER || blockIndices[position] == b)
						continue;
						
					if(removeCandidate(position, n))
						std::cout << "blank block " << int32_t(b) << " all map to "
								<< GROUP_TEXT[horizontal ? ROW:COLUMN] << ' ' << int32_t(line) << ','
								<< " remove candidate " << '\'' << letter(n) << '\'' << " at position "
								<< '(' << int32_t(row) << ", " << int32_t(column) << ')' << '\n';
				}
			};
			
			removeCandidateAndPrint(line);
		}
	}
}

/*
 * If a number in block A can be on two lines, and the same number in block B is on the same two 
 * lines, then it is positive that this number must be in the two line of block A and B, blank cells
 * of other blocks than touches the two lines can cross out this number.
 */
void Sudoku::updateCandidateBetweenTwoLines(bool horizontal)
{
	auto project = [&](int8_t blockIndex, int8_t number, bool horizontal) -> int32_t
	{
		const std::vector<int32_t>& positions = blankBlocks[blockIndex];
		
		constexpr int8_t UNINITIALIZED = -1;
		alignas(4) int8_t lines[4] = {UNINITIALIZED, UNINITIALIZED, UNINITIALIZED, 0};
		for(const int32_t& position: positions)
		{
			const std::vector<int8_t>& candidates = blankCandidates.at(position);
			if(std::find(candidates.begin(), candidates.end(), number) != candidates.end())
			{
				int8_t line = horizontal? position / rank: position % rank;
				if(lines[1] == UNINITIALIZED)
					lines[1] = line;
				else if(lines[1] != line)
				{
					if(lines[2] == UNINITIALIZED)
					{
						lines[2] = line;
						lines[0] = number;  // mark of two seats taken.
					}
					else if(lines[2] != line)
						return 0;
				}
			}
		}
		
		if(lines[0] == UNINITIALIZED)  // less than two seats.
			return 0;
		
		// project() doesn't care about order of line numbers
		if(lines[1] > lines[2])
			std::swap(lines[1], lines[2]);
		
		return *reinterpret_cast<int32_t*>(lines);
	};
	
	for(int8_t b1 = 1;      b1 <= rank; ++b1)
	for(int8_t b2 = b1 + 1; b2 <= rank; ++b2)
	for(int8_t n = 1;        n <= rank; ++n)
	{
		if(getMapPosition(b1, n) != INVALID_POSTION || getMapPosition(b2, n) != INVALID_POSTION)
			continue;
		
		int32_t value1 = project(b1, n, horizontal);
		int32_t value2 = project(b2, n, horizontal);
		if(value1 != 0 && value1 == value2)
		{
			int8_t line1 = reinterpret_cast<int8_t*>(&value1)[1];
			int8_t line2 = reinterpret_cast<int8_t*>(&value1)[2];
			
			auto removeCandidateAndPrint = [&](int8_t line)
			{
				for(int8_t i = 0; i < rank; ++i)
				{
					int8_t& row    = horizontal? line: i;
					int8_t& column = horizontal? i: line;
					int32_t position = row * rank + column;
					if(getNumber(position) != 0 || blockIndices[position] == b1 || blockIndices[position] == b2)
						continue;
						
					if(removeCandidate(position, n))
						std::cout << GROUP_TEXT[BLOCK] << ' ' << int16_t(b1) << " and " << int16_t(b2)
								<< " map to two " << GROUP_TEXT[horizontal ? ROW:COLUMN]
								<< " lines " << int16_t(line1) << " and " << int16_t(line2)
								<< ", remove candidate " << '\'' << letter(n) << '\'' << " at position "
								<< '(' << int16_t(row) << ", " << int16_t(column) << ')' << '\n';
				}
			};
			
			removeCandidateAndPrint(line1);
			removeCandidateAndPrint(line2);
		}
	}
}

void Sudoku::updateCandidateAmongThreeLines(bool horizontal)
{
	auto project = [&](int8_t blockIndex, int8_t number, bool horizontal) -> int32_t
	{
		const std::vector<int32_t>& positions = blankBlocks[blockIndex];
		
		constexpr int8_t UNINITIALIZED = -1;
		alignas(4) int8_t lines[4] = {number, UNINITIALIZED, UNINITIALIZED, UNINITIALIZED};
		bool threeSeats = false;
		for(const int32_t& position: positions)
		{
			const std::vector<int8_t>& candidates = blankCandidates.at(position);
			if(std::find(candidates.begin(), candidates.end(), number) != candidates.end())
			{
				int8_t line = horizontal? position / rank: position % rank;
				if(lines[1] == UNINITIALIZED)
					lines[1] = line;
				else if(lines[1] != line)
				{
					if(lines[2] == UNINITIALIZED)
						lines[2] = line;
					else if(lines[2] != line)
					{
						if(lines[3] == UNINITIALIZED)
						{
							lines[3] = line;
							threeSeats = true;  // mark of two seats taken.
						}
						else if(lines[3] != line)
							return 0;
					}
				}	
			}
		}
		
		if(!threeSeats)
			return 0;
		
		// project() doesn't care about order of line numbers
		if(lines[1] > lines[3])
			std::swap(lines[1], lines[3]);
		
		if(lines[1] > lines[2])
			std::swap(lines[1], lines[2]);
		
		if(lines[2] > lines[3])
			std::swap(lines[2], lines[3]);
		
		return *reinterpret_cast<int32_t*>(lines);
	};
	
	for(int8_t n = 1; n <= rank; ++n)
	for(int8_t b1 = 1; b1 <= rank; ++b1)
	{
		if(getMapPosition(b1, n) != INVALID_POSTION)
			continue;
		
		int32_t value1 = project(b1, n, horizontal);
		for(int8_t b2 = b1 + 1; b2 <= rank; ++b2)
		{
			if(getMapPosition(b2, n) != INVALID_POSTION)
				continue;
			
			int32_t value2 = project(b2, n, horizontal);
			for(int8_t b3 = b2 + 1; b3 <= rank; ++b3)
			{
				if(getMapPosition(b3, n) != INVALID_POSTION)
					continue;
				
				int32_t value3 = project(b3, n, horizontal);
				if(value1 != 0 && value1 == value2 && value1 == value3)
				{
					int8_t line1 = reinterpret_cast<int8_t*>(&value1)[1];
					int8_t line2 = reinterpret_cast<int8_t*>(&value1)[2];
					int8_t line3 = reinterpret_cast<int8_t*>(&value1)[3];
					
					auto removeCandidateAndPrint = [&](int8_t line)
					{
						for(int8_t i = 0; i < rank; ++i)
						{
							int8_t& row    = horizontal? line: i;
							int8_t& column = horizontal? i: line;
							int32_t position = row * rank + column;
							if(getNumber(position) != INVALID_NUMBER)
								continue;
								
							if(blockIndices[position] == b1
									|| blockIndices[position] == b2
									|| blockIndices[position] == b3)
								continue;
								
							if(removeCandidate(position, n))
								std::cout << GROUP_TEXT[BLOCK] << int16_t(b1) << int16_t(b2) << " and " << int16_t(b3)
										<< " map to three " << GROUP_TEXT[horizontal ? ROW:COLUMN] << " lines " 
										<< int16_t(line1) << ' ' << int16_t(line2) << " and " << int16_t(line3) << ','
										<< " remove candidate " << '\'' << letter(n) << '\''
										<< " at position " << '(' << int32_t(row) << ", " << int32_t(column) << ')' << '\n';
						}
					};
					
					removeCandidateAndPrint(line1);
					removeCandidateAndPrint(line2);
					removeCandidateAndPrint(line3);
				}
			}
		}
	}
}

void Sudoku::update()
{
	// many a strategy has used here to remove candidates.
	updateCandidateByNakedPair();
	updateCandidateByNakedTriple();

	updateCandidateByXWing(true/* horizontal */);
	updateCandidateByXWing(false/* horizontal */);
	
	updateCandidateOutBlockOfLine();
	
	updateCandidateInBlockOutOfLine(true/* horizontal */);
	updateCandidateInBlockOutOfLine(false/* horizontal */);
	
	updateCandidateInOneLine(true/* horizontal */);
	updateCandidateInOneLine(false/* horizontal */);
	
	updateCandidateBetweenTwoLines(true/* horizontal */);
	updateCandidateBetweenTwoLines(false/* horizontal */);
	
	updateCandidateAmongThreeLines(true/* horizontal */);
	updateCandidateAmongThreeLines(false/* horizontal */);
}

void Sudoku::printCurrentState() const
{
	int32_t i32 = 1;  // probably overflow
	int64_t i64 = 1;  // can overflow
	float   f32 = 1;  // may overflow
	double  f64 = 1;
	
	for(const std::pair<int32_t, std::vector<int8_t>>& blank: blankCandidates)
	{
		const int32_t& position = blank.first;
		const std::vector<int8_t> candidates = blank.second;
		std::size_t size = candidates.size();
		
		i32 *= size;
		i64 *= size;
		f32 *= size;
		f64 *= size; 
		
		int width = rank < 10 ? 1:2;
		std::cout << GROUP_TEXT[BLOCK]
				<< ' ' << std::setw(width) << int16_t(blockIndices[position]) << ": "
				<< '[' << std::setw(width) << position / rank << ']'
				<< '[' << std::setw(width) << position % rank << ']'
				<< " = " << '{';
		int8_t last = static_cast<int8_t>(candidates.size() - 1);  // at most rank
		for(int8_t i = 0; i < last; ++i)
			std::cout << letter(candidates[i]) << ", ";
		if(last >= 0)
			std::cout << letter(candidates[last]);
		std::cout << '}' << '\n';
	}
	
	std::cout << "combinatorial number:"
			<< " i32=" << i32 << ','
			<< " i64=" << i64 << ','
			<< " f32=" << f32 << ','
			<< " f64=" << f64 << '\n';

	for(int8_t n = 1; n <= rank; ++n)
	{
		int32_t count = 0;
		for(int8_t b = 1; b <= rank; ++b)
			if(getMapPosition(b, n) >= 0)
				++count;
		std::cout << "letter " << '\'' << letter(n) << '\''
				<< " has shown " << count << " time(s) " << '\n';
	}
}

void Sudoku::backtrack(int32_t position)
{
	// looking for next blank cell
	while(position < rank * rank && getNumber(position) != INVALID_NUMBER)
		++position;
	if(position >= rank * rank)  // full filled.
		return;
	
	for(int8_t number = 1; number <= rank; ++number)
	{
		if(!isSafe(position, number))
			continue;
		
		field[position] = number;  // setNumber(position, number);
		backtrack(position + 1);
		field[position] = 0;  // setNumber(position, 0);
	}
}

void Sudoku::backtrack()
{
	backtrack(0);
}

void Sudoku::solve()
{
	if(blankCandidates.empty())
	{
		std::cout << "this sodoku is already solved\n";
		return;
	}
	
	auto printStep = [this](const std::vector<std::pair<int32_t, int8_t>>& steps)
	{
		for(const std::pair<int32_t, int8_t>& step: steps)
		{
			const int32_t& position = step.first;
			const int8_t& number = step.second;
			
			if(getNumber(position) != INVALID_NUMBER)
				continue;
			
			int32_t row = position / rank;
			int32_t column = position % rank;
			int width = rank < 10 ? 1:2;
			std::cout <<"  fill " 
					<< '[' << std::setw(width) << row << ']'
					<< '[' << std::setw(width) << column << ']'
					<< " with " << '\'' << Sudoku::letter(number) << '\'' << '\n';
			
			setNumber(position, number);
		}
	};
	
	const auto& blankCandidates = this->blankCandidates;
	auto countCandidate = [&blankCandidates]() -> int32_t
	{
		int32_t count = 0;
		for(const std::pair<int32_t, std::vector<int8_t>>& pair: blankCandidates)
			count += pair.second.size();  // candidates.size();
		return count;
	};
	
	int32_t count = countCandidate();
	while(true)
	{
		printCurrentState();
		std::cout << '\n';
		
		std::vector<std::pair<int32_t, int8_t>> nakedSingleSteps = findNakedSingle();
		if(!nakedSingleSteps.empty())
		{
			std::cout << "naked single move:" << '\n';
			printStep(nakedSingleSteps);
		}
		
		std::vector<std::pair<int32_t, int8_t>> hiddenSinglesteps = findHiddenSingle();
		if(!hiddenSinglesteps.empty())
		{
			std::cout << "hidden single move:" << '\n';
			printStep(hiddenSinglesteps);
		}

		this->update();
		std::cout<< toString() << '\n';
		
		// We can't take stepsMoved == 0 for termination condition because a sudoku can 
		// remove a cell's single candidate without moving a step during a cycle.
		int32_t newCount = countCandidate();
		if(newCount != 0 && newCount < count)
			count = newCount;
		else
			break;
	}
	
	if(!blankCandidates.empty())
		std::cout << "this sodoku is underdetermined" << '\n';
//	printCurrentState();
}

std::string Sudoku::toString(bool lineByLine/* = true */) const
{
	std::ostringstream os;
#if 1
	os << letter(getNumber(0));
	int32_t last = rank * rank - 1;
	for(int32_t i = 1; i < last; ++i)
	{
		os << letter(getNumber(i));
		if(lineByLine && (i + 1) % rank == 0)
			os << '\n';
	}
	if(last > 0)
		os << letter(getNumber(last));
#else
	for(int8_t r = 0; r < rank; ++r)
	{
		for(int8_t c = 0; c < rank; ++c)
			os << letter(getNumber(r, c));
		
		if(lineByLine && r != rank - 1)
			os << '\n';
	}
#endif
	return os.str();
}
