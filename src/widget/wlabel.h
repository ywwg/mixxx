/***************************************************************************
                          wlabel.h  -  description
                             -------------------
    begin                : Wed Jan 5 2005
    copyright            : (C) 2003 by Tue Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WLABEL_H
#define WLABEL_H

#include <QLabel>

#include "widget/wwidget.h"

class WLabel : public WWidget {
    Q_OBJECT
  public:
    WLabel(QWidget* pParent=NULL);
    virtual ~WLabel();

    virtual void setup(QDomNode node);
    virtual QWidget* getComposedWidget() { return m_pLabel; }

  protected:
    QLabel* m_pLabel;
    QString m_qsText;
    // Foreground and background colors.
    QColor m_qFgColor;
    QColor m_qBgColor;
};

#endif
