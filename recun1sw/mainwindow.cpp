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

#include <QMetaEnum>
//#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , logSize(1000)
    , controller(nullptr)
    , deviceIsCubotN1(false)
{
    ui->setupUi(this);
    ui->plainTextEdit->setMaximumBlockCount(logSize);
    setWindowTitle(programName);

    connect(ui->actionexit, &QAction::triggered,
            this,           &MainWindow::close);
    connect(ui->actionAbout, &QAction::triggered,
            this,           &MainWindow::about);
    connect(ui->actionRescan, &QAction::triggered,
            this,             &MainWindow::reScan);
    connect(ui->deviceListWidget, &QListWidget::itemClicked,
            this,                 &MainWindow::btDeviceItemClicked);

    ui->servicesTreeWidget->setHeaderLabels(QStringList({"Service", "Description", "Read", "Write",
                                                         "Notify", "Indicate", "ExtendedProperty",
                                                         "Broadcast", "WriteNoResp", "WriteSigned"}));
    connect(ui->servicesTreeWidget , &QTreeWidget::itemClicked,
            this,                    &MainWindow::btServiceItemClicked);


    // Heart Rate Chart
    heartRateSeries = new QLineSeries();
    heartRateSeries->setPointsVisible(true);
    heartRateChart = new QChart();
    heartRateChart->addSeries(heartRateSeries);
    heartRateChart->legend()->hide();

    heartRateChartAxisX = new QDateTimeAxis;
    heartRateChartAxisX->setTickCount(20);
    heartRateChartAxisX->setFormat("dd/MM/yyyy HH:mm");
    heartRateChartAxisX->setLabelsAngle(290);
    heartRateChart->addAxis(heartRateChartAxisX, Qt::AlignBottom);

    heartRateChartAxisY = new QValueAxis;
    heartRateChartAxisY->setLabelFormat("%i ");
    heartRateChartAxisY->setTitleText(tr("Beats"));
    heartRateChart->addAxis(heartRateChartAxisY, Qt::AlignLeft);

    ui->HRChartGraphicsView->setChart(heartRateChart);
    connect(ui->HRChartSlider, &QSlider::valueChanged,
            this,              &MainWindow::HRSliderChangeValue);

    // 02 Chart
    O2Series = new QLineSeries();
    O2Series->setPointsVisible(true);
    O2Chart = new QChart();
    O2Chart->addSeries(O2Series);
    O2Chart->legend()->hide();

    O2ChartAxisX = new QDateTimeAxis;
    O2ChartAxisX->setTickCount(20);
    O2ChartAxisX->setFormat("dd/MM/yyyy HH:mm");
    O2ChartAxisX->setLabelsAngle(290);
    O2Chart->addAxis(O2ChartAxisX, Qt::AlignBottom);

    O2ChartAxisY = new QValueAxis;
    O2ChartAxisY->setLabelFormat("%i ");
    O2ChartAxisY->setTitleText(tr("Peripheral O2 saturation"));
    O2Chart->addAxis(O2ChartAxisY, Qt::AlignLeft);

    ui->O2ChartGraphicsView->setChart(O2Chart);
    connect(ui->O2ChartSlider, &QSlider::valueChanged,
            this,              &MainWindow::O2SliderChangeValue);

    // Steps Chart
    stepsSeries = new QStackedBarSeries();
    stepsChart = new QChart();
    stepsChart->addSeries(stepsSeries);
    stepsChart->legend()->setVisible(true);
    stepsChart->legend()->setAlignment(Qt::AlignBottom);

    stepsChartAxisX = new QBarCategoryAxis();
    stepsChartAxisX->setLabelsAngle(290);
    stepsChart->addAxis(stepsChartAxisX, Qt::AlignBottom);
    stepsSeries->attachAxis(stepsChartAxisX);

    stepsChartAxisY = new QValueAxis;
    stepsChartAxisY->setLabelFormat("%i");
    stepsChartAxisY->setTitleText(tr("Steps"));
    stepsChart->addAxis(stepsChartAxisY, Qt::AlignLeft);
    stepsSeries->attachAxis(stepsChartAxisY);

    ui->stepsChartGraphicsView->setChart(stepsChart);
    connect(ui->stepsChartSlider, &QSlider::valueChanged,
            this,                 &MainWindow::stepsChartSliderChangeValue);


    clearCharts();

    // Allow the user to choose the BT interface?
    localDevice = new QBluetoothLocalDevice();
    connect(localDevice, &QBluetoothLocalDevice::hostModeStateChanged,
            this,        &MainWindow::hostModeStateChanged);

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this,           &MainWindow::newBtDevice);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this,           &MainWindow::scanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this,           &MainWindow::scanError);

    reScan();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, tr("Exit"),
                                                                tr("Are you sure?"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        if (controller)
            controller->disconnectFromDevice();
        event->accept();
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About ") + programName, tr("Reverse Engineering Cubot N1 Smart Watch\nhttps://github.com/eduardoposadas/recun1sw\n\
Distributed under GPL 3 License."));
}

void MainWindow::clearCharts()
{
    heartRateSeries->clear();
    for (auto axis : heartRateSeries->attachedAxes())
        heartRateSeries->detachAxis(axis);
    heartRateChartAxisXMax = QDateTime(QDate(1970, 1, 1), QTime(0, 0));
    heartRateChartAxisXMin = QDateTime(QDate(3000, 1, 1), QTime(0, 0));
    heartRateChartAxisX->setRange(heartRateChartAxisXMax, heartRateChartAxisXMax);
    heartRateSeries->attachAxis(heartRateChartAxisX);
    heartRateChartAxisYMin = std::numeric_limits<decltype(heartRateChartAxisYMax)>::min();
    heartRateChartAxisYMin = std::numeric_limits<decltype(heartRateChartAxisYMin)>::max();
    heartRateChartAxisY->setRange(0, 0);
    heartRateSeries->attachAxis(heartRateChartAxisY);
    ui->HRChartSlider->setMinimum(0);
    ui->HRChartSlider->setMaximum(0);

    O2Series->clear();
    for (auto axis : O2Series->attachedAxes())
        O2Series->detachAxis(axis);
    O2ChartAxisXMax = QDateTime(QDate(1970, 1, 1), QTime(0, 0));
    O2ChartAxisXMin = QDateTime(QDate(3000, 1, 1), QTime(0, 0));
    O2ChartAxisX->setRange(O2ChartAxisXMax, O2ChartAxisXMax);
    O2Series->attachAxis(O2ChartAxisX);
    O2ChartAxisYMax = std::numeric_limits<decltype(O2ChartAxisYMax)>::min();
    O2ChartAxisYMin = std::numeric_limits<decltype(O2ChartAxisYMin)>::max();
    O2ChartAxisY->setRange(0, 0);
    O2Series->attachAxis(O2ChartAxisY);
    ui->O2ChartSlider->setMinimum(0);
    ui->O2ChartSlider->setMaximum(0);

    stepsSeries->clear();
    stepsSeriesRunSet = new QBarSet(tr("Run"));
    stepsSeriesWalkSet = new QBarSet(tr("Walk"));
    stepsSeries->append(stepsSeriesRunSet);
    stepsSeries->append(stepsSeriesWalkSet);
    stepsChartAxisXMax = QDateTime(QDate(1970, 1, 1), QTime(0, 0));
    stepsChartAxisXMin = QDateTime(QDate(3000, 1, 1), QTime(0, 0));
    stepsChartAxisX->clear();
    stepsChartAxisYMax = std::numeric_limits<decltype(stepsChartAxisYMax)>::min();
    stepsChartAxisY->setRange(0, 0);

}
void MainWindow::HRSliderChangeValue(int value)
{
    QDateTime min, max;
    min.setSecsSinceEpoch(value - heartRateChartHalfXRange);
    max.setSecsSinceEpoch(value + heartRateChartHalfXRange);
    heartRateChartAxisX->setRange(min, max);
}

void MainWindow::O2SliderChangeValue(int value)
{
    QDateTime min, max;
    min.setSecsSinceEpoch(value - O2ChartHalfXRange);
    max.setSecsSinceEpoch(value + O2ChartHalfXRange);
    O2ChartAxisX->setRange(min, max);
}

void MainWindow::stepsChartSliderChangeValue(int value)
{
    const qsizetype catSize = stepsChartAxisX->count() - 1;

    if (catSize == -1)
        return;
    if (catSize < stepsChartMaxVisibleCategories)
    {
        stepsChartAxisX->setRange(stepsChartAxisX->categories().at(0),
                                  stepsChartAxisX->categories().at(catSize));
        return;
    }

    // It's not necessary to check min and max for the at().
    // In stepsMessage() the maximun and minumun values of the slider are set.
    qsizetype max = value + stepsChartHalfVisibleCategories;
    qsizetype min = value - stepsChartHalfVisibleCategories;
    stepsChartAxisX->setRange(stepsChartAxisX->categories().at(min),
                              stepsChartAxisX->categories().at(max));
}

void MainWindow::showMessage(QString m, bool onlyLog)
{
    if ( ! onlyLog)
        ui->statusbar->showMessage(m, 5000);

    if (ui->plainTextEdit->maximumBlockCount() == 0)
        return;

    m = QTime::currentTime().toString() + " " + m;
    ui->plainTextEdit->appendPlainText(m);
}

void MainWindow::reScan()
{
    if (discoveryAgent->isActive())
        discoveryAgent->stop();

    remoteBtDevices.clear();
    ui->deviceListWidget->clear();
    showMessage(tr("Scanning started"));
    discoveryAgent->start();
}

void MainWindow::scanFinished()
{
    showMessage(tr("Scanning completed"));
}

void MainWindow::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        showMessage(tr("The Bluetooth adaptor is powered off, power it on before doing discovery."));
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        showMessage(tr("Writing or reading from the device resulted in an error."));
    else
    {
        static QMetaEnum qme = discoveryAgent->metaObject()->enumerator(
                    discoveryAgent->metaObject()->indexOfEnumerator("Error"));
        showMessage(tr("Error: %1").arg(QLatin1String(qme.valueToKey(error))));
    }
}

void MainWindow::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
    if (mode != QBluetoothLocalDevice::HostPoweredOff)
        showMessage(tr("Bluetooth local device is powered OFF"));
    else
        showMessage(tr("Bluetooth local device is powered ON"));
}

void MainWindow::newBtDevice(const QBluetoothDeviceInfo &info)
{
    QString address = info.address().toString();
    QString name = info.name();

    if ((info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
            != QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
    {
        showMessage(tr("Not a BLE device. Name: \"%1\" Address: %2").arg(name, address), true);
        return;
    }
    if ( ! name.startsWith("N1(ID-") )
        showMessage(tr("Not a Cubot N1 device. Name: \"%1\" Address: %2").arg(name, address), true);

    // Add the device to the listWidget and to the hash
    if ( ! remoteBtDevices.contains(address) )
    {
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setToolTip(address);
        ui->deviceListWidget->addItem(item);
        remoteBtDevices[address] = info;

        // there was a rescan and it found a new BLE device
        // but, are we already connected to the device?
        if (controller &&
                ( controller->state() == QLowEnergyController::ConnectedState ||
                  controller->state() == QLowEnergyController::DiscoveringState ||
                  controller->state() == QLowEnergyController::DiscoveredState ) &&
                controller->remoteAddress().toString() == address &&
                controller->remoteName() == name)
            item->setForeground(Qt::green);

        // Connect to Cubot N1 devices even without user interaction
        if (name.startsWith("N1(ID-"))
            btDeviceItemClicked(item);
    }
}

void MainWindow::manageConnectionDisplay(bool connected)
{
    if ( ! controller ) return;

    Qt::GlobalColor color = connected ? Qt::green : Qt::black;
    QString label = QString(controller->remoteName());
    QList<QListWidgetItem *> items = ui->deviceListWidget->findItems(label, Qt::MatchExactly);
    if (items.isEmpty()) return;

    items.at(0)->setForeground(color);
    ui->servicesTreeWidget->clear();

    if (connected)
        showMessage(tr("Device connected: %1").arg(controller->remoteName()));
    else
        showMessage(tr("Device disconnected: %1").arg(controller->remoteName()));
}

void MainWindow::btDeviceItemClicked(QListWidgetItem *item)
{
    if (controller)
    {
        manageConnectionDisplay(false);
        controller->disconnectFromDevice();
        delete controller;
        controller = nullptr;
    }

    QString address = item->toolTip();
    controller = QLowEnergyController::createCentral(remoteBtDevices.value(address), this);

    connect(controller, &QLowEnergyController::connected,
            this,       &MainWindow::btDeviceConnected);
    connect(controller, &QLowEnergyController::errorOccurred,
            this,       &MainWindow::btDeviceConnectionError);
    connect(controller, &QLowEnergyController::stateChanged,
            this,       &MainWindow::btDeviceStateChanged);
    connect(controller, &QLowEnergyController::disconnected,
            this,       &MainWindow::btDeviceDisconnected);
    connect(controller, &QLowEnergyController::serviceDiscovered,
            this,       &MainWindow::btServiceDiscovered);
    connect(controller, &QLowEnergyController::discoveryFinished,
            this,       &MainWindow::btServiceDiscoverFinished);

    controller->connectToDevice();
}

void MainWindow::btDeviceConnected()
{
    manageConnectionDisplay(true);
    discoveryAgent->stop();
    controller->discoverServices();
}

void MainWindow::btDeviceDisconnected()
{
    manageConnectionDisplay(false);
}

void MainWindow::btDeviceConnectionError(QLowEnergyController::Error newError)
{
    Q_UNUSED(newError)
    showMessage(tr("Error connecting to device \"%1\". Error: %2")
                .arg(controller->remoteName(), controller->errorString()));
}

void MainWindow::btDeviceStateChanged(QLowEnergyController::ControllerState state)
{
    auto c = qobject_cast<QLowEnergyController *>(sender());
    QString name;
    QString address;
    if (c)
    {
        name = c->remoteName();
        address = c->remoteAddress().toString();
    }

    showMessage(tr("BLE Device controller state changed to: \"%1\". Name: \"%2\" Address: %3")
                .arg(QLatin1String(QMetaEnum::fromType<QLowEnergyController::ControllerState>().valueToKey(state)),
                     name, address),
                true);
}

void MainWindow::btServiceDiscovered(const QBluetoothUuid &newService)
{
    showMessage(tr("Primary service discovered UUID: %1").arg(newService.toString(QUuid::WithoutBraces)));
}

void MainWindow::btServiceDiscoverFinished()
{
    for (auto uuid: controller->services())
    {
        QLowEnergyService *s = controller->createServiceObject(uuid, this);
        if (s->state() == QLowEnergyService::RemoteService)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, s->serviceUuid().toString(QUuid::WithoutBraces));
            ui->servicesTreeWidget->addTopLevelItem(item);

            connect(s,    &QLowEnergyService::stateChanged,
                    this, &MainWindow::btServiceDetailsDiscovered);
            connect(s,    &QLowEnergyService::characteristicChanged,
                    this, &MainWindow::btCharacteristicChanged);
            connect(s,    &QLowEnergyService::characteristicRead,
                    this, &MainWindow::btCharacteristicRead);
            connect(s,    &QLowEnergyService::characteristicWritten,
                    this, &MainWindow::btCharacteristicWritten);
            connect(s,    &QLowEnergyService::descriptorRead,
                    this, &MainWindow::btDescriptorRead);
            connect(s,    &QLowEnergyService::descriptorWritten,
                    this, &MainWindow::btDescriptorWritten);
            connect(s,    &QLowEnergyService::errorOccurred,
                    this, &MainWindow::btErrorOccurred);

            showMessage(tr("Discovering details. Service UUID: %1 Name: \"%2\" Type: %3")
                        .arg(s->serviceUuid().toString(QUuid::WithoutBraces),
                             s->serviceName(),
                             QLatin1String(QMetaEnum::fromType<QLowEnergyService::ServiceType>().valueToKey(s->type()))),
                        true);
            s->discoverDetails();
        }
    }
}

void MainWindow::btServiceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
//    showMessage(tr("%1 %2")
//                .arg(Q_FUNC_INFO,
//                     QLatin1String(QMetaEnum::fromType<QLowEnergyService::ServiceState>().valueToKey(newState))),
//                true);

    if (newState != QLowEnergyService::RemoteServiceDiscovered)
        return;

    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
        return;


    // Find the item in the tree widget for the new service discovered
    QList<QTreeWidgetItem *> itemList = ui->servicesTreeWidget->
            findItems(service->serviceUuid().toString(QUuid::WithoutBraces),
                      Qt::MatchExactly);
    if (itemList.size() == 0)
    {
        showMessage(tr("Error: No service with UUID: %1")
                    .arg(service->serviceUuid().toString(QUuid::WithoutBraces)));
        return;
    }
    if (itemList.size() > 1)
    {
        showMessage(tr("Error: Several services with same UUID: %1")
                    .arg(service->serviceUuid().toString(QUuid::WithoutBraces)));
    }
    QTreeWidgetItem *serviceItem = itemList[0];

    // Add to the service's item a child item
    // for each characteritic of the service
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {

        // Description to show. We try to improve the description
        // looking for a description in the descriptors
        QString description = ch.name();

        QTreeWidgetItem *chItem = new QTreeWidgetItem();
        chItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        chItem->setText(0, ch.uuid().toString(QUuid::WithoutBraces));
        chItem->setText(1, description);  // This can change below
        {
            using Qp = QLowEnergyCharacteristic::PropertyType;
            QLowEnergyCharacteristic::PropertyTypes p = ch.properties();

            if (p & Qp::Read) chItem->setCheckState(2, Qt::Checked);
            else              chItem->setCheckState(2, Qt::Unchecked);
            if (p & Qp::Write) chItem->setCheckState(3, Qt::Checked);
            else               chItem->setCheckState(3, Qt::Unchecked);
            if (p & Qp::Notify) chItem->setCheckState(4, Qt::Checked);
            else                chItem->setCheckState(4, Qt::Unchecked);
            if (p & Qp::Indicate) chItem->setCheckState(5, Qt::Checked);
            else                  chItem->setCheckState(5, Qt::Unchecked);
            if (p & Qp::ExtendedProperty) chItem->setCheckState(6, Qt::Checked);
            else                          chItem->setCheckState(6, Qt::Unchecked);
            if (p & Qp::Broadcasting) chItem->setCheckState(7, Qt::Checked);
            else                      chItem->setCheckState(7, Qt::Unchecked);
            if (p & Qp::WriteNoResponse) chItem->setCheckState(8, Qt::Checked);
            else                         chItem->setCheckState(8, Qt::Unchecked);
            if (p & Qp::WriteSigned) chItem->setCheckState(9, Qt::Checked);
            else                     chItem->setCheckState(9, Qt::Unchecked);
        }
        serviceItem->addChild(chItem);

        // Add to the characteristic's item a child item
        // for each descriptor of the characteristic
        for (const QLowEnergyDescriptor &descriptor : ch.descriptors())
        {
            QTreeWidgetItem *descrItem = new QTreeWidgetItem();
            descrItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            descrItem->setText(0, descriptor.uuid().toString(QUuid::WithoutBraces));
            descrItem->setText(1, descriptor.name());
            chItem->addChild(descrItem);

            //auto cccd = mycharacteristic.clientCharacteristicConfiguration();
            // Note: Calling characteristic.clientCharacteristicConfiguration() is equivalent to calling
            // characteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration).

            // Enable notifications for all CCCD
            // From https://doc.qt.io/qt-6/qtbluetooth-le-overview.html
            // Ultimately QLowEnergyCharacteristic::properties() must have the QLowEnergyCharacteristic::Notify flag set
            // and a descriptor of type QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration must exist to
            // confirm the availability of an appropriate notification.
            //
            // The above paragraph is not true for the Cubot N1
            if (descriptor.type() == QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration)
                service->writeDescriptor(descriptor, QByteArray::fromHex("0100"));

            // Try to find a better description for the characteristic's item
            if (description.isEmpty() &&
                    descriptor.type() == QBluetoothUuid::DescriptorType::CharacteristicUserDescription)
                description = descriptor.value();
        }

        if (description.isEmpty())
            chItem->setText(1, "Unknown");
        else
            chItem->setText(1, description);

        showMessage(tr("Discovered characteristic. Service UUID: %1 Charac UUID: %2 Charac name: \"%3\" Properties: 0x%4")
                    .arg(service->serviceUuid().toString(QUuid::WithoutBraces),
                         ch.uuid().toString(QUuid::WithoutBraces),
                         ch.name(),
                         QString::number( ch.properties().toInt(), 16 ).rightJustified(2, '0')),
                    true);

    }



// *************************************************************************
// At this point the device is connected and all characteristics are known.
// Now we just have to wait for notifications from the BLE device or for
// the user input.
//
// *** From here on, the code of this file is specific to the Cubot N1 ***
//
    if ( ! (controller->remoteName().startsWith("N1(ID-") &&
            controller->remoteAddress().toString().startsWith("78:02:B7:")) )
        deviceIsCubotN1 = false;
    else
    {
        deviceIsCubotN1 = true;

        // Dumps
        if (service->serviceUuid().toString(QUuid::WithoutBraces) == "000055ff-0000-1000-8000-00805f9b34fb")
        {
            QLowEnergyCharacteristic c = service->characteristic(QBluetoothUuid("000033f1-0000-1000-8000-00805f9b34fb"));
            if (c.isValid())
            {
                clearCharts();

                // Request the data dumps
                service->writeCharacteristic(c, makeDumpRequest(heartRate));

                // The remaining dumps are made when the previous one is finished.
                // Cubot N1 doesn't answer well to two simultaneous request
                //service->writeCharacteristic(c, requestForDump(O2));
                //service->writeCharacteristic(c, requestForDump(steps));
                //service->writeCharacteristic(c, requestForDump(otherSports));

                // As not all dumps can be done at the same time
                // we use these variables later
                dumpService = service;
                dumpCharacteristic = c;

            }
        }
    }
}

// ////////// makeDumpRequest
// Dump request sent to the watch:
//              0  1  2  3  4  5  6  7
//          HR: f7 fa 07 e5 0c 08 03 2d
//          O2: 34 fa
//       Steps: b2 fa
// otherSports: fd fa
//              Ty -- YY YY MM DD HH mm
//
// Ty: Request type. See messageType enum in mainwindow.h
// --: Always "0xfa" in all request types
// O2, steps and other sports dump requests are simpler. Doesn't have date.

QByteArray MainWindow::makeDumpRequest(messageType type)
{
    switch (type) {
    case O2:
    case steps:
    case otherSports:
    {
        QByteArray value;
        value.resize(2);
        value[0] = type;
        value[1] = 0xfa;
        return value;
    }
    case heartRate:
    {
        int days = 8; // Cubot N1 only saves 7 days of heart rate data

        // date of the beginning of the dump
        QDateTime dateTime = QDateTime::currentDateTime();
        dateTime = dateTime.addDays(-days);
        QCalendar cal;
        QCalendar::YearMonthDay ymd = cal.partsFromDate(dateTime.date());

        // Request
        QByteArray value;
        value.resize(8);
        value[0] = type;
        value[1] = 0xfa;
        value[2] = ymd.year >> 8;
        value[3] = ymd.year;
        value[4] = ymd.month;
        value[5] = ymd.day;
        value[6] = dateTime.time().hour();
        //value[7] = dateTime.time().minute(); // Minutes are irrelevant
        value[7] = 0;
        return value;
    }
    default:
        // heartRateRT is not a valid type for dumps
        Q_UNREACHABLE();
    }
}

void MainWindow::btServiceItemClicked(QTreeWidgetItem *item, int column)
{
//    showMessage(tr("%1 TODO: User clicked Row: %2 Column: %3")
//                .arg(Q_FUNC_INFO, item->text(0), QString::number(column)),
//                true);
    Q_UNUSED(item)
    Q_UNUSED(column)
}


// ////////// btCharacteristicChanged
// This is the method that dispatches the messages received from the watch.
// Based on the first byte, the message type, the appropriate method for that
// message type is executed.
void MainWindow::btCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    if (newValue.isEmpty()){
        showMessage(tr("Characteristic %1 changed its value to an empty value.")
                    .arg(characteristic.uuid().toString(QUuid::WithoutBraces)),
                    true);
        return;
    }
    if ( ! deviceIsCubotN1 )
    {
        showMessage(tr("Characteristic %1 changed its value to: %2")
                    .arg(characteristic.uuid().toString(QUuid::WithoutBraces),
                         newValue.toHex(' ')),
                    true);
        return;
    }

    switch ( static_cast<messageType>(newValue.at(0)) ) {
    case heartRate:
        heartRateMessage(newValue);
        break;
    case heartRateRT:
        heartRateRealTimeMessage(newValue);
        break;
    case O2:
        O2Message(newValue);
        break;
    case steps:
        stepsMessage(newValue);
        break;
    case otherSports:
        showMessage(tr("Other Sports Dump value: %1").arg(newValue.toHex(' ')),
                    true);
        break;
    default:
        showMessage(tr("UNKNOW DATA. Reverse engineering is not finished!!!! Characteristic %1 changed its value to: %2")
                    .arg(characteristic.uuid().toString(QUuid::WithoutBraces),
                         newValue.toHex(' ')),
                    true);
    }
}

void MainWindow::btCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    showMessage(tr("%1 Characteristic: %2 Value: %3")
                .arg(Q_FUNC_INFO,
                     characteristic.uuid().toString(QUuid::WithoutBraces), value.toHex(' ')),
                true);
}

void MainWindow::btCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    showMessage(tr("%1 Characteristic: %2 Value: %3")
                .arg(Q_FUNC_INFO,
                     characteristic.uuid().toString(QUuid::WithoutBraces), newValue.toHex(' ')),
                true);
}

void MainWindow::btDescriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value)
{
    showMessage(tr("%1 Descriptor: %2 Value: %3")
                .arg(Q_FUNC_INFO,
                     descriptor.uuid().toString(QUuid::WithoutBraces), value.toHex(' ')),
                true);
}

void MainWindow::btDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue)
{
    showMessage(tr("%1 Descriptor: %2 Value: %3")
                .arg(Q_FUNC_INFO,
                     descriptor.uuid().toString(QUuid::WithoutBraces) , newValue.toHex(' ')),
                true);

}

void MainWindow::btErrorOccurred(QLowEnergyService::ServiceError newError)
{
    showMessage(tr("%1 Error: %2")
                .arg(Q_FUNC_INFO,
                     QLatin1String(QMetaEnum::fromType<QLowEnergyService::ServiceError>().valueToKey(newError))),
                true);
}

// ////////// heartRateMessage
// Data example:
//         0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17
// Data:  f7 07 e6 03 0c 10 41 43 40 3e 38 44 3e 43 3b 3d 43 43
//           YY YY MM DD HH 10 20 30 40 50 00 10 20 30 40 50 00
//    HH: Hours range. From HH-2 up to HH.
//        E.g.: 0x10 (16) = from 14:00 to 16:00.
//    From bytes 6 to 17 are HR average of the ten minutes period.
//        byte 06: 0x41 (65) = from 15:01 to 15:10 HR average was 65
//        byte 07: 0x43 (67) = from 15:11 to 15:20 HR average was 67
//        byte 11: 0x44 (68) = from 16:01 to 16:10 HR average was 68
//        byte 14: 0x3b (59) = from 16:21 to 16:30 HR average was 59
//        Value 0xff means "No samples"
//
// Dump ends with:
// f7 fd XX
//       XX: Unknown meaning
//
// Not requested heart rate notifications. Every 10 min.
//  0  1  2  3  4  5  6  7  8
// f7 03 07 e5 0c 07 13 1e 39 = 0x13 0x1e 0x39 (19:30 57) Beats/min at notificated time
//       YY YY MM DD HH mm
//
// Heart rate notifications when the watch displays the HR chart.
//  0  1  2  3  4  5  6  7  8   9  10
// f7 04 07 e6 03 10 10 24 65  2f  3c
//       YY YY MM DD HH mm max min ??
void MainWindow::heartRateMessage(const QByteArray &dataRecived)
{
    // Heart rate notifications when the watch displays the HR chart.
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x04 &&
        dataRecived.size() == 11)
    {
        unsigned int year, month, day, hour, minute, max, min, unknown;
        year    = static_cast<unsigned char>(dataRecived.at(2)) << 8;
        year   |= static_cast<unsigned char>(dataRecived.at(3));
        month   = static_cast<unsigned char>(dataRecived.at(4));
        day     = static_cast<unsigned char>(dataRecived.at(5));
        hour    = static_cast<unsigned char>(dataRecived.at(6));
        minute  = static_cast<unsigned char>(dataRecived.at(7));
        max     = static_cast<unsigned char>(dataRecived.at(8));
        min     = static_cast<unsigned char>(dataRecived.at(9));
        unknown = static_cast<unsigned char>(dataRecived.at(10));
        showMessage(tr("Watch is showing HR chart. Notification: Year: %1 Month: %2 Day: %3 Hour: %4 Minute: %5 Max of day: %6. Min of day: %7 Unknown: %8 Raw Data: %9")
                    .arg(QString::number(year), QString::number(month),
                         QString::number(day), QString::number(hour),
                         QString::number(minute), QString::number(max),
                         QString::number(min), QString::number(unknown),
                         dataRecived.toHex(' ')),
                    true);
        return;
    }

    // Not requested heart rate notifications
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x03 &&
        dataRecived.size() == 9)
    {
        unsigned int year, month, day, hour, minute, beats;
        year   = static_cast<unsigned char>(dataRecived.at(2)) << 8;
        year  |= static_cast<unsigned char>(dataRecived.at(3));
        month  = static_cast<unsigned char>(dataRecived.at(4));
        day    = static_cast<unsigned char>(dataRecived.at(5));
        hour   = static_cast<unsigned char>(dataRecived.at(6));
        minute = static_cast<unsigned char>(dataRecived.at(7));
        beats  = static_cast<unsigned char>(dataRecived.at(8));
        showMessage(tr("Not requested heart rate notification. Year: %1 Month: %2 Day: %3 Hour: %4 Minute: %5 Beats: %6. Raw Data: %7")
                    .arg(QString::number(year), QString::number(month),
                         QString::number(day), QString::number(hour),
                         QString::number(minute), QString::number(beats),
                         dataRecived.toHex(' ')),
                    true);
        return;
    }

    // End of dump
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0xfd &&
        dataRecived.size() == 3)
    {
        showMessage(tr("Heart Rate Dump Finnished. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        showMessage("Requesting Steps Dump.", true);
        dumpService->writeCharacteristic(dumpCharacteristic, makeDumpRequest(steps));
        return;
    }

    if (dataRecived.size() != 18)
    {
        showMessage(tr("Heart Rate Dump. Error: Invalid data. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    QDateTime xValue;
    qreal yValue;
    unsigned int year, month, day, hour;
    QString debugMessage;

    year  = static_cast<unsigned char>(dataRecived.at(1)) << 8;
    year |= static_cast<unsigned char>(dataRecived.at(2));
    month = static_cast<unsigned char>(dataRecived.at(3));
    day   = static_cast<unsigned char>(dataRecived.at(4));
    hour  = static_cast<unsigned char>(dataRecived.at(5));
    xValue.setDate(QDate(year, month, day));
    xValue.setTime(QTime(hour, 10));
    xValue = xValue.addSecs(-60 * 60 * 2); // data is from 2 hours before
    if (! xValue.isValid())
    {
        showMessage(tr("Heart Rate Dump. Error: Invalid date. Year: %1 Month: %2 Day: %3 Hour: %4 Raw Data: %5")
                    .arg(QString::number(year), QString::number(month),
                         QString::number(day), QString::number(hour),
                         dataRecived.toHex(' ')),
                    true);
        return;
    }
    // After subtracting two hours the year, month or day may have changed.
    year  = xValue.date().year();
    month = xValue.date().month();
    day   = xValue.date().day();

    for (int i = 6; i <= 17; i++)
    {
        yValue = static_cast<unsigned char>(dataRecived.at(i));
        if (yValue == 0xff)
            debugMessage.append(xValue.toString("hh:mm") + " HR: --; ");
        else
        {
            heartRateSeries->append(xValue.toMSecsSinceEpoch(), yValue);

            if (xValue > heartRateChartAxisXMax) heartRateChartAxisXMax = xValue;
            if (xValue < heartRateChartAxisXMin) heartRateChartAxisXMin = xValue;
            if (yValue > heartRateChartAxisYMax) heartRateChartAxisYMax = yValue;
            if (yValue < heartRateChartAxisYMin) heartRateChartAxisYMin = yValue;

            debugMessage.append(xValue.toString("hh:mm") + " HR: " +
                                QString::number(yValue) + "; ");
        }

        xValue = xValue.addSecs(600); // add 10 minutes
    }

    // Center the chart to the series end
    const qint64 xMaxSeconds = heartRateChartAxisXMax.toSecsSinceEpoch();
    const qint64 xMinSeconds = heartRateChartAxisXMin.toSecsSinceEpoch();
    const qint64 xVisibleRange = xMaxSeconds - heartRateChartMaxXRange;
    const qint64 min = xMinSeconds < xVisibleRange ? xVisibleRange : xMinSeconds;
    QDateTime minDT;
    minDT.setSecsSinceEpoch(min);
    heartRateChartAxisX->setRange(minDT, heartRateChartAxisXMax);
    heartRateChartAxisY->setRange(heartRateChartAxisYMin - 5, heartRateChartAxisYMax + 5);

    // Slider
    if (xMinSeconds < xVisibleRange)
    {
        ui->HRChartSlider->setMaximum(xMaxSeconds - heartRateChartHalfXRange);
        ui->HRChartSlider->setMinimum(xMinSeconds + heartRateChartHalfXRange);
        ui->HRChartSlider->setValue(xMaxSeconds - heartRateChartHalfXRange);
    }
    else
    {
        ui->HRChartSlider->setMaximum(0);
        ui->HRChartSlider->setMinimum(0);
        //ui->HRChartSlider->setValue(0);
    }

    showMessage(tr("Heart Rate Dump. Year: %1 Month: %2 Day: %3 Data: %4 Raw Data: %5")
                .arg(QString::number(year), QString::number(month),
                     QString::number(day), debugMessage,
                     dataRecived.toHex(' ')),
                true);
}

// ////////// heartRateRealTimeMessage
// Data example:
// e5 11       = The watch displays the HR chart.
// e5 11 00 37 = 0x37 (55) beats/min real time data.
// e5 00 00 36 = The watch stops displaying the HR chart. Last sample 0x36 (54)
// e5 00 ff 00 = The watch stops displaying the HR chart. No samples.
void MainWindow::heartRateRealTimeMessage(const QByteArray &dataRecived)
{
    // e5 11 = The watch displays the HR chart.
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
        dataRecived.size() == 2)
    {
        showMessage(tr("Real-time heart rate. The watch is displaying the HR chart. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    // e5 11 00 37 = 0x37 (55) beats/min real time.
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0x00 &&
        dataRecived.size() == 4)
    {
        unsigned int beats;
        beats = static_cast<unsigned char>(dataRecived.at(3));
        showMessage(tr("Real-time heart rate. Current sample: %1 beats/minute. Raw Data: %2")
                    .arg(QString::number(beats), dataRecived.toHex(' ')),
                    true);
        return;
    }

    // e5 00 00 36 = The watch stops displaying the HR chart. Last sample 0x36 (54)
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x00 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0x00 &&
        dataRecived.size() == 4)
    {
        unsigned int beats;
        beats = static_cast<unsigned char>(dataRecived.at(3));
        showMessage(tr("Real-time heart rate. The watch no longer displays the HR chart. Last sample: %1 beats/minute. Raw Data: %2")
                    .arg(QString::number(beats), dataRecived.toHex(' ')),
                    true);
        return;
    }

    // e5 00 ff 00 = The watch stops displaying the HR chart. No samples.
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x00 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0xff &&
        static_cast<unsigned char>(dataRecived.at(3)) == 0x00 &&
        dataRecived.size() == 4)
    {
        showMessage(tr("Real-time heart rate. The watch no longer displays the HR chart. No samples. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    showMessage(tr("Real-time heart rate. Error: Invalid data. Raw Data: %1")
                .arg(dataRecived.toHex(' ')),
                true);
}

// ////////// O2Message
// Data example:
//        0 1  2 3 4 5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
// Data: 34fa 07e50c08 0e 00 ff ff 62 62 ff 62 62 ff ff ff ff ff
//            YYYYMMDD HH -- 10 20 30 40 50 00 10 20 30 40 50 00
//            HH: Hours range. From HH-2 up to HH.
//                E.g.: 0x0e (12) = from 10:00 to 12:00.
//            --: Byte 7 always is 00.
//            From bytes 8 to 19 are O2 average of the ten minutes period.
//                Value 0xff means "No samples"
//                byte 08: 0xff (255) = from 10:01 to 10:10 there wasn't O2 data
//                byte 10: 0x62 (98) = from 10:21 to 10:30 O2 average was 98
//
// Dump ends with:
// 34 fa fd XX
//          XX: Unknown meaning
//
// O2 Saturation in real time samples:
// 34 11 XX XX   heartRateRealTimeMessage
void MainWindow::O2Message(const QByteArray &dataRecived)
{
    // The watch is sampling O2 Saturation in real time
    if ( (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
          dataRecived.size() == 2)                                 ||
         (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
          dataRecived.size() == 4)                                 ||
         (static_cast<unsigned char>(dataRecived.at(1)) == 0x00 &&
          dataRecived.size() == 4)
         )
    {
        O2MessageRealTime(dataRecived);
        return;
    }

    // End of dump
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0xfa &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0xfd &&
        dataRecived.size() == 4)
    {
        showMessage(tr("O2 Dump Finnished. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        // TODO: Show a chart with Other Sports Dump data
        //showMessage("Requesting Other Sports Dump.", true);
        //dumpService->writeCharacteristic(dumpCharacteristic, requestForDump(otherSports));
        return;
    }

    if (dataRecived.size() != 20)
    {
        showMessage(tr("O2 Dump. Error: Invalid data. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    QDateTime xValue;
    qreal yValue;
    unsigned int year, month, day, hour;
    QString debugMessage;

    year  = static_cast<unsigned char>(dataRecived.at(2)) << 8;
    year |= static_cast<unsigned char>(dataRecived.at(3));
    month = static_cast<unsigned char>(dataRecived.at(4));
    day   = static_cast<unsigned char>(dataRecived.at(5));
    hour  = static_cast<unsigned char>(dataRecived.at(6));
    xValue.setDate(QDate(year, month, day));
    xValue.setTime(QTime(hour, 10));
    xValue = xValue.addSecs(-60 * 60 * 2); // data is from 2 hours before
    if (! xValue.isValid())
    {
        showMessage(tr("O2 Dump. Error: Invalid date. Year: %1 Month: %2 Day: %3 Hour: %4 Raw Data: %5")
                    .arg(QString::number(year), QString::number(month),
                         QString::number(day), QString::number(hour),
                         dataRecived.toHex(' ')),
                    true);
        return;
    }
    // After subtracting two hours the year, month or day may have changed.
    year  = xValue.date().year();
    month = xValue.date().month();
    day   = xValue.date().day();

    for (int i = 8; i <= 19; i++)
    {
        yValue = static_cast<unsigned char>(dataRecived.at(i));
        if (yValue == 0xff)
            debugMessage.append(xValue.toString("hh:mm") + " O2: --; ");
        else
        {
            O2Series->append(xValue.toMSecsSinceEpoch(), yValue);

            if (xValue > O2ChartAxisXMax) O2ChartAxisXMax = xValue;
            if (xValue < O2ChartAxisXMin) O2ChartAxisXMin = xValue;
            if (yValue > O2ChartAxisYMax) O2ChartAxisYMax = yValue;
            if (yValue < O2ChartAxisYMin) O2ChartAxisYMin = yValue;

            debugMessage.append(xValue.toString("hh:mm") + " O2: " +
                                QString::number(yValue) + "; ");
        }

        xValue = xValue.addSecs(600); // add 10 minutes
    }

    // Center the chart to the series end
    const qint64 xMaxSeconds = O2ChartAxisXMax.toSecsSinceEpoch();
    const qint64 xMinSeconds = O2ChartAxisXMin.toSecsSinceEpoch();
    const qint64 xVisibleRange = xMaxSeconds - O2ChartMaxXRange;
    const qint64 min = xMinSeconds < xVisibleRange ? xVisibleRange : xMinSeconds;
    QDateTime minDT;
    minDT.setSecsSinceEpoch(min);
    O2ChartAxisX->setRange(minDT, O2ChartAxisXMax);
    O2ChartAxisY->setRange(O2ChartAxisYMin - 5, O2ChartAxisYMax + 5);

    // Slider
    if (xMinSeconds < xVisibleRange)
    {
        ui->O2ChartSlider->setMaximum(xMaxSeconds - O2ChartHalfXRange);
        ui->O2ChartSlider->setMinimum(xMinSeconds + O2ChartHalfXRange);
        ui->O2ChartSlider->setValue(xMaxSeconds - O2ChartHalfXRange);
    }
    else
    {
        ui->O2ChartSlider->setMaximum(0);
        ui->O2ChartSlider->setMinimum(0);
        //ui->O2ChartSlider->setValue(0);
    }

    showMessage(tr("O2 Dump. Year: %1 Month: %2 Day: %3 Data: %4 Raw Data: %5")
                .arg(QString::number(year), QString::number(month),
                     QString::number(day), debugMessage,
                     dataRecived.toHex(' ')),
                true);
}

// ////////// O2MessageRealTime
// Notifications when the watch is showing the O2 % saturation screen.
//  0  1  2  3
// 34 11       = The watch displays the O2 % saturation screen.
// 34 11 ff ff = No sample
// 34 11 00 62 = 0x62 (98) Sample. 98% O2 saturation
// 34 00 00 63 = The watch stops displaying the O2 saturation screen. Last sample: 0x63 (99)
// 34 11       = The watch displays the O2 % saturation screen.
void MainWindow::O2MessageRealTime(const QByteArray &dataRecived)
{
    // 34 11 = The watch displays the O2 % saturation screen.
    if ((static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
         dataRecived.size() == 2))
    {
        showMessage(tr("Real-time O2 saturation. The watch is displaying the O2 saturation screen. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    // 34 11 ff ff = No sample
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0xff &&
        static_cast<unsigned char>(dataRecived.at(3)) == 0xff &&
        dataRecived.size() == 4)
    {
        showMessage(tr("Real-time O2 saturation. Waiting for samples. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    // 34 11 00 62 = 0x62 (98) Sample. 98% O2 saturation
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x11 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0x00 &&
        dataRecived.size() == 4)
    {
        unsigned int saturation;
        saturation = static_cast<unsigned char>(dataRecived.at(3));
        showMessage(tr("Real-time O2 saturation. Current sample: %1% O2 saturation. Raw Data: %2")
                    .arg(QString::number(saturation), dataRecived.toHex(' ')),
                    true);
        return;
    }

    // 34 00 00 63 = The watch stops displaying the O2 saturation screen. Last sample: 0x63 (99)
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0x00 &&
        static_cast<unsigned char>(dataRecived.at(2)) == 0x00 &&
        dataRecived.size() == 4)
    {
        unsigned int saturation;
        saturation = static_cast<unsigned char>(dataRecived.at(3));
        showMessage(tr("Real-time O2 saturation. The watch no longer displays the O2 saturation screen. Last sample: %1% O2 saturation. Raw Data: %2")
                    .arg(QString::number(saturation), dataRecived.toHex(' ')),
                    true);
        return;
    }

    showMessage(tr("Real-time O2 saturation. Error: Invalid data. Raw Data: %1")
                .arg(dataRecived.toHex(' ')),
                true);
}

// ////////// stepsMessage
// Data Example:
//       0   1 2 3 4    5  6 7  8  9 10 1112  13 14 15 1617
//Data: b2  07e50c02   0f 08ab 00 0a 0a 05cd  09 38 0c 02de
//dec:  178 2021-12-02 15 2219 00 10 10 1485  09 56 12 0734
//          YYYYMMDD   HH Tota m1 m2 n1 TRun  m3 m4 n2 TWal
//    HH: Hour. 0x0f (15) = 15:00 - 15:59
//    Tota: Total steps hour. 0x08ab (2219) steps
//    m1: Minute start running. HH:m1
//    m2: Minute stop running. HH:m2
//    n1: Times running
//    TRun: Total steps running.
//    m3: Minute start walking. HH:m3
//    m4: Minute stop walking. HH:m4
//    n2: Times walking.
//    TWal: Total steps walking.
//
// Dump finnishs with:
// b2 fd XX
//       XX: Unknown meaning
void MainWindow::stepsMessage(const QByteArray &dataRecived)
{
    // End of dump
    if (static_cast<unsigned char>(dataRecived.at(1)) == 0xfd &&
        dataRecived.size() == 3)
    {
        showMessage(tr("Steps Dump Finnished. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        ui->stepsChartSlider->setValue(stepsChartAxisX->count() - 1);

        showMessage("Requesting O2 Dump.", true);
        dumpService->writeCharacteristic(dumpCharacteristic, makeDumpRequest(O2));
        return;
    }

    if (dataRecived.size() != 18)
    {
        showMessage(tr("Steps Dump. Error: Invalid data. Raw Data: %1")
                    .arg(dataRecived.toHex(' ')),
                    true);
        return;
    }

    QDateTime xValue;
    unsigned int year, month, day, hour;
    unsigned int totalSteps, totalRunSteps, totalWalkSteps;
    unsigned int totalStepsCalculated;
    QString debugMessage;

    year  = static_cast<unsigned char>(dataRecived.at(1)) << 8;
    year |= static_cast<unsigned char>(dataRecived.at(2));
    month = static_cast<unsigned char>(dataRecived.at(3));
    day   = static_cast<unsigned char>(dataRecived.at(4));
    hour  = static_cast<unsigned char>(dataRecived.at(5));
    xValue.setDate(QDate(year, month, day));
    xValue.setTime(QTime(hour, 0));
    if (! xValue.isValid())
    {
        showMessage(tr("Steps Dump. Error: Invalid date. Year: %1 Month: %2 Day: %3 Hour: %4 Raw Data: %5")
                    .arg(QString::number(year), QString::number(month),
                         QString::number(day), QString::number(hour),
                         dataRecived.toHex(' ')),
                    true);
        return;
    }

    stepsChartAxisX->append(xValue.toString("dd/MM/yyyy HH:mm"));

    totalSteps      = static_cast<unsigned char>(dataRecived.at(6)) << 8;
    totalSteps     |= static_cast<unsigned char>(dataRecived.at(7));
    totalRunSteps   = static_cast<unsigned char>(dataRecived.at(11)) << 8;
    totalRunSteps  |= static_cast<unsigned char>(dataRecived.at(12));
    totalWalkSteps  = static_cast<unsigned char>(dataRecived.at(16)) << 8;
    totalWalkSteps |= static_cast<unsigned char>(dataRecived.at(17));
    stepsSeriesRunSet->append(totalRunSteps);
    stepsSeriesWalkSet->append(totalWalkSteps);

    totalStepsCalculated = totalRunSteps + totalWalkSteps;
    if (totalStepsCalculated > stepsChartAxisYMax) stepsChartAxisYMax = totalStepsCalculated;
    if (xValue > stepsChartAxisXMax) stepsChartAxisXMax = xValue;
    if (xValue < stepsChartAxisXMin) stepsChartAxisXMin = xValue;

    // Center the chart to the series end
    const qsizetype catSize = stepsChartAxisX->count() - 1;
    const qsizetype min = catSize > stepsChartMaxVisibleCategories ?
                catSize - stepsChartMaxVisibleCategories :
                0;
    stepsChartAxisX->setRange(stepsChartAxisX->categories().at(min),
                              stepsChartAxisX->categories().at(catSize));
    stepsChartAxisY->setRange(0, stepsChartAxisYMax + 5);

    // Slider
    if (catSize > stepsChartMaxVisibleCategories)
    {
        ui->stepsChartSlider->setMaximum(catSize - stepsChartHalfVisibleCategories);
        ui->stepsChartSlider->setMinimum(stepsChartHalfVisibleCategories);
        ui->stepsChartSlider->setValue(catSize - stepsChartHalfVisibleCategories);
    }
    else
    {
        ui->stepsChartSlider->setMaximum(0);
        ui->stepsChartSlider->setMinimum(0);
        //ui->stepsChartSlider->setValue(0);
    }

    if (totalSteps != totalStepsCalculated)
        debugMessage = "Steps Dump. Year: %1 Month: %2 Day: %3 Hour: %4 Total Steps: %5 Total Run Steps: %6 Total Walk Steps: %7. \
Incorrect message: Total Steps != Total Run Steps + Total Walk Steps. Raw Data: %8";
    else
        debugMessage = "Steps Dump. Year: %1 Month: %2 Day: %3 Hour: %4 Total Steps: %5 Total Run Steps: %6 Total Walk Steps: %7. Raw Data: %8";

    showMessage(tr(debugMessage.toLatin1())
                .arg(QString::number(year), QString::number(month),
                     QString::number(day), QString::number(hour),
                     QString::number(totalSteps), QString::number(totalRunSteps),
                     QString::number(totalWalkSteps), dataRecived.toHex(' ')),
                true);
}
