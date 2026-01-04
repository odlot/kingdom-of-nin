#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

git config core.hooksPath "${ROOT_DIR}/.githooks"
echo "Git hooks configured to use ${ROOT_DIR}/.githooks"
