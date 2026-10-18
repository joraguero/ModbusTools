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
#include "client_dataview.h"

#include "client.h"
#include "client_project.h"
#include "client_device.h"

mbClientDataViewItem::mbClientDataViewItem(QObject *parent) :
    mbCoreDataViewItem(parent)
{
    m_status = mb::Status_MbStopped;
    m_timestamp = mb::currentTimestamp();
    m_cache = toVariant(m_value);
}

void mbClientDataViewItem::setFormat(mb::Format format)
{
    QWriteLocker _(&m_lock);
    if (!mbClient::global()->isRunning() || isFormatEqualSize((format)))
    {
        this->blockSignals(true);
        mbCoreDataViewItem::setFormat(format);
        m_cache = toVariant(m_value);
        this->blockSignals(false);
        Q_EMIT changed();
    }
}

bool mbClientDataViewItem::isFormatEqualSize(mb::Format format) const
{
    switch (m_format)
    {
    case mb::Bin16:
    case mb::Oct16:
    case mb::Dec16:
    case mb::UDec16:
    case mb::Hex16:
        switch (format)
        {
        case mb::Bin16:
        case mb::Oct16:
        case mb::Dec16:
        case mb::UDec16:
        case mb::Hex16:
            return true;
        default:
            return false;
        }
    case mb::Bin32:
    case mb::Oct32:
    case mb::Dec32:
    case mb::UDec32:
    case mb::Hex32:
    case mb::Float:
        switch (format)
        {
        case mb::Bin32:
        case mb::Oct32:
        case mb::Dec32:
        case mb::UDec32:
        case mb::Hex32:
        case mb::Float:
            return true;
        default:
            return false;
        }
    case mb::Bin64:
    case mb::Oct64:
    case mb::Dec64:
    case mb::UDec64:
    case mb::Hex64:
    case mb::Double:
        switch (format)
        {
        case mb::Bin64:
        case mb::Oct64:
        case mb::Dec64:
        case mb::UDec64:
        case mb::Hex64:
        case mb::Double:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

QVariant mbClientDataViewItem::value() const
{
    QReadLocker _(&m_lock);
    return m_cache;
}

void mbClientDataViewItem::setValue(const QVariant &value)
{
    m_lock.lockForRead();
    QByteArray data = toByteArray(value); // Note: m_format may change
    m_lock.unlock();
    if (data.length() > 0)
    {
        if (mbClient::global()->isRunning())
        {
            if (!isReadOnly() && m_device)
                mbClient::global()->writeItemData(handle(), data);
        }
        else
            update(data, mb::Status_MbStopped, mb::currentTimestamp());
    }
}

void mbClientDataViewItem::update(const QByteArray &value, mb::StatusCode status, mb::Timestamp_t timestamp)
{
    QWriteLocker _(&m_lock);
    if (value.count())
    {
        m_value = value;
        QVariant newCacheValue = toVariant(m_value);
        if (m_cache != newCacheValue)
        {
            m_cache = newCacheValue;
            m_status = status;
            m_timestamp = timestamp;
            Q_EMIT valueChanged();
            return;
        }
    }
    if (m_status != status)
    {
        m_status = status;
        m_timestamp = timestamp;
        Q_EMIT valueChanged();
        return;
    }
}

mbClientDataView::Strings::Strings() :
    mbCoreDataView::Strings(),
    requestDevice  (QStringLiteral("requestDevice")),
    requestFunction(QStringLiteral("requestFunction")),
    requestAddress (QStringLiteral("requestAddress")),
    requestQuantity(QStringLiteral("requestQuantity"))
{
}

const mbClientDataView::Strings &mbClientDataView::Strings::instance()
{
    static const Strings s;
    return s;
}

mbClientDataView::Defaults::Defaults() :
    mbCoreDataView::Defaults(),
    requestMemoryType(Modbus::Memory_4x),
    requestOffset(0),
    requestQuantity(10)
{
}

const mbClientDataView::Defaults &mbClientDataView::Defaults::instance()
{
    static const Defaults d;
    return d;
}

mbClientDataView::Statistics::Statistics() :
    countTx   (0),
    countErr  (0),
    lastStatus(Modbus::Status_Uncertain)
{
}

QStringList mbClientDataView::availableColumnNames()
{
    // Note: there is no 'Device' column: all rows belong to the device of the request (ID in the header)
    QStringList res = mbCoreDataView::availableColumnNames();
    res.removeAll(QString(QMetaEnum::fromType<mbCoreDataView::CoreColumns>().valueToKey(mbCoreDataView::Device)));
    QMetaEnum me = QMetaEnum::fromType<ClientColumns>();
    for (int i = 0; i < (ColumnCount-mbCoreDataView::ColumnCount); i++)
        res.append(me.key(i));
    return res;
}

Modbus::MemoryType mbClientDataView::memoryTypeByFunction(int func)
{
    switch (func)
    {
    case MBF_READ_COILS            : return Modbus::Memory_0x;
    case MBF_READ_DISCRETE_INPUTS  : return Modbus::Memory_1x;
    case MBF_READ_HOLDING_REGISTERS: return Modbus::Memory_4x;
    case MBF_READ_INPUT_REGISTERS  : return Modbus::Memory_3x;
    default:
        return Modbus::Memory_Unknown;
    }
}

int mbClientDataView::functionByMemoryType(Modbus::MemoryType memoryType)
{
    switch (memoryType)
    {
    case Modbus::Memory_0x: return MBF_READ_COILS            ;
    case Modbus::Memory_1x: return MBF_READ_DISCRETE_INPUTS  ;
    case Modbus::Memory_4x: return MBF_READ_HOLDING_REGISTERS;
    case Modbus::Memory_3x: return MBF_READ_INPUT_REGISTERS  ;
    default:
        return 0;
    }
}

int mbClientDataView::maxQuantity(const mbClientDevice *device, Modbus::MemoryType memoryType)
{
    switch (memoryType)
    {
    case Modbus::Memory_0x: return device ? device->maxReadCoils           () : MB_MAX_DISCRETS;
    case Modbus::Memory_1x: return device ? device->maxReadDiscreteInputs  () : MB_MAX_DISCRETS;
    case Modbus::Memory_3x: return device ? device->maxReadInputRegisters  () : MB_MAX_REGISTERS;
    case Modbus::Memory_4x: return device ? device->maxReadHoldingRegisters() : MB_MAX_REGISTERS;
    default:
        return 0;
    }
}

mbClientDataView::mbClientDataView(QObject *parent)
    : mbCoreDataView{parent}
{
    const Defaults &d = Defaults::instance();
    m_request.memoryType = d.requestMemoryType;
    m_request.offset     = d.requestOffset;
    m_request.quantity   = d.requestQuantity;
    m_columns.removeAll(mbCoreDataView::Device);
    for (int i = mbCoreDataView::ColumnCount; i < ColumnCount; i++)
        m_columns.append(i);
}

int mbClientDataView::columnTypeByName(const QString &name) const
{
    int res = mbCoreDataView::columnTypeByName(name);
    if (res == mbCoreDataView::Device) // Note: device is shown in the header of the data view (ID)
        return -1;
    if (res < 0)
    {
        QMetaEnum me = QMetaEnum::fromType<ClientColumns>();
        res = me.keyToValue(name.toUtf8().constData());
    }
    return res;
}

QString mbClientDataView::columnNameByIndex(int i) const
{
    int type = m_columns.value(i, -1);
    if (type < mbCoreDataView::ColumnCount)
        return mbCoreDataView::columnNameByIndex(i);
    else
        return QMetaEnum::fromType<ClientColumns>().valueToKey(type);;
}

mbClientDevice *mbClientDataView::requestDevice() const
{
    return m_request.device;
}

void mbClientDataView::setRequestDevice(mbClientDevice *device)
{
    if (m_request.device != device)
    {
        m_request.device = device;
        Q_EMIT requestChanged();
    }
}

void mbClientDataView::setRequestMemoryType(Modbus::MemoryType memoryType)
{
    if (!functionByMemoryType(memoryType))
        return;
    if (m_request.memoryType != memoryType)
    {
        m_request.memoryType = memoryType;
        Q_EMIT requestChanged();
    }
}

void mbClientDataView::setRequestOffset(quint16 offset)
{
    if (m_request.offset != offset)
    {
        m_request.offset = offset;
        Q_EMIT requestChanged();
    }
}

void mbClientDataView::setRequestQuantity(int quantity)
{
    if (quantity < 1)
        quantity = 1;
    else if (quantity > MB_MAX_DISCRETS)
        quantity = MB_MAX_DISCRETS;
    if (m_request.quantity != quantity)
    {
        m_request.quantity = quantity;
        Q_EMIT requestChanged();
    }
}

bool mbClientDataView::isRequestValid() const
{
    return m_request.device && (m_request.quantity > 0) && functionByMemoryType(m_request.memoryType);
}

int mbClientDataView::requestEnd() const
{
    // Note: the same quantity as the read message created by the runtime
    int count = qMin(m_request.quantity, maxQuantity(m_request.device, m_request.memoryType));
    return static_cast<int>(m_request.offset) + qMax(count, 0);
}

mbClientDataView::SpanState mbClientDataView::spanState(int index) const
{
    if ((index < 0) || (index >= m_items.count()))
        return Span_Normal;
    int coveredEnd = -1; // end (exclusive) of the value of the last decoded row
    for (int i = 0; i < index; i++)
    {
        const mbCoreDataViewItem *item = m_items.at(i);
        int offset = item->addressOffset();
        if (offset < coveredEnd)
            continue;
        coveredEnd = offset + qMax(item->length(), 1);
    }
    const mbCoreDataViewItem *item = m_items.at(index);
    int offset = item->addressOffset();
    if (offset < coveredEnd)
        return Span_Covered;
    if (offset + qMax(item->length(), 1) > requestEnd())
        return Span_Incomplete;
    return Span_Normal;
}

void mbClientDataView::rebuildItems()
{
    // Keep rows (with their format, comment etc.) which address is still inside of the request
    QHash<quint16, mbCoreDataViewItem*> keep;
    QList<mbCoreDataViewItem*> drop;
    Q_FOREACH (mbCoreDataViewItem *item, m_items)
    {
        if ((item->addressType() == m_request.memoryType) && !keep.contains(item->addressOffset()))
            keep.insert(item->addressOffset(), item);
        else
            drop.append(item);
    }
    while (itemCount())
        mbCoreDataView::itemRemove(itemCount()-1);

    for (int i = 0; i < m_request.quantity; i++)
    {
        int offset = m_request.offset + i;
        if (offset > USHRT_MAX)
            break;
        mbClientDataViewItem *item = static_cast<mbClientDataViewItem*>(keep.take(static_cast<quint16>(offset)));
        if (!item)
        {
            item = new mbClientDataViewItem;
            item->setAddress(m_request.memoryType, static_cast<quint16>(offset));
            if ((m_request.memoryType == Modbus::Memory_0x) || (m_request.memoryType == Modbus::Memory_1x))
                item->setFormat(mb::Bool);
        }
        item->setDevice(m_request.device);
        itemAdd(item);
    }
    qDeleteAll(keep);
    qDeleteAll(drop);
}

void mbClientDataView::setRequestFromItems(mbClientDevice *defaultDevice)
{
    // Request is defined by device and memory type of the first row
    // and by the address range of the rows with the same device and memory type
    mbClientDevice *device = nullptr;
    Modbus::MemoryType memoryType = Modbus::Memory_Unknown;
    Q_FOREACH (mbClientDataViewItem *item, items())
    {
        if (item->device() && functionByMemoryType(item->addressType()))
        {
            device = item->device();
            memoryType = item->addressType();
            break;
        }
    }
    if (!device)
    {
        setRequestDevice(defaultDevice);
        return;
    }

    int begin = USHRT_MAX+1;
    int end = 0;
    Q_FOREACH (mbClientDataViewItem *item, items())
    {
        if ((item->device() == device) && (item->addressType() == memoryType))
        {
            begin = qMin(begin, static_cast<int>(item->addressOffset()));
            end   = qMax(end  , item->addressOffset() + qMax(item->length(), 1));
        }
    }
    int quantity = qMin(end - begin, maxQuantity(device, memoryType));

    int dropped = 0;
    Q_FOREACH (mbClientDataViewItem *item, items())
    {
        int offset = item->addressOffset();
        if ((item->device() != device) || (item->addressType() != memoryType) ||
            (offset < begin) || (offset >= begin + quantity))
        {
            itemRemove(item);
            delete item;
            ++dropped;
        }
    }

    setRequestDevice(device);
    setRequestMemoryType(memoryType);
    setRequestOffset(static_cast<quint16>(begin));
    setRequestQuantity(quantity);

    if (dropped)
        mbClient::LogWarning(name(),
                             QString("%1 item(s) don't fit into the data view request "
                                     "(device '%2', function %3, address %4, quantity %5) and were removed")
                                 .arg(dropped)
                                 .arg(device->name())
                                 .arg(functionByMemoryType(memoryType), 2, 10, QLatin1Char('0'))
                                 .arg(begin)
                                 .arg(quantity));
}

MBSETTINGS mbClientDataView::settings() const
{
    const Strings &s = Strings::instance();

    MBSETTINGS p = mbCoreDataView::settings();
    p[s.requestDevice  ] = m_request.device ? m_request.device->name() : QString();
    p[s.requestFunction] = requestFunction();
    p[s.requestAddress ] = requestOffset();
    p[s.requestQuantity] = requestQuantity();
    return p;
}

bool mbClientDataView::setSettings(const MBSETTINGS &settings)
{
    const Strings &s = Strings::instance();

    mbCoreDataView::setSettings(settings);

    MBSETTINGS::ConstIterator it;
    MBSETTINGS::ConstIterator end = settings.constEnd();
    bool ok;
    bool request = false;

    it = settings.find(s.requestDevice);
    if (it != end)
    {
        mbClientProject *p = project();
        if (!p)
            p = mbClient::global()->project();
        setRequestDevice(p ? p->device(it.value().toString()) : nullptr);
        request = true;
    }

    it = settings.find(s.requestFunction);
    if (it != end)
    {
        int v = it.value().toInt(&ok);
        if (ok)
            setRequestFunction(v);
        request = true;
    }

    it = settings.find(s.requestAddress);
    if (it != end)
    {
        int v = it.value().toInt(&ok);
        if (ok && (v >= 0) && (v <= USHRT_MAX))
            setRequestOffset(static_cast<quint16>(v));
        request = true;
    }

    it = settings.find(s.requestQuantity);
    if (it != end)
    {
        int v = it.value().toInt(&ok);
        if (ok)
            setRequestQuantity(v);
        request = true;
    }

    if (request)
        rebuildItems();
    return true;
}

void mbClientDataView::resetStatistics()
{
    {
        QWriteLocker _(&m_statLock);
        m_stat = Statistics();
    }
    Q_EMIT statisticsChanged();
}

void mbClientDataView::setStatRequestStarted()
{
    {
        QWriteLocker _(&m_statLock);
        m_stat.countTx++;
    }
    Q_EMIT statisticsChanged();
}

void mbClientDataView::setStatRequestCompleted(Modbus::StatusCode status, const QString &errorText)
{
    {
        QWriteLocker _(&m_statLock);
        if (Modbus::StatusIsBad(status))
        {
            m_stat.countErr++;
            m_stat.lastErrorText = errorText;
        }
        else
            m_stat.lastErrorText.clear();
        m_stat.lastStatus = status;
    }
    Q_EMIT statisticsChanged();
}

