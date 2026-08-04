#include "MotorDiagramWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QPolygonF>
#include <QFont>
#include <algorithm>
#include <cstdlib>

namespace {
constexpr std::array<int, 8> DEFAULT_NEUTRAL_US = {1487, 1487, 1487, 1487, 1487, 1487, 1487, 1487};
constexpr int MIN_US = 1000;
constexpr int MAX_US = 2000;

struct MotorPos { int number; qreal fx, fy; };

// foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png ile birebir motor yerlesimi
const MotorPos kMotorPositions[8] = {
    {1, 0.70, 0.15}, {2, 0.30, 0.15},   // on capraz yatay itki
    {3, 0.70, 0.85}, {4, 0.30, 0.85},   // arka capraz yatay itki
    {5, 0.88, 0.35}, {6, 0.12, 0.35},   // dikey itki (on taraf)
    {7, 0.88, 0.65}, {8, 0.12, 0.65},   // dikey itki (arka taraf)
};
}

MotorDiagramWidget::MotorDiagramWidget(QWidget *parent)
    : QWidget(parent), neutralUs(DEFAULT_NEUTRAL_US) {
    pulses = neutralUs;
    setMinimumSize(150, 150);
    setMaximumSize(220, 220);
}

void MotorDiagramWidget::set_motor_pulses(const std::array<int, 8> &pulses_us,
                                           const std::array<int, 8> &correction_us) {
    pulses = pulses_us;
    correction = correction_us;
    update();
}

void MotorDiagramWidget::set_neutral_us(const std::array<int, 8> &neutral_us) {
    neutralUs = neutral_us;
    update();
}

void MotorDiagramWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal w = width();
    qreal h = height();

    // Govde (oktagon)
    QPolygonF body;
    body << QPointF(w * 0.30, h * 0.04) << QPointF(w * 0.70, h * 0.04)
         << QPointF(w * 0.96, h * 0.28) << QPointF(w * 0.96, h * 0.72)
         << QPointF(w * 0.70, h * 0.96) << QPointF(w * 0.30, h * 0.96)
         << QPointF(w * 0.04, h * 0.72) << QPointF(w * 0.04, h * 0.28);
    painter.setBrush(QColor(110, 114, 122));
    painter.setPen(QPen(QColor(45, 48, 54), 2));
    painter.drawPolygon(body);

    // On gosterge ucgeni
    painter.setBrush(QColor(230, 80, 60));
    painter.setPen(Qt::NoPen);
    QPolygonF frontTri;
    frontTri << QPointF(w * 0.5, h * 0.02) << QPointF(w * 0.42, h * 0.15) << QPointF(w * 0.58, h * 0.15);
    painter.drawPolygon(frontTri);

    qreal r = std::min(w, h) * 0.115;
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSizeF(std::max<qreal>(7.0, r * 0.75));
    painter.setFont(font);

    for (const MotorPos &mp : kMotorPositions) {
        int pulse = pulses[mp.number - 1];
        int neutral = neutralUs[mp.number - 1];
        QColor color;
        if (pulse > neutral + 15) {
            double t = std::min(1.0, double(pulse - neutral) / double(MAX_US - neutral));
            color = QColor(30, static_cast<int>(140 + 90 * t), 70); // notrden ileriye: yesil
        } else if (pulse < neutral - 15) {
            double t = std::min(1.0, double(neutral - pulse) / double(neutral - MIN_US));
            color = QColor(static_cast<int>(200 + 40 * t), static_cast<int>(110 - 40 * t), 30); // geri: turuncu/kirmizi
        } else {
            color = QColor(150, 154, 160); // notr: gri
        }

        QPointF center(w * mp.fx, h * mp.fy);
        painter.setBrush(color);
        painter.setPen(QPen(QColor(30, 30, 30), 2));
        painter.drawEllipse(center, r, r);

        painter.setPen(Qt::white);
        painter.drawText(QRectF(center.x() - r, center.y() - r, r * 2, r * 2),
                          Qt::AlignCenter, QString::number(mp.number));

        // Sabitleme (stabilize) modunun bu motora eklemis oldugu duzeltme:
        // dolu daire (yukarida) her zaman gercek/nihai sinyali gosterir; bu
        // seffaf hale sadece "burada otonom bir duzeltme aktif" bilgisini
        // ekler - buyuklugu duzeltmenin siddetine gore olceklenir.
        int corr = correction[mp.number - 1];
        if (corr != 0) {
            double mag = std::min(1.0, std::abs(corr) / double(MAX_US - neutral));
            qreal haloR = r * (1.25 + 0.5 * mag);
            QColor halo(80, 200, 255, static_cast<int>(60 + 90 * mag)); // seffaf camgobegi
            painter.setBrush(halo);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(center, haloR, haloR);

            // Dolu daireyi hale ustunde tekrar ciz, altta kalmasin
            painter.setBrush(color);
            painter.setPen(QPen(QColor(30, 30, 30), 2));
            painter.drawEllipse(center, r, r);
            painter.setPen(Qt::white);
            painter.drawText(QRectF(center.x() - r, center.y() - r, r * 2, r * 2),
                              Qt::AlignCenter, QString::number(mp.number));
        }
    }
}
