#include "core_widgetsettingsview.h"
#include "ui_core_widgetsettingsview.h"

mbCoreWidgetSettingsView::mbCoreWidgetSettingsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mbCoreWidgetSettingsView)
{
    ui->setupUi(this);

    QComboBox *cmb;
    cmb = ui->cmbAddressNotation;
    // Note: item data is the value of 'mb::AddressNotation'
    const mb::AddressNotation notations[] = { mb::Address::Notation_Modbus0    ,
                                              mb::Address::Notation_Modbus     ,
                                              mb::Address::Notation_IEC61131   ,
                                              mb::Address::Notation_IEC61131Hex };
    for (mb::AddressNotation n : notations)
        cmb->addItem(mb::toFineString(n), static_cast<int>(n));
    setAddressNotation(mb::Address::Notation_Modbus);
}

mbCoreWidgetSettingsView::~mbCoreWidgetSettingsView()
{
    delete ui;
}

bool mbCoreWidgetSettingsView::useNameWithSettings() const
{
    return ui->chbUseNameWithSettings->isChecked();
}

void mbCoreWidgetSettingsView::setUseNameWithSettings(bool use)
{
    ui->chbUseNameWithSettings->setChecked(use);
}

mb::AddressNotation mbCoreWidgetSettingsView::addressNotation() const
{
    return static_cast<mb::AddressNotation>(ui->cmbAddressNotation->currentData().toInt());
}

void mbCoreWidgetSettingsView::setAddressNotation(mb::AddressNotation notation)
{
    int i = ui->cmbAddressNotation->findData(static_cast<int>(notation));
    if (i < 0) // Note: 'Notation_Default' is 'Notation_Modbus'
        i = ui->cmbAddressNotation->findData(static_cast<int>(mb::Address::Notation_Modbus));
    ui->cmbAddressNotation->setCurrentIndex(i);
}

