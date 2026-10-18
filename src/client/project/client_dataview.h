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
#ifndef CLIENT_DATAVIEW_H
#define CLIENT_DATAVIEW_H

#include <QReadWriteLock>
#include <QPointer>

#include <client_global.h>

#include <project/core_dataview.h>

class mbClientProject;
class mbClientDevice;

class mbClientDataViewItem : public mbCoreDataViewItem
{
    Q_OBJECT

public:
    explicit mbClientDataViewItem(QObject *parent = nullptr);

public:
    inline mb::Client::ItemHandle_t handle() const { return const_cast<mb::Client::ItemHandle_t>(this); }
    inline bool isReadOnly() const { return m_address.type() == Modbus::Memory_1x || m_address.type() == Modbus::Memory_3x; }

public: // settings
    inline mbClientDevice *device() const { return reinterpret_cast<mbClientDevice*>(deviceCore()); }
    inline void setDevice(mbClientDevice *device) { setDeviceCore(reinterpret_cast<mbCoreDevice*>(device)); }

    void setFormat(mb::Format format) override;
    bool isFormatEqualSize(mb::Format format) const;

public:
    QVariant value() const override;
    void setValue(const QVariant& value) override;

public:
    inline mb::StatusCode status() const { return m_status; }        // TODO: make thread safe
    inline mb::Timestamp_t timestamp() const { return m_timestamp; } // TODO: make thread safe
    void update(const QByteArray &data, mb::StatusCode status, mb::Timestamp_t timestamp);
    inline void update(mb::StatusCode status, mb::Timestamp_t timestamp) { update(QByteArray(), status, timestamp); }

private:
    mb::StatusCode m_status;
    mb::Timestamp_t m_timestamp;
    QByteArray m_value;
    QVariant m_cache;

private:
    mutable QReadWriteLock m_lock;
};

class mbClientDataView : public mbCoreDataView
{
    Q_OBJECT

public:
    struct Strings : public mbCoreDataView::Strings
    {
        const QString requestDevice  ;
        const QString requestFunction;
        const QString requestAddress ;
        const QString requestQuantity;

        Strings();
        static const Strings &instance();
    };

    struct Defaults : public mbCoreDataView::Defaults
    {
        const Modbus::MemoryType requestMemoryType;
        const quint16            requestOffset    ;
        const int                requestQuantity  ;

        Defaults();
        static const Defaults &instance();
    };

    struct Statistics
    {
        quint32            countTx      ;
        quint32            countErr     ;
        Modbus::StatusCode lastStatus   ;
        QString            lastErrorText;

        Statistics();
    };

public:
    enum ClientColumns
    {
        // Note: there is no 'Status' column: status of the request is shown in the header of the data view
        Timestamp = mbCoreDataView::ColumnCount,
        ColumnCount
    };
    Q_ENUM(ClientColumns)

    static QStringList availableColumnNames();

    // Note: New request functions: 1 - Read Coils, 2 - Read Discrete Inputs,
    //       3 - Read Holding Registers, 4 - Read Input Registers
    static Modbus::MemoryType memoryTypeByFunction(int func);
    static int functionByMemoryType(Modbus::MemoryType memoryType);
    static int maxQuantity(const mbClientDevice *device, Modbus::MemoryType memoryType);

public:
    explicit mbClientDataView(QObject *parent = nullptr);

public:
    inline mbClientProject* project() const { return reinterpret_cast<mbClientProject*>(mbCoreDataView::projectCore()); }
    inline void setProject(mbClientProject* project) { mbCoreDataView::setProjectCore(reinterpret_cast<mbCoreProject*>(project)); }

public:
    int columnTypeByName(const QString &name) const override;
    QString columnNameByIndex(int i) const override;

public: // request
    mbClientDevice *requestDevice() const;
    void setRequestDevice(mbClientDevice *device);
    inline Modbus::MemoryType requestMemoryType() const { return m_request.memoryType; }
    void setRequestMemoryType(Modbus::MemoryType memoryType);
    inline int requestFunction() const { return functionByMemoryType(m_request.memoryType); }
    inline void setRequestFunction(int func) { setRequestMemoryType(memoryTypeByFunction(func)); }
    inline quint16 requestOffset() const { return m_request.offset; }
    void setRequestOffset(quint16 offset);
    inline int requestQuantity() const { return m_request.quantity; }
    void setRequestQuantity(int quantity);
    bool isRequestValid() const;
    void rebuildItems();
    // Note: defines request by the rows of the data view of previous versions (that had no own request)
    void setRequestFromItems(mbClientDevice *defaultDevice);

    MBSETTINGS settings() const override;
    bool setSettings(const MBSETTINGS& settings) override;

public: // statistics (thread safe)
    inline Statistics statistics() const { QReadLocker _(&m_statLock); return m_stat; }
    void resetStatistics();
    void setStatRequestStarted();
    void setStatRequestCompleted(Modbus::StatusCode status, const QString &errorText);

Q_SIGNALS:
    void requestChanged();
    void statisticsChanged();

public: // items
    inline bool hasItem(mbClientDataViewItem* item) const { return mbCoreDataView::hasItem(static_cast<mbCoreDataViewItem*>(item)); }
    inline QList<mbClientDataViewItem*> items() const { return QList<mbClientDataViewItem*>(*(reinterpret_cast<const QList<mbClientDataViewItem*>*>(&m_items))); }
    inline int itemIndex(mbClientDataViewItem* item) const { return mbCoreDataView::itemIndex(reinterpret_cast<mbClientDataViewItem*>(item)); }
    inline mbClientDataViewItem *item(int i) const { return reinterpret_cast<mbClientDataViewItem*>(mbCoreDataView::itemCore(i)); }
    inline mbClientDataViewItem *itemAt(int i) const { return reinterpret_cast<mbClientDataViewItem*>(mbCoreDataView::itemCoreAt(i)); }
    inline int itemInsert(mbClientDataViewItem* item, int index = -1) { return mbCoreDataView::itemInsert(reinterpret_cast<mbCoreDataViewItem*>(item), index); }
    inline int itemAdd(mbClientDataViewItem* item) { return mbCoreDataView::itemAdd(reinterpret_cast<mbCoreDataViewItem*>(item)); }
    inline int itemRemove(mbClientDataViewItem* item) { return mbCoreDataView::itemRemove(reinterpret_cast<mbCoreDataViewItem*>(item)); }

private:
    struct
    {
        QPointer<mbClientDevice> device    ;
        Modbus::MemoryType       memoryType;
        quint16                  offset    ;
        int                      quantity  ;
    } m_request;

    mutable QReadWriteLock m_statLock;
    Statistics m_stat;
};

#endif // CLIENT_DATAVIEW_H
