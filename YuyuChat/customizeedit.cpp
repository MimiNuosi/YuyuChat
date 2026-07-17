#include "customizeedit.h"


CustomizeEdit::CustomizeEdit(QWidget *parent) : QLineEdit(parent) {}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent *event) {
    QLineEdit::focusOutEvent(event);
    emit sig_foucus_out();
}