#include "gui/SettingsDialog.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");

    darkMode_ = new QCheckBox("Enable Dark Mode");
    showGrid_ = new QCheckBox("Show grid on canvas");
    autosave_ = new QCheckBox("Autosave on exit");

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(darkMode_);
    layout->addWidget(showGrid_);
    layout->addWidget(autosave_);
    layout->addStretch();
    layout->addWidget(buttons_);

    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

bool SettingsDialog::darkMode() const       { return darkMode_->isChecked(); }
bool SettingsDialog::showGrid() const       { return showGrid_->isChecked(); }
bool SettingsDialog::autosaveOnExit() const { return autosave_->isChecked(); }

void SettingsDialog::setDarkMode(bool on)       { darkMode_->setChecked(on); }
void SettingsDialog::setShowGrid(bool on)       { showGrid_->setChecked(on); }
void SettingsDialog::setAutosaveOnExit(bool on) { autosave_->setChecked(on); }
