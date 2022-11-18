/****************************************************************************
 * recun1sw: Reverse Engineering Cubot N1 Smart Watch
 *
 * Copyright (C) 2022 Eduardo Posadas Fernandez
 *
 * This file is part of recun1sw.
 *
 * Recun1sw is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Recun1sw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
****************************************************************************/

/****************************************************************************
 * This file is a modified version of an example of the Qt Charts module of
 * the Qt Toolkit. The original can be downloaded from:
 * https://code.qt.io/cgit/qt/qtcharts.git/tree/examples/charts/callout?h=6.2
****************************************************************************/

#ifndef CALLOUT_H
#define CALLOUT_H

#include <QGraphicsItem>
#include <QtCharts/QChartGlobal>
#include <QtGui/QFont>

//#include "view.h"

class view;

QT_BEGIN_NAMESPACE
class QChart;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class callout : public QGraphicsItem
{
public:
    enum { Type = QGraphicsItem::UserType + 1 };

    callout(QChart *parent, view *v);

    int type() const override {return Type;}
    void setText(const QString &text);
    void setAnchor(QPointF point);
    void updateGeometry();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString m_text;
    QRectF m_textRect;
    QRectF m_rect;
    QPointF m_anchor;
    QFont m_font;
    QChart *m_chart;
    view *m_view;
};


#endif // CALLOUT_H
