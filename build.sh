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

activate_configuration() {
    local name="$1"
    local active_binary="${project_dir}/build/r-cal"

    cmake -E rm -f "${active_binary}"
    cmake -E create_symlink "${name}/r-cal" "${active_binary}"
    echo "Active executable: ${active_binary} -> ${name}/r-cal"
}

usage() {
    echo "Usage: $0 {debug|release|all}" >&2
}

case "${1:-}" in
    debug)
        build_configuration debug Debug
        activate_configuration debug
        ;;
    release)
        build_configuration release Release
        activate_configuration release
        ;;
    all)
        build_configuration debug Debug
        build_configuration release Release
        activate_configuration release
        ;;
    *)
        usage
        exit 2
        ;;
esac
