# Qtiker

Qtiker is a small Qt Widgets clicker game for Linux.

It keeps the interface compact: click, buy simple upgrades, get passive income,
and keep progress between launches through `QSettings`.

## Build and Run

Requirements:

- CMake 3.16+
- C++ compiler
- Qt 6 Widgets

From the project directory:

```bash
./qtiker.sh run
```

This configures CMake, builds the app, installs/updates it into `~/.local`,
and runs the installed binary.

Useful commands:

```bash
./qtiker.sh build    # compile only
./qtiker.sh install  # install into ~/.local
./qtiker.sh clean    # remove ./build
```

## Install Layout

Local install writes:

```text
~/.local/bin/qtiker
~/.local/share/applications/qtiker.desktop
~/.local/share/icons/hicolor/64x64/apps/qtiker.png
~/.local/share/icons/hicolor/256x256/apps/qtiker.png
~/.local/share/metainfo/io.github.mateball9333.qtiker.metainfo.xml
```

## Easter Egg

Middle-click the `Click` button and release it to open a small Tux window.

## Packaging

An AUR package skeleton is in:

```text
packaging/aur/qtiker-git/
```

Before publishing to AUR, push this project to:

```text
https://github.com/mateball9333-debug/qtiker
```

Then test the package on Arch:

```bash
cd packaging/aur/qtiker-git
makepkg -si
```

## License

Qtiker source code is licensed under GPL-2.0-or-later. See `LICENSE`.

Third-party asset notes are in `THIRD_PARTY.md`.
