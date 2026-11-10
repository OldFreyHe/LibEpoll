#!/usr/bin/env bash
set -euo pipefail

# Resolve paths relative to this script, independent of the caller's directory.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${project_dir}/build/tests"

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Run this script on Linux or inside WSL." >&2
    exit 1
fi

for tool in cmake ctest; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required command not found: $tool" >&2
        exit 1
    fi
done

cmake -S "$project_dir" -B "$build_dir" \
    -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir" --config Debug --parallel
# Additional arguments are forwarded to CTest, e.g. -R connection_send or -V.
ctest --test-dir "$build_dir" -C Debug --output-on-failure --no-tests=error "$@"
