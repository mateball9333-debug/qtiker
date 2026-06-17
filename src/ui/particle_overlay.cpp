// SPDX-License-Identifier: GPL-2.0-or-later
#include "particle_overlay.h"

#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

static constexpr int TickMs = 16;

ParticleOverlay::ParticleOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    timer.setInterval(TickMs);
    connect(&timer, &QTimer::timeout, this, &ParticleOverlay::tick);
}

void ParticleOverlay::burstAt(const QPointF &center, int count, bool big) {
    auto *rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = center;

        const float angle = rng->bounded(360) * M_PI / 180.0f;
        const float speed = big
            ? 4.0f + rng->bounded(1000) / 100.0f
            : 3.0f + rng->bounded(700) / 100.0f;
        p.vel = QPointF(qCos(angle) * speed, qSin(angle) * speed);

        if (big) {
            const int roll = rng->bounded(100);
            if (roll < 30) {
                p.color = QColor(255, 255, 200);
            } else if (roll < 60) {
                p.color = QColor(255, 200 + rng->bounded(56), 50);
            } else {
                p.color = QColor(255, 100 + rng->bounded(80), 0);
            }
        } else {
            const int roll = rng->bounded(100);
            if (roll < 40) {
                p.color = QColor(255, 230, 120);
            } else {
                p.color = QColor(255, 180 + rng->bounded(76), 40);
            }
        }

        p.maxLife = big ? 0.8f + rng->bounded(600) / 1000.0f
                        : 0.6f + rng->bounded(400) / 1000.0f;
        p.life = p.maxLife;
        p.size = big ? 5.0f + rng->bounded(80) / 10.0f
                     : 3.5f + rng->bounded(60) / 10.0f;

        particles.append(p);
    }

    if (!timer.isActive()) {
        timer.start();
    }
    update();
}

void ParticleOverlay::tick() {
    const float dt = TickMs / 1000.0f;

    for (int i = particles.size() - 1; i >= 0; --i) {
        auto &p = particles[i];
        p.pos += p.vel;
        p.vel.setY(p.vel.y() + 6.0f * dt);
        p.vel *= 0.995f;
        p.life -= dt;

        if (p.life <= 0) {
            particles.removeAt(i);
        }
    }

    if (particles.isEmpty()) {
        timer.stop();
    }

    update();
}

void ParticleOverlay::paintEvent(QPaintEvent *) {
    if (particles.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const auto &p : particles) {
        const float t = p.life / p.maxLife;
        const float radius = p.size * t;
        if (radius < 0.5f) continue;

        QColor glow = p.color;
        glow.setAlphaF(t * 0.25f);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(p.pos, radius * 3.0f, radius * 3.0f);

        QColor core = p.color;
        core.setAlphaF(t);
        painter.setBrush(core);
        painter.drawEllipse(p.pos, radius, radius);

        QColor hot(255, 255, 255);
        hot.setAlphaF(t * 0.7f);
        painter.setBrush(hot);
        painter.drawEllipse(p.pos, radius * 0.4f, radius * 0.4f);
    }
}
