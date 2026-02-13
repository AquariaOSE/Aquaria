#!/usr/bin/env bash
set -euo pipefail

# Build a macOS release payload for the updater:
#   <output>/macOS/aquaria
#   <output>/macOS/libSDL2-2.0.0.dylib
#
# Defaults:
# - Universal build (arm64 + x86_64)
# - Deployment target 11.0
# - SDL2 built from source to avoid Homebrew bottle compatibility drift

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SDL2_VERSION="${SDL2_VERSION:-2.30.11}"
ARCHS="${ARCHS:-arm64;x86_64}"
DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET:-11.0}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-8}"

WORK_DIR="${WORK_DIR:-${ROOT_DIR}/.build/macos-release}"
SDL2_SRC_DIR="${WORK_DIR}/SDL2-src-${SDL2_VERSION}"
SDL2_BUILD_DIR="${WORK_DIR}/SDL2-build"
SDL2_INSTALL_DIR="${WORK_DIR}/SDL2-install"
AQUARIA_BUILD_DIR="${WORK_DIR}/aquaria-build"
OUTPUT_DIR="${OUTPUT_DIR:-${ROOT_DIR}/release/macos-universal}"
OUTPUT_MACOS_DIR="${OUTPUT_DIR}/macOS"

log() {
    printf '[mac_release_bundler] %s\n' "$*"
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'Error: required command not found: %s\n' "$1" >&2
        exit 1
    }
}

require_cmd git
require_cmd cmake
require_cmd lipo
require_cmd otool

mkdir -p "${WORK_DIR}"

if [[ ! -d "${SDL2_SRC_DIR}/.git" ]]; then
    log "Cloning SDL2 release-${SDL2_VERSION} ..."
    git clone --depth 1 --branch "release-${SDL2_VERSION}" \
        https://github.com/libsdl-org/SDL.git "${SDL2_SRC_DIR}"
else
    log "Reusing existing SDL2 source at ${SDL2_SRC_DIR}"
fi

log "Configuring SDL2 (${ARCHS}, macOS ${DEPLOYMENT_TARGET}) ..."
cmake -S "${SDL2_SRC_DIR}" -B "${SDL2_BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${SDL2_INSTALL_DIR}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCHS}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
    -DSDL_TEST=OFF \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF

log "Building/installing SDL2 ..."
cmake --build "${SDL2_BUILD_DIR}" --target install -j"${JOBS}"

log "Configuring Aquaria (${ARCHS}, macOS ${DEPLOYMENT_TARGET}) ..."
env SDL2DIR="${SDL2_INSTALL_DIR}" cmake -S "${ROOT_DIR}" -B "${AQUARIA_BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCHS}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
    -DAQUARIA_USE_SDL2=ON

log "Building Aquaria ..."
cmake --build "${AQUARIA_BUILD_DIR}" -j"${JOBS}"

if [[ ! -f "${AQUARIA_BUILD_DIR}/macOS/aquaria" ]]; then
    printf 'Error: expected staged binary not found: %s\n' "${AQUARIA_BUILD_DIR}/macOS/aquaria" >&2
    exit 1
fi

SDL2_DYLIB="$(find "${AQUARIA_BUILD_DIR}/macOS" -maxdepth 1 -name 'libSDL2*.dylib' | head -n 1 || true)"
if [[ -z "${SDL2_DYLIB}" ]]; then
    printf 'Error: expected staged SDL2 dylib not found in %s\n' "${AQUARIA_BUILD_DIR}/macOS" >&2
    exit 1
fi

mkdir -p "${OUTPUT_MACOS_DIR}"
cp -f "${AQUARIA_BUILD_DIR}/macOS/aquaria" "${OUTPUT_MACOS_DIR}/aquaria"
cp -f "${SDL2_DYLIB}" "${OUTPUT_MACOS_DIR}/$(basename "${SDL2_DYLIB}")"

log "Verifying output payload ..."
lipo -info "${OUTPUT_MACOS_DIR}/aquaria"
lipo -info "${OUTPUT_MACOS_DIR}/$(basename "${SDL2_DYLIB}")"
otool -L "${OUTPUT_MACOS_DIR}/aquaria"

cat > "${OUTPUT_DIR}/BUILD_INFO.txt" <<EOF
SDL2_VERSION=${SDL2_VERSION}
ARCHS=${ARCHS}
DEPLOYMENT_TARGET=${DEPLOYMENT_TARGET}
BUILD_TYPE=${BUILD_TYPE}
SDL2DIR=${SDL2_INSTALL_DIR}
EOF

log "Done. Payload is ready at:"
log "  ${OUTPUT_MACOS_DIR}/aquaria"
log "  ${OUTPUT_MACOS_DIR}/$(basename "${SDL2_DYLIB}")"
