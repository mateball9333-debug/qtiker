# Packaging Qtiker

This repository already has a GitHub upstream:

```text
https://github.com/mateball9333-debug/qtiker
```

Keep generated build outputs out of git. Local CMake builds belong in `build/`;
AUR build outputs belong under `packaging/aur/qtiker-git/` and are ignored.

## Local Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The helper script can build, install to `~/.local`, refresh desktop metadata,
and run the app:

```bash
./qtiker.sh run
```

## Release Checklist

1. Update `project(... VERSION ...)` in `CMakeLists.txt`.
2. Update `AppVersion` in `src/appversion.h`.
3. Commit the release changes.
4. Tag the commit, for example `git tag v0.2.0`.
5. Push both the branch and the tag.

## AUR Package

The AUR files live in:

```text
packaging/aur/qtiker-git/
```

Regenerate `.SRCINFO` after changing `PKGBUILD`:

```bash
cd packaging/aur/qtiker-git
makepkg --printsrcinfo > .SRCINFO
```

Build locally:

```bash
makepkg -si
```

Publish by copying `PKGBUILD` and `.SRCINFO` into a checkout of the AUR repo:

```bash
git clone ssh://aur@aur.archlinux.org/qtiker-git.git /tmp/qtiker-git-aur
cp packaging/aur/qtiker-git/PKGBUILD packaging/aur/qtiker-git/.SRCINFO /tmp/qtiker-git-aur/
cd /tmp/qtiker-git-aur
git add PKGBUILD .SRCINFO
git commit -m "Update qtiker-git"
git push
```
