#include "QMessageBoxWithTimer.h"


QMessageBoxWithTimer::QMessageBoxWithTimer(QMessageBox *box)
    :m_box(box)
{
    m_timer = new QTimer(m_box);
    QObject::connect(m_timer, SIGNAL(timeout()), m_box, SLOT(accept()));
    m_timer->start(5000);
}


QMessageBoxWithTimer::~QMessageBoxWithTimer()
{

}
