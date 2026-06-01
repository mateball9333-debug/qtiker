#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
PREFIX="${QTIKER_PREFIX:-$HOME/.local}"

usage() {
    echo "Usage: ./qtiker.sh [run|build|install|update|clean]"
    echo
    echo "  run      build, install/update, and run"
    echo "  build    compile only"
    echo "  install  compile and install to ~/.local"
    echo "  update   same as install"
    echo "  clean    remove ./build"
}

configure() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX"
}

build() {
    configure
    cmake --build "$BUILD_DIR" --parallel
}

install_app() {
    build
    cmake --install "$BUILD_DIR"

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$PREFIX/share/applications" || true
    fi

    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f "$PREFIX/share/icons/hicolor" || true
    fi

    echo "Installed: $PREFIX/bin/qtiker"
}

case "${1:-run}" in
    run)
        install_app
        "$PREFIX/bin/qtiker"
        ;;
    build)
        build
        ;;
    install | update)
        install_app
        ;;
    clean)
        rm -rf "$BUILD_DIR"
        ;;
    -h | --help | help)
        usage
        ;;
    *)
        usage
        exit 1
        ;;
esac
