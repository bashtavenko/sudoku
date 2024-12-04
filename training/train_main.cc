#include <gflags/gflags.h>
#include <glog/logging.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "digit_detector.h"

ABSL_FLAG(std::string, mnist_dir, "/home/joe_doe/mnist_data",
          "Directory for MNIST dataset");
ABSL_FLAG(std::string, model_path, "/tmp/model.yml", "Model output path");

int main(int argc, char** argv) {
  google::InitGoogleLogging(*argv);

  absl::ParseCommandLine(argc, argv);

  // Cannot parse both Abseil and gflags.
  gflags::SetCommandLineOption("logtostderr", "1");

  sudoku::DigitDetector detector;
  if (!detector.Train(absl::GetFlag(FLAGS_mnist_dir),
                      absl::GetFlag(FLAGS_model_path))) {
    LOG(ERROR) << "Training failed";
    return EXIT_FAILURE;
  }
  LOG(INFO) <<  absl::GetFlag(FLAGS_model_path) << " saved.";

  return EXIT_SUCCESS;
}