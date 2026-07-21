#include "ColorPickerButton.hpp"

#include <QColorDialog>

ColorPickerButton::ColorPickerButton(QWidget *parent)
    : QPushButton(parent), m_color(QColor("#9C27B0")) {
    setMinimumHeight(34);
    setText("Renk Seç");
    setCursor(Qt::PointingHandCursor);
    refreshStyle();
    connect(this, &QPushButton::clicked, this, &ColorPickerButton::on_clicked);
}

void ColorPickerButton::setColor(const QColor &c) {
    m_color = c;
    refreshStyle();
}

void ColorPickerButton::on_clicked() {
    QColor c = QColorDialog::getColor(m_color, this, "Renk Seç");
    if (c.isValid()) {
        m_color = c;
        refreshStyle();
        emit colorChosen(m_color);
    }
}

void ColorPickerButton::refreshStyle() {
    // Renk koyuysa yazı beyaz, açıksa siyah
    int luma = (m_color.red() * 299 + m_color.green() * 587 + m_color.blue() * 114) / 1000;
    QString txtColor = (luma < 128) ? "#ffffff" : "#000000";
    setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 2px solid #333;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "  padding: 4px 12px;"
        "}"
        "QPushButton:hover { border: 2px solid #000; }"
    ).arg(m_color.name(), txtColor));
    setText(QString("Renk: %1").arg(m_color.name().toUpper()));
}
