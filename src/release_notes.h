#pragma once

#include <QtGlobal>

#include <iterator>

enum class ChangelogIcon {
    Added,
    Changed
};

struct ChangelogEntry {
    ChangelogIcon icon;
    const char *text;
};

struct ChangelogRelease {
    const char *version;
    const char *date;
    const ChangelogEntry *entries;
    qsizetype entryCount;
};

inline constexpr ChangelogEntry Changelog024[] = {
    {ChangelogIcon::Added, "Added x10 to carat purchases."},
    {ChangelogIcon::Changed, "Renamed gacha button in main menu."},
    {ChangelogIcon::Added, "Added asset overview."},
    {ChangelogIcon::Added, "Added a status bar at the bottom. It does things."},
    {ChangelogIcon::Added, "Added a session timer for the time-wonderers."},
    {ChangelogIcon::Changed, "Saves now carry magic numbers and a checksum to spot impostors."},
    {ChangelogIcon::Changed, "Gave the window a little more room to breathe."},
    {ChangelogIcon::Changed, "Some bugs fixed."},
    {ChangelogIcon::Added, "Added rainbow 67 status bar."},
};

inline constexpr ChangelogEntry Changelog023[] = {
    {ChangelogIcon::Added, "Added critical hits with firework particles."},
    {ChangelogIcon::Added, "Added a 10% income buff."},
    {ChangelogIcon::Added, "Added a Tux viewer."},
    {ChangelogIcon::Added, "Added some bulk options."},
    {ChangelogIcon::Changed, "Click button now occasionally bashes you with random text."},
};

inline constexpr ChangelogEntry Changelog022[] = {
    {ChangelogIcon::Added, "Added a second gacha slot for the brave."},
    {ChangelogIcon::Added, "Added spacebar clicks for mouse-haters."},
    {ChangelogIcon::Added, "Added a carat upgrade."},
    {ChangelogIcon::Added, "Added a button."},
    {ChangelogIcon::Added, "Added a logo click."},
    {ChangelogIcon::Changed, "Stats now borrow colors from equipped cards during buffs."},
};

inline constexpr ChangelogEntry Changelog021[] = {
    {ChangelogIcon::Added, "Added some text effects."},
    {ChangelogIcon::Added, "Added carat buff for clicks."},
    {ChangelogIcon::Added, "Added Just button test states for something."},
    {ChangelogIcon::Added, "Added second gacha card slot with penalty."},
    {ChangelogIcon::Changed, "Changed the release notes icon."},
};

inline constexpr ChangelogEntry Changelog020[] = {
    {ChangelogIcon::Added, "Added clicks show strange text sometimes."},
    {ChangelogIcon::Added, "Added an info button.."},
    {ChangelogIcon::Added, "Added Arch's and gacha cards."},
    {ChangelogIcon::Added, "Added card inventory, selection, and stacked bonuses."},
    {ChangelogIcon::Added, "Added Carat currency with a small top-bar vault."},
    {ChangelogIcon::Added, "Added a Carat window."},
    {ChangelogIcon::Added, "Added Legacy 0.1.2 mode for the old clean click loop."},
    {ChangelogIcon::Added, "Added statistics for progress, time, and a few questionable gestures."},
    {ChangelogIcon::Changed, "Improved number formatting and incremental progression."},
    {ChangelogIcon::Changed, "Added small visual polish for click interactions."},
    {ChangelogIcon::Changed, "Stats now tell the whole truth after cards quietly tip the scales."},
};

inline constexpr ChangelogEntry Changelog012[] = {
    {ChangelogIcon::Changed, "Optimized PNG assets - binary ~1 MB lighter."},
};

inline constexpr ChangelogEntry Changelog011[] = {
    {ChangelogIcon::Added, "Added compact in-game release notes."},
    {ChangelogIcon::Added, "Added colored release note markers."},
    {ChangelogIcon::Added, "Added a settings dialog."},
    {ChangelogIcon::Added, "Added reset confirmation."},
    {ChangelogIcon::Changed, "Moved reset into settings."},
    {ChangelogIcon::Changed, "Updated 0.1.1 metadata."},
};

inline constexpr ChangelogEntry Changelog010[] = {
    {ChangelogIcon::Changed, "Initial packaged version."},
};

inline constexpr ChangelogRelease ChangelogReleases[] = {
    {"0.2.4", "2026-06-11", Changelog024, std::size(Changelog024)},
    {"0.2.3", "2026-06-03", Changelog023, std::size(Changelog023)},
    {"0.2.2", "2026-06-03", Changelog022, std::size(Changelog022)},
    {"0.2.1", "2026-06-03", Changelog021, std::size(Changelog021)},
    {"0.2.0", "2026-06-03", Changelog020, std::size(Changelog020)},
    {"0.1.2", "2026-06-02", Changelog012, std::size(Changelog012)},
    {"0.1.1", "2026-06-02", Changelog011, std::size(Changelog011)},
    {"0.1.0", "2026-06-01", Changelog010, std::size(Changelog010)},
};
