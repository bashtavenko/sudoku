#include "digit_detector.h"
#include <filesystem>
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "opencv2/opencv.hpp"
#include "cell_detection.h"
#include "tools/cpp/runfiles/runfiles.h"

namespace sudoku {
namespace {
using ::bazel::tools::cpp::runfiles::Runfiles;
using ::testing::NotNull;
using ::testing::Optional;
using ::testing::Eq;

constexpr absl::string_view kModelPath = "_main/model.yml";
constexpr absl::string_view kTestDataPath = "_main/testdata";

TEST(DigitDetector, Works) {
  auto files = Runfiles::CreateForTest();

  auto get_image = [&](absl::string_view image_name) {
    cv::Mat image = cv::imread(
        files->Rlocation(std::filesystem::path(kTestDataPath) / image_name));
    return image;
  };
  ASSERT_FALSE(get_image("digit-1.png").empty());

  DigitDetector detector;
  detector.Init(files->Rlocation(kModelPath.data()));
  EXPECT_THAT(detector.Detect(get_image("digit-1.png")), Optional(1));
  EXPECT_THAT(detector.Detect(get_image("digit-2.png")), Optional(2));
  EXPECT_THAT(detector.Detect(get_image("digit-3.png")), Optional(3));
  EXPECT_THAT(detector.Detect(get_image("digit-4.png")), Optional(4));
  EXPECT_THAT(detector.Detect(get_image("digit-5.png")), Optional(5));
  EXPECT_THAT(detector.Detect(get_image("digit-6.png")), Optional(6));
  EXPECT_THAT(detector.Detect(get_image("digit-7.png")), Optional(7));
  EXPECT_THAT(detector.Detect(get_image("digit-8.png")), Optional(8));
  EXPECT_THAT(detector.Detect(get_image("digit-9.png")), Optional(9));
  EXPECT_THAT(detector.Detect(get_image("digit-blank.png")), Optional(0));
}

// TODO: Requires more result matching
TEST(DISABLED_E2ETest, Works) {
  auto files = Runfiles::CreateForTest();
  cv::Mat image = cv::imread(files->Rlocation(
      std::filesystem::path(kTestDataPath) / "sudoku_9_9.png"));
  ASSERT_FALSE(image.empty());
  std::vector<std::vector<SudokuDetection>> cells = DetectCells(image);
  std::vector result(9, std::vector(9, 0));
  DigitDetector detector;
  detector.Init(files->Rlocation(kModelPath.data()));
  for (size_t row = 0; row < 9; ++row) {
    for (size_t col = 0; col < 9; ++col) {
      auto digit = detector.Detect(cells[row][col].digit_image);
      ASSERT_TRUE(digit.has_value());
      result[row][col] = digit.value();
    }
  }
  EXPECT_THAT(result, testing::ContainerEq(std::vector<std::vector<int32_t>>{
                          {5, 3, 0, 0, 7, 0, 0, 0, 0},
                          {6, 0, 0, 1, 9, 5, 0, 0, 0},
                          {0, 9, 8, 0, 0, 0, 0, 6, 0},
                          {8, 0, 0, 0, 6, 0, 0, 0, 3},
                          {4, 0, 0, 8, 0, 3, 0, 0, 1},
                          {7, 0, 0, 0, 2, 0, 0, 0, 6},
                          {0, 6, 0, 0, 0, 0, 2, 8, 0},
                          {0, 0, 0, 4, 1, 9, 0, 0, 5},
                          {0, 0, 0, 0, 8, 0, 0, 7, 9}}));
}
}  // namespace
}  // namespace sudoku