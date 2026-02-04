#!/usr/bin/env bash
set -euo pipefail

# Install Tesseract runtime + dev files.
# This gives:
#   - Headers under /usr/include/tesseract
#   - Libraries under /usr/lib/x86_64-linux-gnu (libtesseract.so, etc.)

install_tesseract_apt() {
  echo "[*] Updating package lists (requires sudo)..."
  sudo apt-get update

  echo "[*] Installing Tesseract OCR and development packages..."
  sudo apt-get install -y \
    tesseract-ocr \
    libtesseract-dev \
    libleptonica-dev
}

show_layout() {
  echo
  echo "[*] Installed Tesseract development layout:"
  echo "    Headers:   /usr/include/tesseract/  (from libtesseract-dev)"
  echo "    Library:   /usr/lib/x86_64-linux-gnu/libtesseract.so"
  echo "    Runtime:   /usr/bin/tesseract"
}

main() {
  install_tesseract_apt
  show_layout
}

main "$@"