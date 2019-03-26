#pragma once

#include <QDialog>

namespace Ui {
class SettingsManager;
}

class SettingsManager : public QDialog
{
   Q_OBJECT

public:
   explicit SettingsManager(QWidget* parent = nullptr);
   ~SettingsManager();

private:
   Ui::SettingsManager* ui;
};
