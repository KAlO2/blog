#ifndef GITHUB_KALO2_SUDOKU_
#define GITHUB_KALO2_SUDOKU_

#include <cstdint>
#include <map>
#include <set>
#include <vector>

/**
 * Sudoku is a logic-based, combinatiorial number-placement puzzle. The objective is to fill a 9×9 
 * grid with digits so that each column, each row, and each block (the nine 3×3 subgrids) contain 
 * all of the digits from 1 to 9. The puzzle setter provides a partially completed grid, which for 
 * a well-posed puzzle has a single solution.
 *
 * Math is fun, so it is with <a href="https://en.wikipedia.org/wiki/Mathematics_of_Sudoku">sudoku</a>.
 * The standard Sudoku's rank is 9. Sudoku of larger size or irregular shape can be more challenging.
 * The general problem of solving sudoku puzzles on n x n grids of sqrt(n) x sqrt(n) blocks is known 
 * to be NP-complete.
 *
 * <a href="https://en.wikipedia.org/wiki/Sudoku_solving_algorithms#Backtracking">Backtracking</a>
 * is a baseline solver for sudoku, but time will increase dramatically when size grows large, so
 * other feasible algorithms should be developed.
 */
class alignas(4) Sudoku
{
private:
	const int8_t rank;
	const std::vector<int8_t> initialState;
	const std::vector<int8_t> blockIndices;
	
	std::vector<int8_t> field;  // it will be updated step by step, until all the cells are filled.
	std::vector<int32_t> map;  // 2D array to store position, map[blockIndex][value] = position.
	std::map<int32_t, std::vector<int8_t>> blankCandidates;  // number candidates of blank cells.
	std::vector<std::vector<int32_t>> blankBlocks;  // blank positions of blocks
	
private:
	/**
	 * To parse the input text to cells' number.
	 * @param[in] letters The letters to feed for a sudoku. Valid letters are [0-9a-zA-Z], and 0 is used
	 *        as the empty character.
	 * @param[in] length It's equal to rank * rank.
	 * @return cell number array.
	 */
	static std::vector<int8_t> parse(const char* letters, int32_t length);
	
	/**
	 * To parse the input text to cells' number.
	 * @param letters The letters to feed for a sudoku. Valid letters are [1-9a-zA-Z], and [0 *\.] 
	 *        can be used as empty character.
	 * @param length It's equal to rank * rank.
	 * @param placeholder The empty cell to be filled.
	  * @return cell number array.
	 */
	static std::vector<int8_t> parse(const char* letters, int32_t length, char placeholder);
	
	/**
	 * validate sudoku's initial state.
	 */
	void validate(const std::vector<int8_t>& state) const noexcept(false);
	
	/**
	 * @return whether the block partition is valid or not.
	 */
	bool isBlockPartitionValid() const;
	
	/**
	 * Check whether the group contains at most one value, group can be row, column or block.
	 */
	bool isGroupValid() const;
	
	/**
	 * @param blockIndex range [1, rank]
	 * @param number range [1, rank]
	 * @return valid range [0, rank * rank). If it is blank, return an invalid (nagative) value.
	 */
	int32_t getMapPosition(int8_t blockIndex, int8_t number) const;
	void    setMapPosition(int8_t blockIndex, int8_t number, int32_t position);
	
	/**
	 * @param position cell must be unfilled.
	 * @param number range [1, rank]
	 * @return true if successfully removed, false if it doesn't exist.
	 */
	bool removeCandidate(int32_t position, int8_t number);
	
	/**
	 * This is the simplest logic. If there is one cell that contains a single candidate, then that 
	 * candidate is the solution for that cell. 
	 * @return steps that can take, each step is (position, number) pair.
	 */
	std::vector<std::pair<int32_t, int8_t>> findNakedSingle() const;
	
	/**
	 * Hidden single strategy: If a group (row, column, or block) has one unique number for a cell,
	 * namely, this number is not other cells's candidate, then it's time to fill it.
	 * @return steps that can take, each step is (position, number) pair.
	 */
	std::vector<std::pair<int32_t, int8_t>> findHiddenSingle() const;
	
	/**
	 * A naked pair is two cells of identical candidates found in a particular group.
	 */
	void updateCandidateByNakedPair();
	
	/**
	 * Hidden pairs are identified by the fact that a pair of numbers occur in only two cells of a 
	 * group. They are "hidden" because the other numbers in the two cells make their presence 
	 * harder to spot. It is safe to remove all other candidates from the two cells so that only the
	 * two hidden candidates remain. 
	 */
	void updateCandidateByHiddenPair();
	
	/**
	 * A naked triple is three cells of three candidates found in a particular group. Unlike naked 
	 * pairs, naked triples don't need all of the three candidates in every cell. Quite often only 
	 * two of the three candidates will be shown. 
	 */
	void updateCandidateByNakedTriple();
	
	void updateCandidateOutBlockOfLine();
	void updateCandidateInBlockOutOfLine(bool horizontal);
	
	void updateCandidateByXWing(bool horizontal);
	
	void updateCandidateInOneLine      (bool horizontal);
	void updateCandidateBetweenTwoLines(bool horizontal);
	void updateCandidateAmongThreeLines(bool horizontal);
	
	/**
	 * Update candidates of this sudoku when @p position is filled with @p number.
	 */
	void updateNumber(int32_t position, int8_t number);
	
	void backtrack(int32_t position);
	
	void printCurrentState() const;
	
public:
	static constexpr int8_t RANK_MAX = 9 + 26;  ///< 1 ~ 9, a ~ z. 0 is reserved for blank area.
	static constexpr int32_t INVALID_POSTION = -1;
	static constexpr int8_t  INVALID_NUMBER = 0;
	
	enum Group: int8_t
	{
		NONE   = 0,
		ROW    = 1,
		COLUMN = 2,
		BLOCK  = 3,
	};
	
	static const char* GROUP_TEXT[4];  // = {"none", "row", "column", "block"}
	
	/**
	 * map letter to number.
	 * @param letter characters can be '0' ~ '9', 'a' ~ 'z'. Capital letters are allowed here, and 
	 *        they are treated like lower letters.
	 * @return number.
	 */
	static int8_t number(char letter);
	
	/**
	 * map number to letter.
	 * @param number numbers between 0 (inclusive) and 36(exclusive).
	 * @return lower letter.
	 */
	static char letter(int8_t number);
	
public:
	/**
	 * @param[in] rank sudoku's size.
	 * @param[in] state cell values in row-major, 0 for unfilled cell.
	 * @param[in] block cell partion, values are from 1 to @p rank.
	 */
	Sudoku(int8_t rank, const char* state, const char* block, char placeholder = '0') noexcept(false);
	
	int8_t getRank() const;
	
	/**
	 * This is a simple assign @p number to the @p position operation, and you may need to call 
	 */
	void   setNumber(int32_t position, int8_t number);
	int8_t getNumber(int32_t position) const;
	
	void   setNumber(int8_t row, int8_t column, int8_t number);
	int8_t getNumber(int8_t row, int8_t column) const;
	
	/**
	 * Check whether current state conflicts or not, namely at least two numbers are in one row,
	 * column or block. Note that it is a global searching, it's inefficient than @fn 
	 * bool isSafe(int8_t row, int8_t column, int8_t number) const.
	 * @ return true if no conflicts, otherwise false.
	 */
	bool isSafe() const;
	
	/**
	 * Check whether current state conflicts when a blank cell (@p row, @p column) is filled with.
	 * This is often used in step by step detection.
	 * @p number.
	 * @param row    range [0, rank)
	 * @param column range [0, rank)
	 * @param number range [1, rank]
	 * @ return true if no conflicts, otherwise false.
	 */
	bool isSafe(int8_t row, int8_t column, int8_t number) const;
	bool isSafe(int32_t position, int8_t number) const;
	
	/**
	 * @return current blank cells' candidates.
	 */
	const std::map<int32_t, std::vector<int8_t>>& getBlankCandidates() const;
	
	/**
	 * update cells' candidate numbers.
	 */
	void update();
	
	/**
	 * Find solution by backtracking algorithm, brute-force search can be time-consuming.
	 */
	void backtrack();
	
	void solve();
	
	std::string toString(bool lineByLine = true) const;

};

#endif  // GITHUB_KALO2_SUDOKU_
