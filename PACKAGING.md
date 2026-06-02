# Packaging Qtiker

This project is prepared for normal Linux installation and AUR packaging.

## Local Install

```bash
./qtiker.sh run
```

This builds, installs into `~/.local`, updates desktop/icon metadata when tools
are available, and starts the app.

## GitHub Upstream

The AUR package expects this upstream URL:

```text
https://github.com/mateball9333-debug/qtiker
```

Before publishing, make sure GitHub CLI auth is fixed:

```bash
gh auth login
```

or:

```bash
gh auth refresh -h github.com
```

Then initialize and publish:

```bash
git init
git add .
git commit -m "Initial Qtiker package"
gh repo create mateball9333-debug/qtiker --public --source=. --remote=origin --push
git tag v0.1.1
git push origin v0.1.1
```

If an empty `.git` directory is present and Git says this is not a repository,
remove that empty directory first:

```bash
rmdir .git
```

Only do this if `.git` is empty.

## AUR

The AUR skeleton is here:

```text
packaging/aur/qtiker-git/
```

After the GitHub repo exists:

```bash
cd packaging/aur/qtiker-git
makepkg --printsrcinfo > .SRCINFO
makepkg -si
```

To publish to AUR:

```bash
git clone ssh://aur@aur.archlinux.org/qtiker-git.git /tmp/qtiker-git-aur
cp PKGBUILD .SRCINFO /tmp/qtiker-git-aur/
cd /tmp/qtiker-git-aur
git add PKGBUILD .SRCINFO
git commit -m "Initial import"
git push
```
