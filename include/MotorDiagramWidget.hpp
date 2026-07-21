#ifndef MOTORDIAGRAMWIDGET_H
#define MOTORDIAGRAMWIDGET_H

#include <QWidget>
#include <array>

// 8 motorlu ROV govdesinin canli motor durumu diyagrami.
// Motor numaralandirmasi ve yerlesimi foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png
// ile birebir eslesir: 1,2 = on capraz yatay itki, 3,4 = arka capraz yatay itki,
// 5,6,7,8 = dikey (derinlik) itki motorlari. Her motor, o an gonderilen darbe
// genisligine (us) gore notr(gri) / ileri(yesil) / geri(turuncu) renklenir.
class MotorDiagramWidget : public QWidget {
    Q_OBJECT
public:
    explicit MotorDiagramWidget(QWidget *parent = nullptr);
    void set_motor_pulses(const std::array<int, 8> &pulses_us);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::array<int, 8> pulses;
};

#endif // MOTORDIAGRAMWIDGET_H
