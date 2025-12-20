#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    bool darkMode() const;
    bool showGrid() const;
    bool autosaveOnExit() const;

    void setDarkMode(bool on);
    void setShowGrid(bool on);
    void setAutosaveOnExit(bool on);

private:
    QCheckBox* darkMode_ = nullptr;
    QCheckBox* showGrid_ = nullptr;
    QCheckBox* autosave_ = nullptr;
    QDialogButtonBox* buttons_ = nullptr;
};
