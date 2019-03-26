#include "settingsmanager.h"
#include "ui_settingsmanager.h"


SettingsManager::SettingsManager(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::SettingsManager){
   ui->setupUi(this);
}

SettingsManager::~SettingsManager(){
   delete ui;
}
