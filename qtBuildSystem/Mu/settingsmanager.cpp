#include "settingsmanager.h"
#include "ui_settingsmanager.h"

#include <QString>
#include <QFileDialog>
#include <QKeySequence>
#include <QDir>
#include <QSettings>
#include <QKeyEvent>

#include "mainwindow.h"


SettingsManager::SettingsManager(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::SettingsManager){
   settings = ((MainWindow*)parent)->settings;

   //init GUI
   ui->setupUi(this);

   //set all GUI items to current config values
   ui->homeDirectory->setText(settings->value("resourceDirectory", "").toString());
   ui->showOnscreenKeys->setChecked(!settings->value("hideOnscreenKeys", "").toBool());

   ui->feature128mbRam->setChecked(settings->value("feature128mbRam", false).toBool());
   ui->featureFastCpu->setChecked(settings->value("featureFastCpu", false).toBool());
   ui->featureHybridCpu->setChecked(settings->value("featureHybridCpu", false).toBool());
   ui->featureCustomFb->setChecked(settings->value("featureCustomFb", false).toBool());
   ui->featureSyncedRtc->setChecked(settings->value("featureSyncedRtc", false).toBool());
   ui->featureHleApis->setChecked(settings->value("featureHleApis", false).toBool());
   ui->featureEmuHonest->setChecked(settings->value("featureEmuHonest", false).toBool());
   ui->featureExtraKeys->setChecked(settings->value("featureExtraKeys", false).toBool());
   ui->featureSoundStreams->setChecked(settings->value("featureSoundStreams", false).toBool());

   setKeySelectorState(-1);
   updateButtonKeys();
}

SettingsManager::~SettingsManager(){
   delete ui;
}

void SettingsManager::keyPressEvent(QKeyEvent* event){
   //nothing, all keys are handled at the end of a press in keyReleaseEvent()
}

void SettingsManager::keyReleaseEvent(QKeyEvent* event){
   if(waitingOnKeyForButton != -1){
      settings->setValue("palmButton" + QString::number(waitingOnKeyForButton) + "Key", event->key());

      setKeySelectorState(-1);
      updateButtonKeys();
   }
}

void SettingsManager::setKeySelectorState(int8_t key){
   waitingOnKeyForButton = key;
   if(key != -1)
      ui->keySelectorState->setText("Press a new key!");
   else
      ui->keySelectorState->setText("");
}

void SettingsManager::updateButtonKeys(){
   ui->selectUpKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_UP) + "Key", "").toInt()).toString());
   ui->selectDownKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_DOWN) + "Key", "").toInt()).toString());
   ui->selectLeftKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_LEFT) + "Key", "").toInt()).toString());
   ui->selectRightKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_RIGHT) + "Key", "").toInt()).toString());
   ui->selectCenterKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_CENTER) + "Key", "").toInt()).toString());
   ui->selectCalendarKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_CALENDAR) + "Key", "").toInt()).toString());
   ui->selectAddressBookKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_ADDRESS) + "Key", "").toInt()).toString());
   ui->selectTodoKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_TODO) + "Key", "").toInt()).toString());
   ui->selectNotesKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_NOTES) + "Key", "").toInt()).toString());
   ui->selectPowerKey->setText(QKeySequence(settings->value("palmButton" + QString::number(EmuWrapper::BUTTON_POWER) + "Key", "").toInt()).toString());
}

void SettingsManager::on_showOnscreenKeys_toggled(bool checked){
   settings->setValue("hideOnscreenKeys", !checked);
}

void SettingsManager::on_pickHomeDirectory_clicked(){
   MainWindow* mainWindow = (MainWindow*)parentWidget();
   QString newHomeDir = QFileDialog::getExistingDirectory(this, "Select Home Directory", QDir::root().path(), QFileDialog::ShowDirsOnly);

   if(newHomeDir != ""){
      mainWindow->createHomeDirectoryTree(newHomeDir);
      settings->setValue("resourceDirectory", newHomeDir);
      ui->homeDirectory->setText(newHomeDir);
   }
}

void SettingsManager::on_selectUpKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_UP);
}

void SettingsManager::on_selectDownKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_DOWN);
}

void SettingsManager::on_selectLeftKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_LEFT);
}

void SettingsManager::on_selectRightKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_RIGHT);
}

void SettingsManager::on_selectCenterKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_CENTER);
}

void SettingsManager::on_selectCalendarKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_CALENDAR);
}

void SettingsManager::on_selectAddressBookKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_ADDRESS);
}

void SettingsManager::on_selectTodoKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_TODO);
}

void SettingsManager::on_selectNotesKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_NOTES);
}

void SettingsManager::on_selectPowerKey_clicked(){
   setKeySelectorState(EmuWrapper::BUTTON_POWER);
}

void SettingsManager::on_feature128mbRam_toggled(bool checked){
   settings->setValue("feature128mbRam", checked);
}

void SettingsManager::on_featureFastCpu_toggled(bool checked){
   settings->setValue("featureFastCpu", checked);
}

void SettingsManager::on_featureHybridCpu_toggled(bool checked){
   settings->setValue("featureHybridCpu", checked);
}

void SettingsManager::on_featureCustomFb_toggled(bool checked){
   settings->setValue("featureCustomFb", checked);
}

void SettingsManager::on_featureSyncedRtc_toggled(bool checked){
   settings->setValue("featureSyncedRtc", checked);
}

void SettingsManager::on_featureHleApis_toggled(bool checked){
   settings->setValue("featureHleApis", checked);
}

void SettingsManager::on_featureEmuHonest_toggled(bool checked){
   settings->setValue("featureEmuHonest", checked);
}

void SettingsManager::on_featureExtraKeys_toggled(bool checked){
   settings->setValue("featureExtraKeys", checked);
}

void SettingsManager::on_featureSoundStreams_toggled(bool checked){
   settings->setValue("featureSoundStreams", checked);
}
