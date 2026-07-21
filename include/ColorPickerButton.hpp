#ifndef COLORPICKERBUTTON_H
#define COLORPICKERBUTTON_H

#include <QPushButton>
#include <QColor>

// Renk secici buton widget'i
class ColorPickerButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorPickerButton(QWidget *parent = nullptr);
    void setColor(const QColor &c);
signals:
    void colorChosen(const QColor &c);
private slots:
    void on_clicked();
private:
    QColor m_color;
    void refreshStyle();
};

#endif // COLORPICKERBUTTON_H
