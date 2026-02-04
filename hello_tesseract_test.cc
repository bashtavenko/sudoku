#include <filesystem>
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "tesseract_helpers.h"
#include "tools/cpp/runfiles/runfiles.h"

namespace sudoku {
namespace {

using ::absl_testing::IsOk;
using ::bazel::tools::cpp::runfiles::Runfiles;

constexpr absl::string_view kTestDataPath = "_main/testdata";

TEST(TesseractSmokeTest, Works) {
  const auto files = Runfiles::CreateForTest();

  Tesseract api(new tesseract::TessBaseAPI());
  ASSERT_EQ(0, api->Init(nullptr, "eng"));

  cv::Mat image = cv::imread(
      files->Rlocation(std::filesystem::path(kTestDataPath) / "digit-1.png"));
  ASSERT_FALSE(image.empty());

  api->SetImage(image.data, image.cols, image.rows, 3, image.step);

  TesseractText text(api->GetUTF8Text());
  ASSERT_TRUE(text != nullptr);

  ASSERT_EQ("1\n", std::string(text.get()));
}

}  // namespace
}  // namespace sudoku