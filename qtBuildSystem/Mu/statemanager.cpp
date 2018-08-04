#include "statemanager.h"
#include "ui_statemanager.h"

#include <QPixmap>
#include <QString>
#include <QDir>
#include <QListWidgetItem>

#include "mainwindow.h"


StateManager::StateManager(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::StateManager){
   ui->setupUi(this);
}

StateManager::~StateManager(){
   delete ui;
}

void StateManager::updateStateList(){
   QString saveDirPath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates";
   QDir saveDir(saveDirPath);
   QStringList saveStateNames;
   QStringList filter;

   //query a list of all .state files in directory
   filter << "*.state";
   saveStateNames = saveDir.entryList(filter, QDir::Files, QDir::Name);

   //list all file names
   ui->states->clear();
   for(int stateIndex = 0; stateIndex < saveStateNames.length(); stateIndex++)
      ui->states->addItem(saveStateNames[stateIndex]);
}

void StateManager::on_saveState_clicked(){
   QString statePath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ;
   ((MainWindow*)parentWidget())->emu.saveState();
   updateStateList();
}

void StateManager::on_loadState_clicked(){
   updateStateList();
}

void StateManager::on_states_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous){
    //set the new state preview picture
   QString savePath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates/" + current->text();
   QPixmap previewPicture;

   previewPicture.load(savePath);
   ui->statePreview->setPixmap(previewPicture);
}
