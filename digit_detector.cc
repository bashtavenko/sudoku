#include "digit_detector.h"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "glog/logging.h"

namespace sudoku {

void DigitDetector::Init(absl::string_view model_path) {
  model_ = cv::ml::KNearest::load(std::string(model_path));
  CHECK(model_ != nullptr) << "Failed to load model from: " << model_path;
}

absl::optional<int32_t> DigitDetector::Detect(const cv::Mat& image) const {
  if (image.empty()) return std::nullopt;
  if (!model_->isTrained()) {
    LOG(ERROR) << "Model is not trained";
    return std::nullopt;
  }

  cv::Mat gray_image;
  if (image.channels() == 3) {
    cv::cvtColor(image, gray_image, cv::COLOR_BGR2GRAY);
  } else {
    gray_image = image;
  }

  // Invert the image - white digit on black background due to MNIST examples.
  cv::Mat inverted_image;
  cv::bitwise_not(gray_image, inverted_image);

  // Resize the input image to 28x28 MNIST input
  cv::Mat resized_image;
  cv::resize(inverted_image, resized_image, cv::Size(28, 28));

  // Preprocess the image (e.g., flatten and normalize)
  cv::Mat processed_image =
      resized_image.reshape(1, 1);  // Flatten to a row vector
  processed_image.convertTo(processed_image, CV_32F,
                            1.0 / 255.0);  // Ensure correct type

  // Predict the digit
  float response = model_->predict(processed_image);
  if (response < 0) return std::nullopt;

  return static_cast<int32_t>(response);
}

bool DigitDetector::Train(absl::string_view mnist_directory,
                          absl::string_view model_path,
                          size_t synthetic_count) {
  // Augment and return a clone of the original image.
  auto augment_image = [&](const cv::Mat& img) {
    cv::Mat augmented_image = img.clone();

    // Random rotation between -10 and 10 degrees
    double angle =
        (rand() % 21 - 10);  // Random angle between -10 and 10 degrees
    cv::Point2f center(augmented_image.cols / 2, augmented_image.rows / 2);
    cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(augmented_image, augmented_image, rotation_matrix,
                   augmented_image.size(), cv::INTER_LINEAR,
                   cv::BORDER_REFLECT);

    // Random translation (up to 2 pixels)
    int tx = std::rand() % 5 -
             2;  // Random translation in x direction (-2 to 2 pixels)
    int ty = std::rand() % 5 -
             2;  // Random translation in y direction (-2 to 2 pixels)
    cv::Mat translation_matrix = cv::Mat::eye(2, 3, CV_32F);
    translation_matrix.at<float>(0, 2) = tx;
    translation_matrix.at<float>(1, 2) = ty;
    cv::warpAffine(augmented_image, augmented_image, translation_matrix,
                   augmented_image.size(), cv::INTER_LINEAR,
                   cv::BORDER_REFLECT);

    // Random blurring (kernel size 3x3 or 5x5)
    int blur_kernel_size = (std::rand() % 2 == 0)
                               ? 3
                               : 5;  // Randomly select 3x3 or 5x5 kernel size
    cv::GaussianBlur(augmented_image, augmented_image,
                     cv::Size(blur_kernel_size, blur_kernel_size), 0);

    return augmented_image;
  };

  auto knn_model = cv::ml::KNearest::create();
  knn_model->setAlgorithmType(cv::ml::KNearest::Types::BRUTE_FORCE);
  std::vector<cv::Mat> images;
  std::vector<int> label_list;

  // Ignore 0
  for (int digit = 1; digit <= 9; ++digit) {
    std::string digit_dir =
        std::string(mnist_directory.data()) + "/" + std::to_string(digit);
    for (const auto& entry : std::filesystem::directory_iterator(digit_dir)) {
      cv::Mat img = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
      if (img.empty()) {
        LOG(ERROR) << "Failed to load image: " << entry.path();
        return false;
      }
      CHECK(img.size() == cv::Size(28, 28));
      images.push_back(img);
      label_list.push_back(digit);

      // Augment the image with slight transformations
      for (size_t i = 0; i < synthetic_count; ++i) {
        cv::Mat augmented_img = augment_image(img);
        images.push_back(augmented_img);
        label_list.push_back(digit);
      }
    }
  }

  // Add blank samples and label them as 0
  for (int i = 0; i < 100; ++i) {
    cv::Mat blank_image = cv::Mat::zeros(28, 28, CV_8U) * 255;  // All black
    images.push_back(blank_image);
    label_list.push_back(0);
  }

  // Flattened all images to a single row and normalize.
  cv::Mat flattened_images(images.size(), 28 * 28, CV_32F);
  cv::Mat labels = cv::Mat(label_list).reshape(1, label_list.size());

  for (size_t i = 0; i < images.size(); ++i) {
    cv::Mat flattened_image = images[i].reshape(1, 1);
    flattened_image.convertTo(flattened_images.row(static_cast<int>(i)), CV_32F,
                              1.0 / 255.0);
  }
  auto train_data =
      cv::ml::TrainData::create(flattened_images, cv::ml::ROW_SAMPLE, labels);
  train_data->setTrainTestSplitRatio(0.8, /*shuffle=*/true);

  LOG(INFO) << "Starting training...";
  knn_model->setDefaultK(3);  // Looks for at most k training examples
  knn_model->train(train_data);

  LOG(INFO) << "Compute accuracy...";
  auto compute_accuracy = [&](const cv::Mat& predictions,
                              const cv::Mat& ground_truth) {
    int correct_predictions = 0;

    for (int i = 0; i < predictions.rows; ++i) {
      // TODO: No idea.
      LOG(INFO) << ground_truth.at<int32_t>(i, 0) << " : " << predictions.at<double>(i, 0);
      if (ground_truth.at<int32_t>(i, 0) == predictions.at<int32_t>(i, 0)) {
        correct_predictions++;
      }
    }
    return static_cast<float>(correct_predictions) / predictions.rows;
  };
  cv::Mat train_predictions;
  cv::Mat val_predictions;
  knn_model->predict(train_data->getTrainSamples(), train_predictions);
  knn_model->predict(train_data->getTestSamples(), val_predictions);
  float train_accuracy =
      compute_accuracy(train_predictions, train_data->getTrainResponses());
  float val_accuracy =
      compute_accuracy(val_predictions, train_data->getTestResponses());
  LOG(INFO) << "Training Accuracy: " << train_accuracy
            << " Validation Accuracy: " << val_accuracy;

  model_ = knn_model;
  model_->save(std::string(model_path));
  return true;
}
}  // namespace sudoku
