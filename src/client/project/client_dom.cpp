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
#include "client_dom.h"

#include "client_dataview.h"

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------ DATA VIEW ------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

mbClientDomDataViewItem::mbClientDomDataViewItem() : mbCoreDomDataViewItem()
{
}

mbClientDomDataView::mbClientDomDataView() : mbCoreDomDataView()
{
}

bool mbClientDomDataView::readExtraElement(const QString &tag, mbCoreXmlStreamReader &reader)
{
    const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();
    if ((tag == s.requestDevice  ) ||
        (tag == s.requestFunction) ||
        (tag == s.requestAddress ) ||
        (tag == s.requestQuantity))
    {
        m_request.insert(tag, reader.readElementText());
        return true;
    }
    return false;
}

void mbClientDomDataView::writeExtraElements(mbCoreXmlStreamWriter &writer) const
{
    const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();
    const QStringList tags = { s.requestDevice, s.requestFunction, s.requestAddress, s.requestQuantity };
    Q_FOREACH (const QString &tag, tags)
    {
        MBSETTINGS::ConstIterator it = m_request.find(tag);
        if (it != m_request.constEnd())
            writer.writeTextElement(tag, it.value().toString());
    }
}

// -----------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------- DEVICE -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

mbClientDomDevice::Strings::Strings() : mbCoreDomDevice::Strings()
{
}

const mbClientDomDevice::Strings &mbClientDomDevice::Strings::instance()
{
    static Strings s;
    return s;
}

mbClientDomDevice::mbClientDomDevice() : mbCoreDomDevice()
{
}

// -----------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------- PORT --------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

mbClientDomPort::Strings::Strings() : mbCoreDomPort::Strings()
{
}

const mbClientDomPort::Strings &mbClientDomPort::Strings::instance()
{
    static Strings s;
    return s;
}

mbClientDomPort::mbClientDomPort() : mbCoreDomPort()
{
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------- PROJECT -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

mbClientDomProject::mbClientDomProject() : mbCoreDomProject(new mbClientDomPorts,
                                                            new mbClientDomDevices,
                                                            new mbClientDomDataViews)
{
}

mbClientDomProject::~mbClientDomProject()
{
}
