#ifndef TIMERBUTTON_H
#define TIMERBUTTON_H
#include <QTimer>
#include <QPushButton>
#include <QMouseEvent>
#include <QDebug>

class TimerButton :public QPushButton
{
public:
    TimerButton(QWidget *parent = nullptr);
    ~TimerButton();
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QTimer *_timer;
    int _counter;
};

#endif // TIMERBUTTON_H
