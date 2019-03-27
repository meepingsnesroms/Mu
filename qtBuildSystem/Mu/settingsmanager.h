#pragma once

#include <QDialog>
#include <QString>
#include <QKeyEvent>
#include <QSettings>

#include "emuwrapper.h"

namespace Ui {
class SettingsManager;
}

class SettingsManager : public QDialog
{
   Q_OBJECT

public:
   explicit SettingsManager(QWidget* parent = nullptr);
   ~SettingsManager();

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private:
   void setKeySelectorState(int8_t key);
   void updateButtonKeys();

private slots:
   void on_showOnscreenKeys_toggled(bool checked);
   void on_pickHomeDirectory_clicked();

   void on_selectUpKey_clicked();
   void on_selectDownKey_clicked();
   void on_selectLeftKey_clicked();
   void on_selectRightKey_clicked();
   void on_selectCenterKey_clicked();
   void on_selectCalendarKey_clicked();
   void on_selectAddressBookKey_clicked();
   void on_selectTodoKey_clicked();
   void on_selectNotesKey_clicked();
   void on_selectPowerKey_clicked();

   void on_feature128mbRam_toggled(bool checked);
   void on_featureFastCpu_toggled(bool checked);
   void on_featureHybridCpu_toggled(bool checked);
   void on_featureCustomFb_toggled(bool checked);
   void on_featureSyncedRtc_toggled(bool checked);
   void on_featureHleApis_toggled(bool checked);
   void on_featureEmuHonest_toggled(bool checked);
   void on_featureExtraKeys_toggled(bool checked);
   void on_featureSoundStreams_toggled(bool checked);

private:
   Ui::SettingsManager* ui;
   QSettings*           settings;
   int8_t               waitingOnKeyForButton;
};
