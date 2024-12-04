"""
Download and extract MNIST from Kaggle.

This assumes:
1. All requirements are satisfied according to requirements.txt
2. Kaggle.json exists in ~/.kaggle/kaggle.json
3. Directory is created, for example ~/mnist_data and passed to output_dir
"""
import os
import numpy as np
from PIL import Image
from scipy.io import loadmat
from absl import app, flags
import kaggle

FLAGS = flags.FLAGS

# Define command-line flags
flags.DEFINE_string("output_dir", "", "Directory to store processed MNIST images.")
flags.DEFINE_string("mat_file", "mnist-original.mat", "Path to the MATLAB .mat file.")
flags.DEFINE_boolean("skip_download", False, "Skip downloading the dataset.")
flags.DEFINE_string("kaggle_json", 'kaggle.json', "Path to the Kaggle JSON file for authentication.")


def authenticate_kaggle(kaggle_json):
    """Authenticate Kaggle API using the provided Kaggle JSON file."""
    if kaggle_json:
        os.environ["KAGGLE_CONFIG_DIR"] = os.path.dirname(kaggle_json)

    kaggle.api.authenticate()


def download_mnist_kaggle(output_dir):
    """Download MNIST data from Kaggle using Kaggle API."""
    kaggle.api.dataset_download_files(
        "avnishnish/mnist-original", path=output_dir, unzip=True
    )
    print(f"Downloaded and unzipped dataset to: {output_dir}")


def extract_mat_to_images(mat_file, output_dir):
    """Extract MATLAB MNIST data and save as PNGs."""
    if not os.path.exists(mat_file):
        raise FileNotFoundError(f"{mat_file} not found.")

    # Load the MATLAB file
    data = loadmat(mat_file)
    images = data['data'].T  # Transpose to get rows as samples
    labels = data['label'][0]

    # Create output directories
    os.makedirs(output_dir, exist_ok=True)

    for i, (image, label) in enumerate(zip(images, labels)):
        label_dir = os.path.join(output_dir, str(int(label)))
        os.makedirs(label_dir, exist_ok=True)

        # Save image as a PNG file
        img = image.reshape(28, 28).astype(np.uint8)  # Reshape and convert to uint8
        img_path = os.path.join(label_dir, f"{i}.png")
        Image.fromarray(img).save(img_path)

    print(f"Extracted MNIST images saved to: {output_dir}")


def main(argv):
    # Parse arguments
    output_dir = FLAGS.output_dir
    mat_file = os.path.join(output_dir, FLAGS.mat_file)

    if output_dir is None:
        raise ValueError("Please specify --output_dir.")

    # Step 1: Authenticate and Download dataset if not skipped
    if not FLAGS.skip_download:
        print("Authenticating Kaggle API...")
        authenticate_kaggle(FLAGS.kaggle_json)
        print("Downloading MNIST dataset from Kaggle...")
        download_mnist_kaggle(output_dir)

    # Step 2: Convert MATLAB data to images
    print("Processing MATLAB file to generate PNG images...")
    extract_mat_to_images(mat_file, output_dir)


if __name__=="__main__":
    app.run(main)
