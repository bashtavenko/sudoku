#include "cell_detection.h"
#include "opencv2/imgproc.hpp"

namespace sudoku {

std::vector<std::vector<SudokuDetection>> DetectCells(const cv::Mat& image) {
  constexpr size_t n = 9;
  std::vector<std::vector<SudokuDetection>> detections(n, std::vector<SudokuDetection>(n));
  cv::Mat gray;

  // 1. Preprocessing
  cv::Mat blurred;
  cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
  cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);  // Noise suppression
  cv::Mat thresholded;
  cv::adaptiveThreshold(blurred, thresholded, 255,
                        cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV,
                        11, 2);

  // 2. Detect the outer grid
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
  cv::dilate(thresholded, thresholded, kernel);
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(thresholded, contours, cv::RETR_EXTERNAL,
                   cv::CHAIN_APPROX_SIMPLE);

  double max_area = 0;
  std::vector<cv::Point> largest_contour;
  for (size_t i = 0; i < contours.size(); ++i) {
    double area = cv::contourArea(contours[i]);
    if (area > max_area) {
      max_area = area;
      largest_contour = contours[i];
    }
  }
  if (largest_contour.size() < 4) return detections;

  // 3. Warp Perspective
  std::vector<cv::Point2f> corners(4);
  // Simplifies contour into a polygon with fewer vertices
  // while retaining its overall shape.
  cv::approxPolyDP(/*curve=*/largest_contour,
                   /*approxCurve=*/corners, /*epsilon=*/
                   0.02 * cv::arcLength(largest_contour,
                                        /*closed=*/true),
                   /*closed=*/true);
  if (corners.size() != 4) return detections;

  // Order corners consistently (top-left, top-right, bottom-right, bottom-left)
  std::sort(corners.begin(), corners.end(),
            [](const cv::Point2f& a, const cv::Point2f& b) {
              return (a.y < b.y) || (a.y == b.y && a.x < b.x);
            });

  std::vector<cv::Point2f> sorted_corners = {corners[0], corners[1], corners[3],
                                             corners[2]};

  // Maps sorted points to the destination points.
  // It outputs 3x3 matrix which includes translation, rotation, scaling and
  // perspective distortion. First two rows describes how points are transformed
  // in x and y coordinate. Third row adds perspective effect, allowing for
  // non-linear distortion.
  cv::Mat warp_matrix = cv::getPerspectiveTransform(
      sorted_corners,
      std::vector<cv::Point2f>{cv::Point2f(0, 0), cv::Point2f(450, 0),
                               cv::Point2f(450, 450), cv::Point2f(0, 450)});

  // Inverse transform matrix to map warped back to original coordinates
  cv::Mat inverse_warp_matrix = cv::getPerspectiveTransform(
      std::vector<cv::Point2f>{cv::Point2f(0, 0), cv::Point2f(450, 0),
                               cv::Point2f(450, 450), cv::Point2f(0, 450)},
      sorted_corners);

  cv::Mat warped;
  // Applies transformation matrix to the entire image.
  cv::warpPerspective(gray, warped, warp_matrix, cv::Size(450, 450));

  // 4. Divide into Cells
  constexpr int32_t cell_size = 450 / n;
  for (size_t y = 0; y < n; ++y) {
    for (size_t x = 0; x < n; ++x) {
      cv::Rect cell_region(x * cell_size, y * cell_size, cell_size, cell_size);
      cv::Mat cell = warped(cell_region).clone();

      // Crop borders
      cell = cell(cv::Rect(5, 5, 40, 40));

      // Calculate the corners of the cell in warped coordinates
      std::vector<cv::Point2f> cell_corners = {
          cv::Point2f(x * cell_size, y * cell_size),
          cv::Point2f((x + 1) * cell_size, y * cell_size),
          cv::Point2f((x + 1) * cell_size, (y + 1) * cell_size),
          cv::Point2f(x * cell_size, (y + 1) * cell_size),
      };

      // Map cell corners back to the original image to get a bounding box
      std::vector<cv::Point2f> original_corners(4);
      cv::perspectiveTransform(cell_corners, original_corners, inverse_warp_matrix);

      cv::Rect bounding_box = cv::boundingRect(original_corners);

      // Add the cell and its original bounding box to the output
      detections[y][x] = SudokuDetection{cell, bounding_box};
    }
  }

  return detections;
}

} // namespace sudoku
