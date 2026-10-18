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
#ifndef CLIENT_DIALOGDATAVIEW_H
#define CLIENT_DIALOGDATAVIEW_H

#include <gui/dialogs/core_dialogdataview.h>

class QComboBox;
class QSpinBox;

// Data view dialog with Modbus request of the window (device, function, address, quantity)
class mbClientDialogDataView : public mbCoreDialogDataView
{
    Q_OBJECT

public:
    explicit mbClientDialogDataView(QWidget *parent = nullptr);

public:
    MBSETTINGS getSettings(const MBSETTINGS &settings = MBSETTINGS(), const QString &title = QString()) override;

private:
    void fillFormInner(const MBSETTINGS &settings) override;
    void fillDataInner(MBSETTINGS &settings) override;

private Q_SLOTS:
    void updateMaxQuantity();

private:
    QComboBox *m_cmbDevice  ;
    QComboBox *m_cmbFunction;
    QSpinBox  *m_spAddress  ;
    QSpinBox  *m_spQuantity ;
};

#endif // CLIENT_DIALOGDATAVIEW_H
