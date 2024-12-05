// This is largely based on https://elementsofprogramminginterviews.com/
//
#include <cstdint>
#include <vector>

#ifndef SUDOKU_SOLVER_SOLVER_H_
#define SUDOKU_SOLVER_SOLVER_H_

namespace sudoku {

// Checks if partially assembled grid is valid
bool IsValid(const std::vector<std::vector<int32_t>>& grid);

// 9x9 grid  with 3 x 3 sections
// Constraints:
//  No duplicates in rows
//  No duplicates in columns
//  All sections contain all numbers from 1 to 9
//
// Recursive solver with backtracking. Tries all numbers recursively and
// backtracks if that number didn't work.  Check validity on add (row,
// column, region);
bool Solve(std::vector<std::vector<int32_t>>& grid);

} // namespace sudoku

#endif  // SUDOKU_SOLVER_SOLVER_H_
