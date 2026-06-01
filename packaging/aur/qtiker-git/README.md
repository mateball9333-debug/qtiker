# qtiker-git AUR package

This directory contains the AUR package files for the Git version of Qtiker.

Expected upstream:

```text
https://github.com/mateball9333-debug/qtiker
```

Update `.SRCINFO` after changing `PKGBUILD`:

```bash
makepkg --printsrcinfo > .SRCINFO
```

Test locally on Arch:

```bash
makepkg -si
```
