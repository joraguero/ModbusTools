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
#ifndef CLIENT_RUNITEM_H
#define CLIENT_RUNITEM_H

#include <QDateTime>

#include <client_global.h>

class mbClientRunItem
{
private:
    void init(mb::Client::ItemHandle_t handle, Modbus::MemoryType memoryType, uint16_t offset, uint16_t count);

public:
    mbClientRunItem(mb::Client::ItemHandle_t handle, Modbus::MemoryType memoryType, uint16_t offset, uint16_t count, int size);
    mbClientRunItem(mb::Client::ItemHandle_t handle, Modbus::MemoryType memoryType, uint16_t offset, uint16_t count, const QByteArray &data);
    ~mbClientRunItem();

public:
    inline mbClientRunMessage *message() const { return m_message; }
    inline void setMessage(mbClientRunMessage *message) { m_message = message;}

public:
    inline Modbus::MemoryType memoryType() const { return m_memoryType; }
    inline uint16_t offset() const { return m_offset; }
    inline uint16_t count() const { return m_count; }

public:
    void setData(const QByteArray &data);
    void readDataFromMessage();
    void writeDataToMessage();

protected:
    void update(const QByteArray &data, Modbus::StatusCode status, mb::Timestamp_t timestamp);
    void update(Modbus::StatusCode status, mb::Timestamp_t timestamp);

private:
    mbClientRunMessage *m_message;
    mb::Client::ItemHandle_t m_handle;
    Modbus::MemoryType m_memoryType;
    uint16_t m_offset;
    uint16_t m_count;
    QByteArray m_data;
};

#endif // CLIENT_RUNITEM_H
