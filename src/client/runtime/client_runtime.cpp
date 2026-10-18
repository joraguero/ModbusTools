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
#include "client_runtime.h"

#include <QCoreApplication>

#include <client.h>

#include <project/client_project.h>
#include <project/client_port.h>
#include <project/client_device.h>
#include <project/client_dataview.h>

#include "client_runport.h"
#include "client_rundevice.h"
#include "client_runitem.h"
#include "client_runmessage.h"
#include "client_runthread.h"

mbClientRuntime::mbClientRuntime(QObject *parent)
    : mbCoreRuntime{parent}
{
}

void mbClientRuntime::createComponents()
{
    const mb::StatusCode status = mb::Status_MbInitializing;
    const mb::Timestamp_t timestamp = mb::currentTimestamp();

    Q_FOREACH (mbClientDataView *dataView, project()->dataViews())
        dataView->resetStatistics();

    Q_FOREACH (mbClientPort *port, project()->ports())
    {
        mbClientRunPort *rp = createRunPort(port);
        QList<mbClientRunDevice*> runDevices;
        Q_FOREACH (mbClientDevice *device, port->devices())
        {
            mbClientRunDevice *rd = createRunDevice(device);
            runDevices.append(rd);
        }
        // Every data view window is one Modbus request polled in turn with other windows of the port
        Q_FOREACH (mbClientDataView *dataView, project()->dataViews())
        {
            if (!dataView->isEnableProcessing() || !dataView->isRequestValid())
                continue;
            mbClientDevice *device = dataView->requestDevice();
            if (device->port() != port)
                continue;
            mbClientRunMessagePtr message = createReadMessage(dataView);
            if (!message)
                continue;
            message->setSourceName(QString("%1/%2").arg(device->name(), dataView->name()));
            Q_FOREACH (mbClientDataViewItem *item, dataView->items())
            {
                item->update(status, timestamp);
                message->attachItem(createRunItem(item));
            }
            uint32_t scanRate = static_cast<uint32_t>(qMax(dataView->period(), 0));
            rp->pushPollEntry(mbClientRunPort::PollEntry{m_devices.value(device), message, dataView, scanRate});
        }
        createRunThread(rp);
        rp->pushDevices(runDevices);
    }
}

mbClientRunMessagePtr mbClientRuntime::createReadMessage(mbClientDataView *dataView)
{
    mbClientDevice *device = dataView->requestDevice();
    uint16_t offset = dataView->requestOffset();
    int count = qMin(dataView->requestQuantity(), mbClientDataView::maxQuantity(device, dataView->requestMemoryType()));
    if (count <= 0)
        return nullptr;
    uint16_t c = static_cast<uint16_t>(count);
    switch (dataView->requestMemoryType())
    {
    case Modbus::Memory_0x: return new mbClientRunMessageReadCoils           (offset, c, c);
    case Modbus::Memory_1x: return new mbClientRunMessageReadDiscreteInputs  (offset, c, c);
    case Modbus::Memory_3x: return new mbClientRunMessageReadInputRegisters  (offset, c, c);
    case Modbus::Memory_4x: return new mbClientRunMessageReadHoldingRegisters(offset, c, c);
    default:
        return nullptr;
    }
}

void mbClientRuntime::startComponents()
{
    mbCoreRuntime::startComponents();
    Q_FOREACH (mbClientRunThread *t, m_threads)
        t->start();
}

void mbClientRuntime::beginStopComponents()
{
    mbCoreRuntime::beginStopComponents();
    Q_FOREACH (mbClientRunThread *t, m_threads)
        t->stop();
}

bool mbClientRuntime::tryStopComponents()
{
    if (!mbCoreRuntime::tryStopComponents())
        return false;
    Q_FOREACH (mbClientRunThread *t, m_threads)
    {
        if (t->isRunning())
            return false;
    }
    return true;
}

void mbClientRuntime::clearComponents()
{
    const mb::StatusCode status = mb::Status_MbStopped;
    const mb::Timestamp_t timestamp = mb::currentTimestamp();
    QList<mbClientDataViewItem*> items = m_items.keys();
    Q_FOREACH (mbClientDataViewItem *item, items)
    {
        item->update(status, timestamp);
    }

    qDeleteAll(m_items);
    m_items.clear();

    qDeleteAll(m_devices);
    m_devices.clear();

    qDeleteAll(m_threads);
    m_threads.clear();

    qDeleteAll(m_ports);
    m_ports.clear();
}

void mbClientRuntime::sendPortMessage(mb::Client::PortHandle_t handle, const mbClientRunMessagePtr &message)
{
    mbClientRunPort *rd = m_ports.value(handle);
    if (rd)
        rd->pushExternalMessage(message);
    else
        message->setComplete(Modbus::Status_Bad, mb::currentTimestamp());
}

void mbClientRuntime::sendMessage(mb::Client::DeviceHandle_t handle, const mbClientRunMessagePtr &message)
{
    mbClientRunDevice *rd = m_devices.value(handle);
    if (rd)
        rd->pushExternalMessage(message);
    else
        message->setComplete(Modbus::Status_Bad, mb::currentTimestamp());
}

void mbClientRuntime::updateItem(mb::Client::ItemHandle_t handle, const QByteArray &data, mb::StatusCode status, mb::Timestamp_t timestamp)
{
    if (!m_items.contains(handle))
        return;
    mbClientDataViewItem *item = handle;
    item->update(data, status, timestamp);
}

void mbClientRuntime::writeItemData(mb::Client::ItemHandle_t handle, const QByteArray &data)
{
    if (!m_items.contains(handle))
        return;
    mbClientDataViewItem *item = handle;
    mbClientDevice *d = item->device();
    mbClientRunDevice *rd = m_devices.value(d);
    if (rd)
    {
        mbClientRunItem *ri = createRunItem(item, data);
        rd->pushItemToWrite(ri);
    }
}

mbClientRunItem *mbClientRuntime::createRunItem(mbClientDataViewItem *item)
{
    mbClientRunItem *t = new mbClientRunItem(item->handle(),
                                             item->addressType(),
                                             item->addressOffset(),
                                             item->count(),
                                             item->sizeOf());
    m_items.insert(item, t);
    return t;
}

mbClientRunItem *mbClientRuntime::createRunItem(mbClientDataViewItem *item, const QByteArray &data)
{
    mbClientRunItem *t = new mbClientRunItem(item->handle(),
                                             item->addressType(),
                                             item->addressOffset(),
                                             item->count(),
                                             data);
    return t;
}

mbClientRunPort *mbClientRuntime::createRunPort(mbClientPort *port)
{
    mbClientRunPort *t = new mbClientRunPort(port);
    m_ports.insert(port, t);
    return t;
}

mbClientRunDevice *mbClientRuntime::createRunDevice(mbClientDevice *device)
{
    mbClientRunDevice *t = new mbClientRunDevice(device);
    m_devices.insert(device, t);
    return t;
}

mbClientRunThread *mbClientRuntime::createRunThread(mbClientRunPort *port)
{
    mbClientRunThread *t = new mbClientRunThread(port);
    m_threads.insert(port, t);
    return t;
}
