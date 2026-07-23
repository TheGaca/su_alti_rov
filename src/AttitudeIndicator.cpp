#include "AttitudeIndicator.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPolygonF>
#include <QFont>
#include <cmath>

AttitudeIndicator::AttitudeIndicator(QWidget *parent)
    : QWidget(parent), roll(0.0f), pitch(0.0f) {
    setMinimumSize(260, 260);   // 180 → 260
    setMaximumSize(280, 280);
}

void AttitudeIndicator::set_attitude(float r, float p) {
    roll = r;
    pitch = p;
    update();
}

void AttitudeIndicator::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    QPointF center(width() / 2.0, height() / 2.0);
    qreal radius = side / 2.0 - 6;

    QPainterPath clipPath;
    clipPath.addEllipse(center, radius, radius);
    painter.setClipPath(clipPath);

    painter.save();
    painter.translate(center);
    // NOT: Kullanici testinde sola/saga yatirma gorseldeki donme yonuyle ters
    // geliyordu; isaret ceviridi (eskiden -roll idi).
    painter.rotate(roll);

    qreal pitchOffset = pitch * (radius / 45.0);

    // Gökyüzü
    QRectF skyRect(-radius * 2, -radius * 2 + pitchOffset, radius * 4, radius * 2);
    painter.fillRect(skyRect, QColor(94, 168, 218));
    // Zemin
    QRectF groundRect(-radius * 2, pitchOffset, radius * 4, radius * 2);
    painter.fillRect(groundRect, QColor(133, 80, 40));
    // Ufuk çizgisi
    painter.setPen(QPen(Qt::white, 2.5));
    painter.drawLine(QPointF(-radius * 2, pitchOffset), QPointF(radius * 2, pitchOffset));

    // Pitch işaretleri
    painter.setPen(QPen(Qt::white, 2));
    QFont f("Sans", 10, QFont::Bold);
    painter.setFont(f);
    for (int p = -30; p <= 30; p += 10) {
        if (p == 0) continue;
        qreal y = pitchOffset - p * (radius / 45.0);
        qreal lineLen = (p % 20 == 0) ? radius * 0.50 : radius * 0.34;
        painter.drawLine(QPointF(-lineLen / 2, y), QPointF(lineLen / 2, y));
        QString txt = QString::number(std::abs(p));
        painter.drawText(QPointF(-lineLen / 2 - 28, y + 5), txt);
        painter.drawText(QPointF(lineLen / 2 + 8, y + 5), txt);
    }

    painter.restore();

    // Sarı üst üçgen
    painter.setClipping(false);
    painter.setBrush(QColor(255, 215, 0));
    painter.setPen(Qt::NoPen);
    QPolygonF topTri;
    topTri << QPointF(center.x(), center.y() - radius + 6)
           << QPointF(center.x() - 13, center.y() - radius + 24)
           << QPointF(center.x() + 13, center.y() - radius + 24);
    painter.drawPolygon(topTri);

    // Turuncu uçak (daha büyük)
    painter.setBrush(QColor(255, 140, 0));
    painter.setPen(QPen(Qt::black, 1.5));
    QPolygonF plane;
    plane << QPointF(center.x() - 30, center.y())
          << QPointF(center.x() - 8, center.y())
          << QPointF(center.x(), center.y() + 10)
          << QPointF(center.x() + 8, center.y())
          << QPointF(center.x() + 30, center.y())
          << QPointF(center.x() + 30, center.y() + 5)
          << QPointF(center.x() + 8, center.y() + 5)
          << QPointF(center.x() + 8, center.y() + 16)
          << QPointF(center.x() - 8, center.y() + 16)
          << QPointF(center.x() - 8, center.y() + 5)
          << QPointF(center.x() - 30, center.y() + 5);
    painter.drawPolygon(plane);

    // Dış çerçeve
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(40, 40, 40), 3));
    painter.drawEllipse(center, radius, radius);
}
