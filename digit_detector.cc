#include "digit_detector.h"
#include <filesystem>
#include "absl/strings/numbers.h"
#include "glog/logging.h"
#include "opencv2/opencv.hpp"

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

  // Invert the image-white digit on a black background due to MNIST examples.
  cv::Mat inverted_image;
  cv::bitwise_not(gray_image, inverted_image);

  // Resize the input image to 28x28 MNIST input
  cv::Mat resized_image;
  cv::resize(inverted_image, resized_image, cv::Size(28, 28));

  // Preprocess the image (e.g., flatten and normalize)
  cv::Mat processed_image =
      resized_image.reshape(1, 1);  // Flatten to a row vector
  processed_image.convertTo(processed_image, CV_32F,
                            1.0 / 255.0);  // Ensure the correct type

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

  LOG(INFO) << "Computing accuracy...";
  auto compute_accuracy = [&](const cv::Mat& predictions,
                              const cv::Mat& ground_truth) {
    int correct_predictions = 0;
    for (int i = 0; i < predictions.rows; ++i) {
      if (ground_truth.at<int32_t>(i, 0) ==
          static_cast<int32_t>(predictions.at<float>(i, 0))) {
        correct_predictions++;
      }
    }
    return static_cast<float>(correct_predictions) / predictions.rows;
  };
  cv::Mat train_predictions;
  cv::Mat val_predictions;
  // auto data = train_data->getTrainSamples();
  // auto test_samples = train_data->getTestSamples();
  knn_model->predict(train_data->getTrainSamples(), train_predictions);
  knn_model->predict(train_data->getTestSamples(), val_predictions);
  float train_accuracy =
      compute_accuracy(train_predictions, train_data->getTrainResponses());
  float val_accuracy =
      compute_accuracy(val_predictions, train_data->getTestResponses());
  LOG(INFO) << "Training Accuracy: " << train_accuracy
            << " Validation Accuracy: " << val_accuracy;

  auto confusion_matrix = [&](const cv::Mat& predictions,
                              const cv::Mat& ground_truth, int num_classes) {
    cv::Mat confusion_matrix = cv::Mat::zeros(num_classes, num_classes, CV_32S);
    for (size_t i = 0; i < static_cast<size_t>(predictions.rows); ++i) {
      int32_t predicted_label =
          static_cast<int32_t>(predictions.at<float>(i, 0));
      int32_t true_label = ground_truth.at<int32_t>(i, 0);
      confusion_matrix.at<int32_t>(true_label, predicted_label)++;
    }
    return confusion_matrix;
  };
  auto compute_metrics = [&](const cv::Mat& confusion_matrix) {
    int32_t num_classes = confusion_matrix.rows;
    int32_t true_positive = 0;
    int32_t false_positive = 0;
    int32_t false_negative = 0;
    float precision = 0;
    float recall = 0;
    float f1_score = 0;
    // Row = true labels; Col = prediction labels
    for (int i = 0; i < num_classes; ++i) {
      true_positive += confusion_matrix.at<int>(i, i);
      false_positive +=
          cv::sum(confusion_matrix.row(i))[0] - confusion_matrix.at<int>(i, i);
      false_negative +=
          cv::sum(confusion_matrix.col(i))[0] - confusion_matrix.at<int>(i, i);
    }
    precision +=
        (true_positive + false_positive > 0)
            ? true_positive / static_cast<float>(true_positive + false_positive)
            : 0.0f;
    recall =
        (true_positive + false_negative > 0)
            ? true_positive / static_cast<float>(true_positive + false_negative)
            : 0.0f;
    f1_score = (precision + recall > 0)
                   ? 2 * precision * recall / (precision + recall)
                   : 0.0f;
    LOG(INFO) << "Precision: " << precision;
    LOG(INFO) << "Recall: " << recall;
    LOG(INFO) << "F1 Score: " << f1_score;
  };
  compute_metrics(
      confusion_matrix(train_predictions, train_data->getTrainResponses(), 10));

  model_ = knn_model;
  model_->save(std::string(model_path));
  return true;
}

DigitDetectorTesseract DigitDetectorTesseract::Create() {
  Tesseract api(new tesseract::TessBaseAPI());
  CHECK_EQ(0, api->Init(nullptr, "eng")) << "Failed to initialize Tesseract";
  return DigitDetectorTesseract(Tesseract(std::move(api)));
}

DigitDetectorTesseract::DigitDetectorTesseract(Tesseract tesseract)
    : tesseract_(std::move(tesseract)) {}

absl::optional<int32_t> DigitDetectorTesseract::Detect(
    const cv::Mat& image) const {
  tesseract_->SetImage(image.data, image.cols, image.rows, image.channels(),
                       image.step);
  TesseractText text(tesseract_->GetUTF8Text());
  CHECK(text != nullptr);

  // Parse OCR for integer
  const std::string raw_text = text.get();
  if (raw_text.empty()) return 0;
  const std::string stripped_text =
      raw_text.substr(raw_text.find_first_not_of(('\n')));

  std::string digits;
  digits.reserve(stripped_text.size());
  bool is_digit_run = false;
  for (char c : stripped_text) {
    if (c >= '0' && c <= '9') {
      digits.push_back(c);
      is_digit_run = true;
      continue;
    }
    if (is_digit_run) break;
  }
  if (digits.empty()) return std::nullopt;
  int32_t value = 0;
  if (!absl::SimpleAtoi(digits, &value)) {
    return std::nullopt;
  }
  return value;
}

}  // namespace sudoku
