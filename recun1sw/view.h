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

#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>
#include <QtCharts/QChartGlobal>
#include <QLineSeries>
#include <QBarSet>
#include "callout.h"


class view : public QGraphicsView
{
    Q_OBJECT
public:
    view(QWidget *parent = 0);
    virtual ~view() = default;
    virtual void setChart(QChart *chart) = 0;

protected:
    void resizeEvent(QResizeEvent *event);
//    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event) = 0;

public slots:
    void keepCallout();

protected:
    QGraphicsSimpleTextItem *m_coordX;
    QGraphicsSimpleTextItem *m_coordY;
    QChart *m_chart;
    callout *m_tooltip;
};


// ***************** viewLineChart *****************
class viewLineChart: public view
{
    Q_OBJECT
public:
    viewLineChart(QWidget *parent = 0);
    void setChart(QChart *chart) override;

private:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void tooltip(QPointF point, bool state) ;
};


// ***************** viewBarChart *****************
class viewBarChart: public view
{
    Q_OBJECT
public:
    viewBarChart(QWidget *parent = 0);
    void setChart(QChart *chart) override;

private:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void tooltip(bool state, int index, QBarSet *barset);
};


#endif // VIEW_H
