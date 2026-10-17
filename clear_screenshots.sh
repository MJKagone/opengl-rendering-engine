#!/usr/bin/env bash
# Clears everything inside the screenshots/ folder (keeps the folder itself)
set -euo pipefail

if [[ -d screenshots ]]; then
    find screenshots -mindepth 1 -maxdepth 1 -exec rm -rf {} +
    echo "Cleared screenshots/"
else
    echo "No screenshots/ folder found, nothing to clear"
fi
