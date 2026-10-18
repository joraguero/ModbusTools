/*
    Modbus Tools

    Created: 2026

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
#include "client_dialogdataview.h"

#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>

#include <client.h>
#include <project/client_project.h>
#include <project/client_device.h>
#include <project/client_dataview.h>

mbClientDialogDataView::mbClientDialogDataView(QWidget *parent) :
    mbCoreDialogDataView(parent)
{
    // Note: period of the data view is the scan rate of its request (one request every 'period' ms).
    //       Requests of the windows of the same port are executed one at a time with port's 'Poll delay' between them
    setPeriodLabel(QStringLiteral("Scan rate (ms)"));

    const mbClientDataView::Defaults &d = mbClientDataView::Defaults::instance();

    m_cmbDevice = new QComboBox(this);

    m_cmbFunction = new QComboBox(this);
    m_cmbFunction->addItem(QStringLiteral("01 Read Coils"            ), MBF_READ_COILS            );
    m_cmbFunction->addItem(QStringLiteral("02 Read Discrete Inputs"  ), MBF_READ_DISCRETE_INPUTS  );
    m_cmbFunction->addItem(QStringLiteral("03 Read Holding Registers"), MBF_READ_HOLDING_REGISTERS);
    m_cmbFunction->addItem(QStringLiteral("04 Read Input Registers"  ), MBF_READ_INPUT_REGISTERS  );
    m_cmbFunction->setCurrentIndex(m_cmbFunction->findData(mbClientDataView::functionByMemoryType(d.requestMemoryType)));

    m_spAddress = new QSpinBox(this);
    m_spAddress->setRange(0, USHRT_MAX);
    m_spAddress->setValue(d.requestOffset);
    m_spAddress->setToolTip(QStringLiteral("Start address (offset) of the request, 0-based"));

    m_spQuantity = new QSpinBox(this);
    m_spQuantity->setRange(1, MB_MAX_REGISTERS);
    m_spQuantity->setValue(d.requestQuantity);

    // Insert request fields right after the name of the data view
    QFormLayout *l = formLayout();
    l->insertRow(1, QStringLiteral("Device"  ), m_cmbDevice  );
    l->insertRow(2, QStringLiteral("Function"), m_cmbFunction);
    l->insertRow(3, QStringLiteral("Address" ), m_spAddress  );
    l->insertRow(4, QStringLiteral("Quantity"), m_spQuantity );

    connect(m_cmbDevice  , QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mbClientDialogDataView::updateMaxQuantity);
    connect(m_cmbFunction, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mbClientDialogDataView::updateMaxQuantity);
}

MBSETTINGS mbClientDialogDataView::getSettings(const MBSETTINGS &settings, const QString &title)
{
    // Refresh list of the devices of current project
    QString current = m_cmbDevice->currentText();
    m_cmbDevice->clear();
    if (mbClientProject *project = mbClient::global()->project())
    {
        Q_FOREACH (mbClientDevice *device, project->devices())
            m_cmbDevice->addItem(device->name());
    }
    if (settings.isEmpty())
    {
        // New data view: default request
        const mbClientDataView::Defaults &d = mbClientDataView::Defaults::instance();
        m_cmbDevice->setCurrentIndex(m_cmbDevice->count() ? 0 : -1);
        m_cmbFunction->setCurrentIndex(m_cmbFunction->findData(mbClientDataView::functionByMemoryType(d.requestMemoryType)));
        m_spAddress->setValue(d.requestOffset);
        m_spQuantity->setValue(d.requestQuantity);
    }
    else
        m_cmbDevice->setCurrentText(current);
    updateMaxQuantity();
    return mbCoreDialogDataView::getSettings(settings, title);
}

void mbClientDialogDataView::fillFormInner(const MBSETTINGS &m)
{
    const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();

    MBSETTINGS::const_iterator it;
    MBSETTINGS::const_iterator end = m.end();

    it = m.find(s.requestDevice);
    if (it != end)
        m_cmbDevice->setCurrentIndex(m_cmbDevice->findText(it.value().toString()));

    it = m.find(s.requestFunction);
    if (it != end)
    {
        int i = m_cmbFunction->findData(it.value().toInt());
        if (i >= 0)
            m_cmbFunction->setCurrentIndex(i);
    }

    updateMaxQuantity();
    it = m.find(s.requestAddress ); if (it != end) m_spAddress ->setValue(it.value().toInt());
    it = m.find(s.requestQuantity); if (it != end) m_spQuantity->setValue(it.value().toInt());
}

void mbClientDialogDataView::fillDataInner(MBSETTINGS &settings)
{
    const mbClientDataView::Strings &s = mbClientDataView::Strings::instance();

    settings[s.requestDevice  ] = m_cmbDevice  ->currentText();
    settings[s.requestFunction] = m_cmbFunction->currentData();
    settings[s.requestAddress ] = m_spAddress  ->value();
    settings[s.requestQuantity] = m_spQuantity ->value();
}

void mbClientDialogDataView::updateMaxQuantity()
{
    mbClientDevice *device = nullptr;
    if (mbClientProject *project = mbClient::global()->project())
        device = project->device(m_cmbDevice->currentText());
    Modbus::MemoryType memoryType = mbClientDataView::memoryTypeByFunction(m_cmbFunction->currentData().toInt());
    int max = mbClientDataView::maxQuantity(device, memoryType);
    if (max > 0)
        m_spQuantity->setMaximum(max);
}
