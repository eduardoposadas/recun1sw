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

#include "view.h"
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtCharts/QChart>
#include <QBarCategoryAxis>
#include <QStackedBarSeries>
#include <QtWidgets/QGraphicsTextItem>
#include "callout.h"
#include <QtGui/QMouseEvent>
#include <QDateTime>

view::view(QWidget *parent)
    : QGraphicsView(new QGraphicsScene, parent),
      m_coordX(0),
      m_coordY(0),
      m_chart(0),
      m_tooltip(nullptr)
{
    setDragMode(QGraphicsView::NoDrag);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMouseTracking(true);
}

void view::resizeEvent(QResizeEvent *event)
{
    if (scene()) {
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
         m_chart->resize(event->size());
         m_coordX->setPos(m_chart->size().width()/2 - 70, m_chart->size().height() - 20);
         m_coordY->setPos(m_chart->size().width()/2 + 70, m_chart->size().height() - 20);
         for (auto i: scene()->items())
             if (i->type() == callout::Type)
                 static_cast<callout*>(i)->updateGeometry();
    }
    QGraphicsView::resizeEvent(event);
}

//void view::wheelEvent(QWheelEvent *event)
//{
//    qreal scaleFactor;

//    if(event->angleDelta().y() > 0)
//        scaleFactor = 0.8;
//    else
//        scaleFactor = 1.25;

//    scale(scaleFactor, scaleFactor);
//    event->accept();
//}

void view::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        for (auto i: scene()->items(event->pos()))
            if (i->type() == callout::Type && ! i->pos().isNull())
            {
                delete i;
                break; // Remove only one. There may be several stacked.
            }

    QGraphicsView::mousePressEvent(event);
}

void view::keepCallout()
{
    // Just "forget" the old callout. It isn't a memory leak.
    // It may be deleted by the user in mousePressEvent()
    m_tooltip = new callout(m_chart, this);
}



// ***************** viewLineChart *****************
viewLineChart::viewLineChart(QWidget *parent)
{
    Q_UNUSED(parent)
}
void viewLineChart::setChart(QChart *chart)
{
    m_chart = chart;
    m_chart->setAcceptHoverEvents(true);
    setRenderHint(QPainter::Antialiasing);
    scene()->addItem(m_chart);

    m_coordX = new QGraphicsSimpleTextItem(m_chart);
    m_coordX->setPos(m_chart->size().width()/2 - 70, m_chart->size().height());
    m_coordX->setText("X: ");
    m_coordY = new QGraphicsSimpleTextItem(m_chart);
    m_coordY->setPos(m_chart->size().width()/2 + 70, m_chart->size().height());
    m_coordY->setText("Y: ");

    for (auto s : m_chart->series() )
    {
        connect(qobject_cast<QLineSeries*>(s), &QLineSeries::clicked,
                this,                          &viewLineChart::keepCallout);
        connect(qobject_cast<QLineSeries*>(s), &QLineSeries::hovered,
                this,                          &viewLineChart::tooltip);
    }
}

void viewLineChart::mouseMoveEvent(QMouseEvent *event)
{
    m_coordX->setText(QString("X: %1")
                      .arg(QDateTime::fromMSecsSinceEpoch(m_chart->mapToValue(event->pos()).x()).toString("dd/MM/yyyy HH:mm")));
    m_coordY->setText(QString("Y: %1")
                      .arg(m_chart->mapToValue(event->pos()).y()));
    QGraphicsView::mouseMoveEvent(event);
}

void viewLineChart::tooltip(QPointF point, bool state)
{
    if (m_tooltip == nullptr)
        m_tooltip = new callout(m_chart, this);

    if (state) {
        m_tooltip->setText(QString("X: %1 \nY: %2 ")
                           .arg(QDateTime::fromMSecsSinceEpoch(point.x()).toString("dd/MM/yyyy HH:mm"))
                           .arg(point.y()));
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}

// ***************** viewBarChart *****************
viewBarChart::viewBarChart(QWidget *parent)
{
    Q_UNUSED(parent)
}
void viewBarChart::setChart(QChart *chart)
{
    m_chart = chart;
    m_chart->setAcceptHoverEvents(true);
    setRenderHint(QPainter::Antialiasing);
    scene()->addItem(m_chart);

    m_coordX = new QGraphicsSimpleTextItem(m_chart);
    m_coordX->setPos(m_chart->size().width()/2 - 70, m_chart->size().height());
    m_coordX->setText("X: ");
    m_coordY = new QGraphicsSimpleTextItem(m_chart);
    m_coordY->setPos(m_chart->size().width()/2 + 70, m_chart->size().height());
    m_coordY->setText("Y: ");

    for (auto s : m_chart->series() )
    {
        connect(qobject_cast<QStackedBarSeries*>(s), &QStackedBarSeries::clicked,
                this,                                &viewBarChart::keepCallout);
        connect(qobject_cast<QStackedBarSeries*>(s), &QStackedBarSeries::hovered,
                this,                                &viewBarChart::tooltip);
    }
}

void viewBarChart::mouseMoveEvent(QMouseEvent *event)
{
    QString xText;
    qreal x = m_chart->mapToValue(event->pos()).x();
    qreal y = m_chart->mapToValue(event->pos()).y();
    QBarCategoryAxis *horizontalAxis = static_cast<QBarCategoryAxis*>(m_chart->axes(Qt::Horizontal).at(0));

    if (horizontalAxis->count() == 0)
        xText = "";
    else
    {
        int xRounded = qRound(x);
        if (xRounded < 0) xRounded = 0;
        if (xRounded >= horizontalAxis->count()) xRounded = horizontalAxis->count() - 1;
        xText = horizontalAxis->categories().at(xRounded);
    }

    m_coordX->setText(QString("X: %1").arg(xText));
    m_coordY->setText(QString("Y: %1").arg(y));
    QGraphicsView::mouseMoveEvent(event);
}

void viewBarChart::tooltip(bool state, int index, QBarSet *barset)
{
    if (m_tooltip == nullptr)
        m_tooltip = new callout(m_chart, this);

    if (state) {
        m_tooltip->setText(QString("%1 \nX: %2 \nY: %3 ")
                           .arg(barset->label(),
                                static_cast<QBarCategoryAxis*>(m_chart->axes(Qt::Horizontal).at(0))->categories().at(index),
                                QString::number(barset->at(index))));

        auto point = QCursor::pos();
        point = this->mapFromGlobal(point);
        auto pointF = this->mapToScene(point);
        pointF = m_chart->mapFromScene(point);
        pointF = m_chart->mapToValue(point);
        m_tooltip->setAnchor(pointF);

        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}
