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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QTreeWidgetItem>
#include <QChart>
#include <QLineSeries>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QDateTime>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    const QString programName = "recun1sw";

private slots:
    void hostModeStateChanged(QBluetoothLocalDevice::HostMode mode);
    void reScan();
    void scanFinished();
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);

    void newBtDevice(const QBluetoothDeviceInfo &info);
    void btDeviceItemClicked(QListWidgetItem *item);
    void btDeviceConnected();
    void btDeviceDisconnected();
    void btDeviceConnectionError(QLowEnergyController::Error newError);
    void btDeviceStateChanged(QLowEnergyController::ControllerState state);

    void btServiceDiscovered(const QBluetoothUuid &newService);
    void btServiceDiscoverFinished();
    void btServiceDetailsDiscovered(QLowEnergyService::ServiceState newState);
    void btServiceItemClicked(QTreeWidgetItem *item, int column);

    void btCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void btCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void btCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void btDescriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void btDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue);
    void btErrorOccurred(QLowEnergyService::ServiceError newError);

    void HRSliderChangeValue(int value);
    void O2SliderChangeValue(int value);
    void stepsChartSliderChangeValue(int value);

private:
    enum messageType: unsigned char {heartRate   = 0xf7,
                                     heartRateRT = 0xe5,
                                     O2          = 0x34,
                                     steps       = 0xb2,
                                     otherSports = 0xfd};

    Ui::MainWindow *ui;
    int logSize;

    QChart        *heartRateChart;
    QLineSeries   *heartRateSeries;
    QDateTimeAxis *heartRateChartAxisX;
    QDateTime      heartRateChartAxisXMax;
    QDateTime      heartRateChartAxisXMin;
    QValueAxis    *heartRateChartAxisY;
    qreal          heartRateChartAxisYMax;
    qreal          heartRateChartAxisYMin;
    const quint64  heartRateChartMaxXRange = 3600 * 12; // 12 hours
    const quint64  heartRateChartHalfXRange = heartRateChartMaxXRange / 2;

    QChart        *O2Chart;
    QLineSeries   *O2Series;
    QDateTimeAxis *O2ChartAxisX;
    QDateTime      O2ChartAxisXMax;
    QDateTime      O2ChartAxisXMin;
    QValueAxis    *O2ChartAxisY;
    qreal          O2ChartAxisYMax;
    qreal          O2ChartAxisYMin;
    const quint64  O2ChartMaxXRange = 86400 * 7; // 7 days
    const quint64  O2ChartHalfXRange = O2ChartMaxXRange / 2;

    QChart            *stepsChart;
    QStackedBarSeries *stepsSeries;
    QBarSet           *stepsSeriesRunSet;
    QBarSet           *stepsSeriesWalkSet;
    QBarCategoryAxis  *stepsChartAxisX;
    QDateTime          stepsChartAxisXMax;
    QDateTime          stepsChartAxisXMin;
    QValueAxis        *stepsChartAxisY;
    qreal              stepsChartAxisYMax;
    //qreal            stepsChartAxisYMin; // It's always 0.
    const int          stepsChartMaxVisibleCategories = 14;
    const int          stepsChartHalfVisibleCategories = stepsChartMaxVisibleCategories / 2;


    QBluetoothLocalDevice *localDevice;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QHash<QString, QBluetoothDeviceInfo> remoteBtDevices;
    QLowEnergyController *controller;
    bool deviceIsCubotN1;
    QLowEnergyService *dumpService;
    QLowEnergyCharacteristic dumpCharacteristic;

    void closeEvent(QCloseEvent *event);
    void about();
    void clearCharts();
    void showMessage(QString m, bool onlyLog = false);
    void manageConnectionDisplay(bool connected);
    QByteArray makeDumpRequest(messageType type);

    void heartRateMessage(const QByteArray &dataRecived);
    void heartRateRealTimeMessage(const QByteArray &dataRecived);
    void stepsMessage(const QByteArray &dataRecived);
    void O2Message(const QByteArray &dataRecived);
    void O2MessageRealTime(const QByteArray &dataRecived);
};
#endif // MAINWINDOW_H
