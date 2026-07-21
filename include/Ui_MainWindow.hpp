#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "PixhawkPanel.hpp"

class Ui_MainWindow {
public:
    QWidget *main_widget;
    QVBoxLayout *main_layout;

    QHBoxLayout *panels_layout;
    PixhawkPanel *anaRovPanel;
    PixhawkPanel *miniRovPanel;
    QPushButton *btn_theme;

    void setupUi(QWidget *MainWindow);
    void apply_styles(QWidget *MainWindow, bool dark = false);
};

#endif // UI_MAINWINDOW_H
