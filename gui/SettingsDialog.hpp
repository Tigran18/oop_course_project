#pragma once

#include <QDialog>

class QCheckBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setDarkMode(bool on);
    void setShowGrid(bool on);
    void setSnapToGrid(bool on);
    void setAutosaveOnExit(bool on);

    bool darkMode() const;
    bool showGrid() const;
    bool snapToGrid() const;
    bool autosaveOnExit() const;

private:
    QCheckBox* dark_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* snap_ = nullptr;
    QCheckBox* autosave_ = nullptr;
};
