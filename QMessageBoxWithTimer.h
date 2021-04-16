#pragma once

#include <QMessageBox>
#include <QTimer>

class QMessageBoxWithTimer
{
public:
    QMessageBoxWithTimer(QMessageBox *box);
    ~QMessageBoxWithTimer();

private:
    QTimer* m_timer = nullptr;
    QMessageBox* m_box = nullptr;
};

