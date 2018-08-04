#include "statemanager.h"
#include "ui_statemanager.h"

StateManager::StateManager(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::StateManager){
   ui->setupUi(this);
}

StateManager::~StateManager(){
   delete ui;
}
