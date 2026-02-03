#ifndef SUDOKU__DIGIT_DETECTOR_H_
#define SUDOKU__DIGIT_DETECTOR_H_
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "opencv2/core.hpp"
#include "opencv2/ml.hpp"

namespace sudoku {

// Detects Sudoku digits
class DigitDetector {
 public:
  // Loads saved model. In any errors crashes binary with CHECK.
  void Init(absl::string_view model_path);

  // Detects image. Returns `std::nullopt` if the image could not be recognized.
  absl::optional<int32_t> Detect(const cv::Mat& image) const;

  // Uses StatModel::train for training. May throw an exception from OpenCV.
  // Otherwise, returns true.
  bool Train(absl::string_view mnist_directory, absl::string_view model_path,
             size_t synthetic_count);

  template <typename T>
  cv::Ptr<T> GetModelAs() const {
    return model_.dynamicCast<T>();
  }

 private:
  cv::Ptr<cv::ml::StatModel> model_;
};

}  // namespace sudoku

#endif  // SUDOKU__DIGIT_DETECTOR_H_
