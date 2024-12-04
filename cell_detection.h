#ifndef SUDOKU__CELL_DETECTION_H_
#define SUDOKU__CELL_DETECTION_H_

#include <vector>
#include "opencv2/core.hpp"

namespace sudoku{

// Result of sudoku detection
struct SudokuDetection {
  cv::Mat digit_image;      // The extracted cell image (digit or empty cell)
  cv::Rect digit_position;  // The position of the cell in the original image
};

// Detect Sudoku cells from an image
std::vector<std::vector<SudokuDetection>> DetectCells(const cv::Mat& image);

} // namespace sudoku

#endif  // SUDOKU__CELL_DETECTION_H_
