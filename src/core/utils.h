// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QFont>
#include <QSize>
#include <QWidget>

inline void setWidgetFont(QWidget *widget, int pointSize, bool bold) {
    auto font = widget->font();
    if (pointSize > 0)
        font.setPointSize(pointSize);
    font.setBold(bold);
    widget->setFont(font);
}

inline constexpr QSize TopIconSize(16, 16);
inline constexpr QSize TopIconButtonSize(32, 28);
inline constexpr QSize ChangelogButtonSize(92, 28);
inline constexpr QSize CaratIconSize(18, 18);
inline constexpr int WindowMargin = 14;
inline constexpr int WindowSpacing = 10;
inline constexpr int DialogMargin = 12;
inline constexpr int DialogSpacing = 8;
inline constexpr int PanelMargin = 10;
inline constexpr int TopBarSpacing = 6;
