#!/usr/bin/env bash
set -euo pipefail

THIRD_PARTY_DIR="third_party/tesseract"

#  Assuming
#  sudo apt-get install tesseract-ocr libtesseract-dev libleptonica-dev
#  We only need headers because Tesseract wasn't built from source.
SYS_TESS_INCLUDE="/usr/include/tesseract"
SYS_LEPT_INCLUDE="/usr/include/leptonica"
SYS_LIB_DIR="/usr/lib/x86_64-linux-gnu"

mkdir -p "${THIRD_PARTY_DIR}"

# Headers
ln -sf "${SYS_TESS_INCLUDE}"    "${THIRD_PARTY_DIR}/include_tesseract"
ln -sf "${SYS_LEPT_INCLUDE}"    "${THIRD_PARTY_DIR}/include_leptonica"

echo "[*] Created Bazel-friendly symlinks:"
echo "    ${THIRD_PARTY_DIR}/include_tesseract -> ${SYS_TESS_INCLUDE}"
echo "    ${THIRD_PARTY_DIR}/include_leptonica -> ${SYS_LEPT_INCLUDE}"

