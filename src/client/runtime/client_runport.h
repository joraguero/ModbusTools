#ifndef CLIENT_RUNPORT_H
#define CLIENT_RUNPORT_H

#include <QQueue>
#include <QReadWriteLock>

#include <client_global.h>
#include <project/client_port.h>

#include "client_runmessage.h"

class mbClientRunDevice;
class mbClientDataView;

class mbClientRunPort
{
public:
    // Note: one Modbus request of the data view window which is polled in turn with other windows of the port
    struct PollEntry
    {
        mbClientRunDevice    *device  ;
        mbClientRunMessagePtr message ;
        mbClientDataView     *dataView;
        uint32_t              scanRate; // period (ms) of the request of the data view
    };

public:
    explicit mbClientRunPort(mbClientPort *port);

public:
    inline mbClientPort *port() { return m_port; }

public: // mbClientPort interface
    // Note: as port in runtime is unchangeable so access functions can be used safely
    inline QString name() const { return m_port->name(); }
    inline MBSETTINGS settings() const { return m_port->settings(); }
    inline bool isBroadcastEnabled() const { return m_port->isBroadcastEnabled(); }
    inline uint32_t pollDelay() const { return m_pollDelay; }

public:
    inline QList<mbClientRunDevice*> devices() const { return m_devices; }
    inline void pushDevices(const QList<mbClientRunDevice*> &devices) { m_devices.append(devices); }

public:
    inline QList<PollEntry> pollEntries() const { return m_pollEntries; }
    inline void pushPollEntry(const PollEntry &entry) { m_pollEntries.append(entry); }

public:
    bool hasExternalMessage() const;
    void pushExternalMessage(const mbClientRunMessagePtr &message);
    bool popExternalMessage(mbClientRunMessagePtr *message);

private:
    mutable QReadWriteLock m_lock;
    mbClientPort *m_port;
    uint32_t m_pollDelay;
    QList<mbClientRunDevice*> m_devices;
    QList<PollEntry> m_pollEntries;
    QQueue<mbClientRunMessagePtr> m_externalMessages;
};

#endif // CLIENT_RUNPORT_H
