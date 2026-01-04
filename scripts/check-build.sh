#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build}"

if [[ ! -f "${ROOT_DIR}/${BUILD_DIR}/CMakeCache.txt" ]]; then
  cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/${BUILD_DIR}"
fi

cmake --build "${ROOT_DIR}/${BUILD_DIR}"
