#include "gui/SettingsDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");

    auto* root = new QVBoxLayout(this);

    dark_ = new QCheckBox("Dark mode");
    grid_ = new QCheckBox("Show grid");
    snap_ = new QCheckBox("Snap to grid");
    autosave_ = new QCheckBox("Autosave on exit");

    root->addWidget(dark_);
    root->addWidget(grid_);
    root->addWidget(snap_);
    root->addWidget(autosave_);

    auto* buttons = new QHBoxLayout;
    auto* ok = new QPushButton("OK");
    auto* cancel = new QPushButton("Cancel");
    buttons->addStretch(1);
    buttons->addWidget(ok);
    buttons->addWidget(cancel);

    root->addLayout(buttons);

    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::setDarkMode(bool on) { dark_->setChecked(on); }
void SettingsDialog::setShowGrid(bool on) { grid_->setChecked(on); }
void SettingsDialog::setSnapToGrid(bool on) { snap_->setChecked(on); }
void SettingsDialog::setAutosaveOnExit(bool on) { autosave_->setChecked(on); }

bool SettingsDialog::darkMode() const { return dark_->isChecked(); }
bool SettingsDialog::showGrid() const { return grid_->isChecked(); }
bool SettingsDialog::snapToGrid() const { return snap_->isChecked(); }
bool SettingsDialog::autosaveOnExit() const { return autosave_->isChecked(); }
