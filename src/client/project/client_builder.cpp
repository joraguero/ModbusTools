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
#include "client_builder.h"

#include <QDir>
#include <QSettings>

#include "client_project.h"
#include "client_port.h"
#include "client_device.h"
#include "client_dataview.h"

#include "client_dom.h"

#include <client.h>

mbClientBuilder::Strings::Strings() :
    sep(';')
{
}

const mbClientBuilder::Strings &mbClientBuilder::Strings::instance()
{
    static const Strings s;
    return s;
}

mbClientBuilder::mbClientBuilder(QObject *parent) : mbCoreBuilder(parent)
{
}

QStringList mbClientBuilder::csvDataViewItemAttributes() const
{
    return mbCoreBuilder::csvDataViewItemAttributes();
}

mbCoreProject *mbClientBuilder::newProject() const
{
    return new mbClientProject;
}

mbCorePort *mbClientBuilder::newPort() const
{
    return new mbClientPort;
}

mbCoreDevice *mbClientBuilder::newDevice() const
{
    return new mbClientDevice;
}

mbCoreDataView *mbClientBuilder::newDataView() const
{
    return new mbClientDataView;
}

mbCoreDataViewItem *mbClientBuilder::newDataViewItem() const
{
    return new mbClientDataViewItem;
}

mbCoreDomProject *mbClientBuilder::newDomProject() const
{
    return new mbClientDomProject;
}

mbCoreDomPort *mbClientBuilder::newDomPort() const
{
    return new mbClientDomPort;
}

mbCoreDomDevice *mbClientBuilder::newDomDevice() const
{
    return new mbClientDomDevice;
}

mbCoreDomDataView *mbClientBuilder::newDomDataView() const
{
    return new mbClientDomDataView;
}

mbCoreDomDataViewItem *mbClientBuilder::newDomDataViewItem() const
{
    return new mbClientDomDataViewItem;
}

void mbClientBuilder::fillProject(mbCoreProject *obj, const mbCoreDomProject *dom)
{
    mbCoreBuilder::fillProject(obj, dom);
    mbClientProject *project = static_cast<mbClientProject*>(obj);
    setWorkingProjectCore(project);
    Q_FOREACH (mbClientDevice *device, project->devices())
    {
        mbClientPort *port = project->port(device->portName());
        if (port)
            port->deviceAdd(device);
    }
    setWorkingProjectCore(nullptr);
}

void mbClientBuilder::fillDataView(mbCoreDataView *obj, const mbCoreDomDataView *dom)
{
    mbCoreBuilder::fillDataView(obj, dom);
    mbClientDataView *dataView = static_cast<mbClientDataView*>(obj);
    const mbClientDomDataView *domDataView = dynamic_cast<const mbClientDomDataView*>(dom);
    MBSETTINGS request;
    if (domDataView)
        request = domDataView->request();
    if (request.isEmpty())
    {
        // Project of previous version: data view has no own request
        mbClientProject *p = project();
        dataView->setRequestFromItems(p && p->deviceCount() ? p->device(0) : nullptr);
    }
    else
    {
        const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();
        mbClientProject *p = project();
        bool ok;
        dataView->setRequestDevice(p ? p->device(request.value(s.requestDevice).toString()) : nullptr);
        int func = request.value(s.requestFunction).toInt(&ok);
        if (ok)
            dataView->setRequestFunction(func);
        int offset = request.value(s.requestAddress).toInt(&ok);
        if (ok && (offset >= 0) && (offset <= USHRT_MAX))
            dataView->setRequestOffset(static_cast<quint16>(offset));
        int quantity = request.value(s.requestQuantity).toInt(&ok);
        if (ok)
            dataView->setRequestQuantity(quantity);
    }
    dataView->rebuildItems();
}

void mbClientBuilder::fillDomDataView(mbCoreDomDataView *dom, const mbCoreDataView *obj)
{
    mbCoreBuilder::fillDomDataView(dom, obj);
    mbClientDomDataView *domDataView = dynamic_cast<mbClientDomDataView*>(dom);
    if (!domDataView)
        return;
    const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();
    const mbClientDataView *dataView = static_cast<const mbClientDataView*>(obj);
    MBSETTINGS request;
    request[s.requestDevice  ] = dataView->requestDevice() ? dataView->requestDevice()->name() : QString();
    request[s.requestFunction] = dataView->requestFunction();
    request[s.requestAddress ] = dataView->requestOffset();
    request[s.requestQuantity] = dataView->requestQuantity();
    domDataView->setRequest(request);
}

void mbClientBuilder::fillDataViewItem (mbCoreDataViewItem *obj, const mbCoreDomDataViewItem *dom)
{
    mbCoreBuilder::fillDataViewItem(obj, dom);
    QVariant v = dom->settings().value(mbClientDataViewItem::Strings::instance().value);
    obj->setValue(v);
}

void mbClientBuilder::fillDomDataViewItem(mbCoreDomDataViewItem *dom, const mbCoreDataViewItem *obj)
{
    mbCoreBuilder::fillDomDataViewItem(dom, obj);
    MBSETTINGS s = dom->settings();
    s[mbClientDataViewItem::Strings::instance().value] = obj->value();
    dom->setSettings(s);
}

void mbClientBuilder::importDomProject(mbCoreDomProject *dom)
{
    Q_FOREACH (mbCoreDomDevice *d, dom->devices())
    {
        MBSETTINGS s = d->settings();
        QString name = s.value(mbClientDevice::Strings::instance().name).toString();
        QString portName = s.value(mbClientDevice::Strings::instance().portName).toString();
        mbClientPort *port = project()->port(portName);
        if (port)
        {
            mbClientDevice *device = project()->device(name);
            if (device)
                port->deviceAdd(device);
        }
    }
}
