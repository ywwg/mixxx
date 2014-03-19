/***************************************************************************
                          wstatuslight.cpp  -  description
                             -------------------
    begin                : Wed May 30 2007
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
                           (C) 2007 by John Sully (converted from WVumeter)
    email                : jsully@scs.ryerson.ca
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "widget/wstatuslight.h"

#include <QPaintEvent>
#include <QStylePainter>
#include <QStyleOption>
#include <QtDebug>
#include <QPixmap>

WStatusLight::WStatusLight(QWidget * parent)
        : WWidget(parent),
          m_iPos(0) {
    setNoPos(0);
}

WStatusLight::~WStatusLight() {
}

void WStatusLight::setNoPos(int iNoPos) {
    // If pixmap array is already allocated, delete it.
    if (!m_pixmaps.empty()) {
        // Clear references to existing pixmaps.
        m_pixmaps.resize(0);
    }

    // values less than 2 make no sense (need at least off, on)
    if (iNoPos < 2) {
        iNoPos = 2;
    }
    m_pixmaps.resize(iNoPos);
}

WStatusLight::SizeMode WStatusLight::SizeModeFromString(QString str) {
    if (str.toUpper() == "FIXED" ) {
        return FIXED;
    } else if (str.toUpper() == "RESIZE") {
        return RESIZE;
    }
    qWarning() << "Unknown status light size mode " << str << ", using FIXED";
    return FIXED;
}

void WStatusLight::setup(QDomNode node, const SkinContext& context) {
    // Number of states. Add one to account for the background.
    setNoPos(context.selectInt(node, "NumberPos") + 1);

    // Set pixmaps
    for (int i = 0; i < m_pixmaps.size(); ++i) {
        // Accept either PathStatusLight or PathStatusLight1 for value 1,
        QString nodeName = QString("PathStatusLight%1").arg(i);
        if (context.hasNode(node, nodeName)) {
            QString mode = context.selectAttributeString(
                context.selectElement(node, nodeName), "sizemode", "FIXED");
            setPixmap(i, context.getSkinPath(context.selectString(node, nodeName)),
                                             SizeModeFromString(mode));
        } else if (i == 0 && context.hasNode(node, "PathBack")) {
            QString mode = context.selectAttributeString(
                context.selectElement(node, "PathBack"), "sizemode", "FIXED");
            setPixmap(i, context.getSkinPath(context.selectString(node, "PathBack")),
                                             SizeModeFromString(mode));
        } else if (i == 1 && context.hasNode(node, "PathStatusLight")) {
            QString mode = context.selectAttributeString(
                context.selectElement(node, "PathStatusLight"), "sizemode", "FIXED");
            setPixmap(i, context.getSkinPath(context.selectString(node, "PathStatusLight")),
                                             SizeModeFromString(mode));
        } else {
            m_pixmaps[i].clear();
        }
    }
}

void WStatusLight::setPixmap(int iState, const QString& filename, SizeMode mode) {
    if (iState < 0 || iState >= m_pixmaps.size()) {
        return;
    }

    PaintablePointer pPixmap = WPixmapStore::getPaintable(filename,
                                                          Paintable::STRETCH);

    if (!pPixmap.isNull() && !pPixmap->isNull()) {
        m_pixmaps[iState] = pPixmap;

        switch (mode) {
            case RESIZE:
                // Allow the pixmap to stretch or tile.
                setBaseSize(pPixmap->size());
                break;
            case FIXED:
                // Set size of widget equal to pixmap size
                setFixedSize(pPixmap->size());
                break;
            default:
                setFixedSize(pPixmap->size());
                break;
        }
    } else {
        qDebug() << "WStatusLight: Error loading pixmap:" << filename << iState;
        m_pixmaps[iState].clear();
    }
}

void WStatusLight::onConnectedControlValueChanged(double v) {
    int val = static_cast<int>(v);
    if (m_iPos == val) {
        return;
    }

    if (m_pixmaps.size() == 2) {
        // original behavior for two-state lights: any non-zero value is "on"
        m_iPos = val > 0 ? 1 : 0;
        update();
    } else if (val < m_pixmaps.size() && val >= 0) {
        // multi-state behavior: values must be correct
        m_iPos = val;
        update();
    } else {
        qDebug() << "Warning: wstatuslight asked for invalid position:"
                 << val << "max val:" << m_pixmaps.size()-1;
    }
}

void WStatusLight::paintEvent(QPaintEvent *) {
    QStyleOption option;
    option.initFrom(this);
    QStylePainter p(this);
    p.drawPrimitive(QStyle::PE_Widget, option);

    if (m_iPos < 0 || m_iPos >= m_pixmaps.size()) {
        return;
    }

    PaintablePointer pPixmap = m_pixmaps[m_iPos];

    if (pPixmap.isNull() || pPixmap->isNull()) {
        return;
    }

    pPixmap->draw(0, 0, &p);
}
