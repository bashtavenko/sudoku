#include "tesseract_helpers.h"

namespace sudoku {
absl::StatusOr<TesseractImage> ConvertMatToTesseractImage(const cv::Mat& mat) {
  if (mat.empty()) {
    return absl::InvalidArgumentError("Input cv::Mat is empty");
  }

  const int32_t width = mat.cols;
  const int32_t height = mat.rows;
  const int32_t channels = mat.channels();

  Pix* pix = nullptr;

  switch (channels) {
    case 1: {
      pix = pixCreate(width, height, 8);
      break;
    }
    case 3:
    case 4: {
      pix = pixCreate(width, height, 32);
      break;
    }
    default: {
      return absl::InvalidArgumentError(
          absl::StrFormat("Unsupported number of channels: %d", channels));
    }
  }

  if (pix == nullptr) {
    return absl::InternalError("pixCreate failed");
  }

  l_uint32* pix_data = pixGetData(pix);
  const int32_t pix_wpl = pixGetWpl(pix);

  for (int32_t y = 0; y < height; ++y) {
    const uint8_t* mat_row = mat.ptr<uint8_t>(y);
    l_uint32* pix_row = pix_data + y * pix_wpl;

    switch (channels) {
      case 1: {
        for (int32_t x = 0; x < width; ++x) {
          SET_DATA_BYTE(pix_row, x, mat_row[x]);
        }
        break;
      }
      case 3: {
        for (int32_t x = 0; x < width; ++x) {
          const uint8_t b = mat_row[x * 3 + 0];
          const uint8_t g = mat_row[x * 3 + 1];
          const uint8_t r = mat_row[x * 3 + 2];

          pix_row[x] = (r << 24) | (g << 16) | (b << 8);
        }
        break;
      }
      case 4: {
        for (int32_t x = 0; x < width; ++x) {
          const uint8_t b = mat_row[x * 4 + 0];
          const uint8_t g = mat_row[x * 4 + 1];
          const uint8_t r = mat_row[x * 4 + 2];
          const uint8_t a = mat_row[x * 4 + 3];

          pix_row[x] = (r << 24) | (g << 16) | (b << 8) | a;
        }
        break;
      }
      default: {
        return absl::InternalError("Unexpected channel count during conversion");
      }
    }
  }

  return TesseractImage(pix);
}


} // namespace sudoku