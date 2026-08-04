#ifndef MOTORDIAGRAMWIDGET_H
#define MOTORDIAGRAMWIDGET_H

#include <QWidget>
#include <array>

// 8 motorlu ROV govdesinin canli motor durumu diyagrami.
// Motor numaralandirmasi ve yerlesimi foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png
// ile birebir eslesir: 1,2 = on capraz yatay itki, 3,4 = arka capraz yatay itki,
// 5,6,7,8 = dikey (derinlik) itki motorlari. Her motor, o an gonderilen darbe
// genisligine (us) gore notr(gri) / ileri(yesil) / geri(turuncu) renklenir.
//
// correction_us: sabitleme (stabilize) modunun o motora eklemis oldugu fark
// (us). Sadece dikey motorlarda (5-8) sifirdan farkli olabilir. Dolu daire
// hala GERCEK/nihai (manuel+duzeltme) darbeyi gosterir; korreksiyon aktifken
// bunun uzerine seffaf bir "hale" cizilir, boylece "bu motora otonom
// duzeltme uygulaniyor" ayri bir gostergeyle (silinmeden) belli olur.
class MotorDiagramWidget : public QWidget {
    Q_OBJECT
public:
    explicit MotorDiagramWidget(QWidget *parent = nullptr);
    void set_motor_pulses(const std::array<int, 8> &pulses_us,
                           const std::array<int, 8> &correction_us = {});

    void set_neutral_us(const std::array<int, 8> &neutral_us);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::array<int, 8> pulses;
    std::array<int, 8> correction{};
    std::array<int, 8> neutralUs;
};

#endif // MOTORDIAGRAMWIDGET_H
