// Load Sudoku image, detect cells, solves and puts back answer into an image.
// bazel run //:solver_main -- --image_path=$HOME/sudoku/testdata/sudoku_9_9.png
// --model_path=$HOME/sudoku/model.yml
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "cell_detection.h"
#include "digit_detector.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "solver/solver.h"

ABSL_FLAG(std::string, image_path, "/tmp/sudoku_9_9.png", "Sudoku image path");

std::vector<std::vector<int32_t>> DetectDigitsFromCells(
    const std::vector<std::vector<sudoku::SudokuDetection>>& cells,
    const sudoku::DigitDetectorTesseract& detector) {
  auto grid = std::vector<std::vector<int32_t>>(9, std::vector<int32_t>(9, 0));
  for (size_t row = 0; row < 9; ++row) {
    for (size_t col = 0; col < 9; ++col) {
      auto digit = detector.Detect(cells[row][col].digit_image);
      CHECK(digit.has_value()) << "Failed to detect " << row << " : " << col;
      grid[row][col] = digit.value();
    }
  }
  return grid;
}

void ShowAnswers(
    const cv::Mat& image,
    const std::vector<std::vector<sudoku::SudokuDetection>> detection,
    const std::vector<std::vector<int32_t>>& grid,
    const std::vector<std::vector<int32_t>>& solved_grid) {
  for (size_t row = 0; row < 9; ++row) {
    for (size_t col = 0; col < 9; ++col) {
      if (grid[row][col] == 0) {
        const int32_t answer = solved_grid[row][col];
        const cv::Rect digit_box = detection[row][col].digit_position;

        cv::Point text_center(digit_box.x + digit_box.width / 2,
                              digit_box.y + digit_box.height / 2);

        // Adjust the font scale and thickness to fit the digit box size
        const double font_scale = static_cast<double>(digit_box.height) / 20.0;
        const int32_t thickness = static_cast<int>(font_scale * 1.5);

        // Calculate the text size and adjust the position to center it
        const cv::Size text_size = cv::getTextSize(
            std::to_string(answer), cv::FONT_HERSHEY_COMPLEX_SMALL, font_scale,
            thickness, nullptr);
        text_center.x -= text_size.width / 2;
        text_center.y += text_size.height / 3;

        cv::putText(image, std::to_string(answer), text_center,
                    cv::FONT_HERSHEY_PLAIN, font_scale, cv::Scalar(255, 0, 0),
                    thickness);
      }
    }
  }
}

bool Run() {
  using sudoku::DetectCells;
  using sudoku::DigitDetectorTesseract;
  using sudoku::Solve;
  using sudoku::SudokuDetection;

  DigitDetectorTesseract detector = DigitDetectorTesseract::Create();

  cv::Mat image = cv::imread(absl::GetFlag(FLAGS_image_path));
  CHECK(!image.empty()) << "Could not open " << absl::GetFlag(FLAGS_image_path);

  const std::string kWindow = "Sudoku";
  cv::imshow(kWindow, image);

  // Detect cells from image
  std::vector<std::vector<SudokuDetection>> cells = DetectCells(image);
  std::vector<std::vector<int32_t>> grid =
      DetectDigitsFromCells(cells, detector);

  // Solve and show answers
  std::vector<std::vector<int32_t>> solved_grid(grid.begin(), grid.end());
  CHECK(Solve(solved_grid));
  ShowAnswers(image, cells, grid, solved_grid);
  cv::imshow(kWindow, image);

  cv::waitKey(0);
  cv::destroyAllWindows();

  return true;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(*argv);
  absl::ParseCommandLine(argc, argv);
  gflags::SetCommandLineOption("logtostderr", "1");
  return Run() ? EXIT_SUCCESS : EXIT_FAILURE;
}