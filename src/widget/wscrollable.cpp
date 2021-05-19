#include "widget/wscrollable.h"

#include <QList>

WScrollable::WScrollable(QWidget* pParent)
        : QScrollArea(pParent),
          WBaseWidget(this) {
}
