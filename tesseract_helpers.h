#ifndef SUDOKU_TESSERACT_HELPERS_H
#define SUDOKU_TESSERACT_HELPERS_H
#include "leptonica/allheaders.h"
#include "tesseract/baseapi.h"
#include "opencv2/core.hpp"
#include "absl/status/statusor.h"

namespace sudoku {

// Custom deleter for Tesseract API
struct TessApiDeleter {
  void operator()(tesseract::TessBaseAPI* api) const noexcept {
    if (api) {
      api->End();   // releases internal Tesseract resources
      delete api;
    }
  }
};

// Custom deleter for Leptonica Pix
struct PixDeleter {
  void operator()(Pix* p) const noexcept {
    if (p) {
      pixDestroy(&p);
    }
  }
};

// Custom deleter for Tesseract's UTF8 text buffer
struct Utf8TextDeleter {
  void operator()(char* p) const noexcept {
    delete[] p;
  }
};

using Tesseract = std::unique_ptr<tesseract::TessBaseAPI, TessApiDeleter>;
using TesseractImage = std::unique_ptr<Pix, PixDeleter>;
using TesseractText = std::unique_ptr<char, Utf8TextDeleter>;

// Converts cv::Mat to Leptonica Pix image format.
// This is probably redundant since Tesseract now supports cv::Mat directly
absl::StatusOr<TesseractImage> ConvertMatToTesseractImage(const cv::Mat& mat);

} // namespace sudoku


#endif  // SUDOKU_TESSERACT_HELPERS_H
