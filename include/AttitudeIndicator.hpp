#ifndef ATTITUDEINDICATOR_H
#define ATTITUDEINDICATOR_H

#include <QWidget>

// Suni ufuk (attitude indicator) - yuvarlak, mavi/kahverengi yarim kure + sari ust ucgen + turuncu ucak
class AttitudeIndicator : public QWidget {
    Q_OBJECT
public:
    explicit AttitudeIndicator(QWidget *parent = nullptr);
    void set_attitude(float roll, float pitch);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float roll;
    float pitch;
};

#endif // ATTITUDEINDICATOR_H
