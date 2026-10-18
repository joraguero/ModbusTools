/*
    Modbus Tools

    Created: 2023
    Author: Serhii Marchuk, https://github.com/serhmarch

    Copyright (C) 2023  Serhii Marchuk

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "client_dataviewui.h"

#include <QLabel>
#include <QVBoxLayout>

#include <client.h>
#include <project/client_project.h>
#include <project/client_device.h>
#include <project/client_dataview.h>

#include "client_dataviewmodel.h"
#include "client_dataviewdelegate.h"

mbClientDataViewUi::mbClientDataViewUi(mbClientDataView *dataView, QWidget *parent) :
    mbCoreDataViewUi(dataView, new mbClientDataViewModel(dataView), new mbClientDataViewDelegate(), parent)
{
    m_lbStatistics = new QLabel(this);
    m_lbStatistics->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lbStatus = new QLabel(this);
    QFont headerFont = font();
    headerFont.setPointSizeF(headerFont.pointSizeF() * 1.2);
    m_lbStatistics->setFont(headerFont);
    m_lbStatus->setFont(headerFont);

    QWidget *header = new QWidget(this);
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(4, 2, 4, 2);
    headerLayout->setSpacing(0);
    headerLayout->addWidget(m_lbStatistics);
    headerLayout->addWidget(m_lbStatus);
    if (QBoxLayout *l = qobject_cast<QBoxLayout*>(layout()))
        l->insertWidget(0, header);

    connect(dataView, &mbClientDataView::statisticsChanged      , this, &mbClientDataViewUi::updateHeader);
    connect(dataView, &mbClientDataView::requestChanged         , this, &mbClientDataViewUi::updateHeader);
    connect(dataView, &mbClientDataView::enableProcessingChanged, this, &mbClientDataViewUi::updateHeader);
    connect(dataView, &mbClientDataView::periodChanged          , this, &mbClientDataViewUi::updateHeader);
    connect(mbClient::global(), &mbClient::statusChanged        , this, &mbClientDataViewUi::updateHeader);
    updateHeader();
}

void mbClientDataViewUi::updateHeader()
{
    mbClientDataView *view = dataView();
    mbClientDataView::Statistics stat = view->statistics();
    mbClientDevice *device = view->requestDevice();
    m_lbStatistics->setText(QString("%1[%2]: Tx=%3, Err=%4, F=%5, SR=%6ms")
                                .arg(device ? device->name() : QStringLiteral("-"))
                                .arg(device ? QString::number(device->unit()) : QStringLiteral("-"))
                                .arg(stat.countTx)
                                .arg(stat.countErr)
                                .arg(view->requestFunction(), 2, 10, QLatin1Char('0'))
                                .arg(view->period()));

    // Status of the last request of the data view: Good is green, any error is red
    const QString red   = QStringLiteral("color: red;");
    const QString green = QStringLiteral("color: #008000;");
    const QString gray  = QStringLiteral("color: gray;");
    QString status;
    QString color = red;
    if (!mbClient::global()->isRunning())
        status = QStringLiteral("No connection");
    else if (!view->isEnableProcessing())
    {
        status = QStringLiteral("Processing disabled");
        color = gray;
    }
    else if (!view->isRequestValid())
        status = QStringLiteral("No device");
    else if (Modbus::StatusIsGood(stat.lastStatus))
    {
        status = mb::toString(stat.lastStatus);
        color = green;
    }
    else if (Modbus::StatusIsBad(stat.lastStatus))
    {
        status = mb::toString(stat.lastStatus);
        if (!stat.lastErrorText.isEmpty())
            status += QStringLiteral(" - ") + stat.lastErrorText;
    }
    else
    {
        status = QStringLiteral("Waiting...");
        color = gray;
    }
    m_lbStatus->setStyleSheet(color);
    m_lbStatus->setText(status);
}

QList<mbClientDataViewItem*> mbClientDataViewUi::selectedItems() const
{
    QList<mbCoreDataViewItem*> ls = selectedItemsCore();
    return QList<mbClientDataViewItem*>(*(reinterpret_cast<const QList<mbClientDataViewItem*>*>(&ls)));
}
