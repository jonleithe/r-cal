#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

build_configuration() {
    local name="$1"
    local cmake_type="$2"
    local build_dir="${project_dir}/build/${name}"

    echo "Configuring ${cmake_type} build in ${build_dir}"
    cmake \
        -S "${project_dir}" \
        -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE="${cmake_type}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    echo "Building ${cmake_type}"
    cmake --build "${build_dir}" --parallel
}

usage() {
    echo "Usage: $0 {debug|release|all}" >&2
}

case "${1:-}" in
    debug)
        build_configuration debug Debug
        ;;
    release)
        build_configuration release Release
        ;;
    all)
        build_configuration debug Debug
        build_configuration release Release
        ;;
    *)
        usage
        exit 2
        ;;
esac
