// This is largely based on https://elementsofprogramminginterviews.com/
//
#include "solver.h"
#include <queue>
#include <cmath>
#include "absl/algorithm/container.h"

namespace sudoku {

bool IsValid(const std::vector<std::vector<int32_t>>& grid) {
  const size_t n = grid[0].size();

  auto has_duplicate = [&](size_t start_row, size_t end_row, size_t start_col,
                           size_t end_col) {
    std::deque<bool> is_present(n + 1, false);
    for (size_t row = start_row; row < end_row; ++row) {
      for (size_t col = start_col; col < end_col; ++col) {
        if (grid[row][col] != 0 && is_present[grid[row][col]]) {
          return true;
        }
        is_present[grid[row][col]] = true;
      }
    }
    return false;
  };

  // Check row constraint
  for (size_t row = 0; row < n; ++row) {
    if (has_duplicate(row, row + 1, 0, n)) return false;
  }

  // Check column constraint
  for (size_t col = 0; col < n; ++col) {
    if (has_duplicate(0, n, col, col + 1)) return false;
  }

  // Check region constraint
  // In 9x9 grid there are 9 sub-grids
  const size_t region_size = std::sqrt(n);
  for (size_t region_row = 0; region_row < region_size; ++region_row) {
    for (size_t region_col = 0; region_col < region_size; ++region_col) {
      if (has_duplicate(
              region_size * region_row, region_size * (region_row + 1),
              region_size * region_col, region_size * (region_col + 1))) {
        return false;
      }
    }
  }
  return true;
}

bool Solve(std::vector<std::vector<int32_t>>& grid) {
  const size_t n = grid.size();
  constexpr int32_t kEmpty = 0;

  // We are trying to add a new value and assume that before grid was valid
  auto valid_to_add = [&](size_t row, size_t col, int32_t val) {
    // Check row constraint
    for (size_t k = 0; k < n; ++k) {
      if (val == grid[k][col]) return false;
    }

    // Check column constraint
    if (absl::c_any_of(grid[row], [val](int32_t v) { return val == v; }))
      return false;

    // Check region constraint, meaning 3 x 3 sections
    const size_t region_size = std::sqrt(n);
    size_t region_row = row / region_size;
    size_t region_col = col / region_size;
    for (size_t r = 0; r < region_size; ++r) {
      for (size_t c = 0; c < region_size; ++c) {
        if (val ==
            grid[region_size * region_row + r][region_size * region_col + c])
          return false;
      }
    }
    return true;
  };

  std::function<bool(size_t, size_t)> solve_partial = [&](size_t row,
                                                          size_t col) {
    if (row == n) {
      row = 0;                                     // Start a new row
      if (++col == grid[row].size()) return true;  // Done, no conflicts.
    }

    // Skips nonempty entries
    if (grid[row][col] != kEmpty) {
      return solve_partial(row + 1, col);
    }

    // Try all numbers from 1 to 9
    for (int32_t val = 1; val <= static_cast<int32_t>(n); ++val) {
      if (valid_to_add(row, col, val)) {
        grid[row][col] = val;  // try this number
        if (solve_partial(row + 1, col)) {
          return true;
        }
      }
    }
    grid[row][col] = kEmpty;  // Backtrack, didn't work.
    return false;
  };

  return solve_partial(0, 0);
}

} // namespace sudoku

