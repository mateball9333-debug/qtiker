#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QPointF>
#include <QTimer>

class ParticleOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ParticleOverlay(QWidget *parent);

    void burstAt(const QPointF &center, int count, bool big);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Particle {
        QPointF pos;
        QPointF vel;
        QColor color;
        float life;
        float maxLife;
        float size;
    };

    void tick();

    QVector<Particle> particles;
    QTimer timer;
};
