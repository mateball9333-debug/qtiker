// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QBuffer>
#include <QColor>
#include <QIcon>
#include <QIODevice>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QSvgRenderer>

inline QIcon tintedSvgIcon(const QString &path, const QColor &color, const QSize &size) {
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QSvgRenderer renderer(path);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();

    return QIcon(pixmap);
}

inline QString tintedSvgDataUri(const QString &path, const QColor &color, const QSize &size) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    QSvgRenderer renderer(path);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), color);
    painter.end();

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    return QString("data:image/png;base64,%1").arg(QString::fromLatin1(bytes.toBase64()));
}
