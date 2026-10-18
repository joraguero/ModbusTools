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
#include "client_dataviewmodel.h"

#include <QBrush>

#include <core.h>
#include <project/client_project.h>
#include <project/client_device.h>
#include <project/client_dataview.h>

mbClientDataViewModel::mbClientDataViewModel(mbClientDataView *dataView, QObject *parent) :
    mbCoreDataViewModel(dataView, parent)
{
    connect(dataView, &mbCoreDataView::itemChanged, this, &mbClientDataViewModel::itemSpanChanged);
    connect(dataView, &mbCoreDataView::itemAdded  , this, [this]() { m_lengths.clear(); });
    connect(dataView, &mbCoreDataView::itemRemoved, this, [this]() { m_lengths.clear(); });
}

mbClientDataViewModel::~mbClientDataViewModel()
{
}

// Text of the value of the row which is covered by the value of a previous row
// (e.g. 2nd register of a Float) or which needs addresses beyond the end of the request
static const QString CoveredValueStr    = QStringLiteral("--");
static const QString IncompleteValueStr = QStringLiteral("**");

QVariant mbClientDataViewModel::data(const QModelIndex &index, int role) const
{
    mbClientDataViewItem* d = dataView()->item(index.row());
    if (d)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            int c = dataView()->getColumnTypeByIndex(index.column());
            switch(c)
            {
            case mbCoreDataView::Device:
                if (mbClientDevice *dev = d->device())
                    return dev->name();
                break;
            case mbCoreDataView::Address    : return d->addressStr();
            case mbCoreDataView::Format     :
                m_lengths.insert(d, d->length());
                return d->formatStr();
            case mbCoreDataView::Comment    : return d->comment();
            case mbClientDataView::Timestamp:
            case mbCoreDataView::Value      :
                m_lengths.insert(d, d->length());
                switch (dataView()->spanState(index.row()))
                {
                case mbClientDataView::Span_Covered   : return CoveredValueStr;
                case mbClientDataView::Span_Incomplete: return IncompleteValueStr;
                default:
                    break;
                }
                if (c == mbCoreDataView::Value)
                    return d->value();
                return mb::toString(d->timestamp());
            }
        }
            break;
        case Qt::ForegroundRole:
        {
            int c = dataView()->getColumnTypeByIndex(index.column());
            if (((c == mbCoreDataView::Value) || (c == mbClientDataView::Timestamp)) &&
                (dataView()->spanState(index.row()) != mbClientDataView::Span_Normal))
                return QBrush(Qt::gray);
        }
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags mbClientDataViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    int c = dataView()->getColumnTypeByIndex(index.column());
    switch (c)
    {
    // Note: Device and Address of the row are defined by the request of the data view
    case mbCoreDataView::Value:
        // value can be editable only when it's polling mode and the row has its own value
        if (mbClient::global()->isRunning() && (dataView()->spanState(index.row()) == mbClientDataView::Span_Normal))
            f |= Qt::ItemIsEditable;
        break;
    case mbCoreDataView::Format:
    case mbCoreDataView::Comment:
        f |= Qt::ItemIsEditable;
        break;
    }
    return f;
}

bool mbClientDataViewModel::setDataEdit(const QModelIndex &/*index*/, const QVariant &/*value*/, int /*role*/)
{
    return false;
}

void mbClientDataViewModel::itemSpanChanged(mbCoreDataViewItem *item)
{
    // Refresh the following rows when the length of the value of the row is changed (e.g. format Dec16 -> Float)
    int len = item->length();
    QHash<const mbCoreDataViewItem*, int>::iterator it = m_lengths.find(item);
    if ((it != m_lengths.end()) && (it.value() == len))
        return;
    m_lengths.insert(item, len);
    int i = dataView()->itemIndex(reinterpret_cast<mbClientDataViewItem*>(item));
    int last = rowCount() - 1;
    if ((i < 0) || (i >= last))
        return;
    Q_EMIT dataChanged(createIndex(i + 1, 0), createIndex(last, columnCount() - 1));
}

