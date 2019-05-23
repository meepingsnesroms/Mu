#include "statemanager.h"
#include "ui_statemanager.h"

#include <QPixmap>
#include <QString>
#include <QObject>
#include <QEvent>
#include <QFile>
#include <QDir>
#include <QListWidgetItem>

#include "mainwindow.h"


StateManager::StateManager(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::StateManager){
   emu = &((MainWindow*)parent)->emu;

   //init GUI
   ui->setupUi(this);

   //this allows resizing the screenshot of the savestate
   ui->statePreview->installEventFilter(this);
   ui->statePreview->setObjectName("statePreview");

   updateStateList();
}

StateManager::~StateManager(){
   delete ui;
}

bool StateManager::eventFilter(QObject* object, QEvent* event){
   if(object->objectName() == "statePreview" && event->type() == QEvent::Resize)
      updateStatePreview();

   return QDialog::eventFilter(object, event);
}

void StateManager::updateStateList(){
   QDir saveDir(emu->getStatePath());
   QStringList saveStateNames;
   QStringList filter;

   //query a list of all .state files in directory
   filter << "*.state";
   saveStateNames = saveDir.entryList(filter, QDir::Files, QDir::Name);

   //list all file names
   ui->states->clear();
   for(int stateIndex = 0; stateIndex < saveStateNames.length(); stateIndex++){
      //remove .state from name
      saveStateNames[stateIndex].remove(saveStateNames[stateIndex].length() - 6, 6);

      ui->states->addItem(saveStateNames[stateIndex]);
   }
}

void StateManager::updateStatePreview(){
   if(ui->states->currentItem()){
      QString statePath = emu->getStatePath() + "/" + ui->states->currentItem()->text();

      ui->statePreview->setPixmap(QPixmap(statePath + ".png").scaled(ui->statePreview->width() * 0.98, ui->statePreview->height() * 0.98, Qt::KeepAspectRatio, Qt::SmoothTransformation));
   }
   else{
      //remove outdated image
      ui->statePreview->clear();
   }

   ui->statePreview->update();
}

int StateManager::getStateIndexRowByName(const QString& name){
   for(int index = 0; index < ui->states->count(); index++)
      if(ui->states->item(index)->text() == name)
         return index;

   return -1;//nothing by that name exists
}

void StateManager::on_saveState_clicked(){
   if(ui->newStateName->text() != ""){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = emu->getStatePath() + "/" + ui->newStateName->text();

      parent->emu.saveState(ui->newStateName->text());
      parent->emu.getFramebuffer().save(statePath + ".png");
      updateStateList();

      ui->states->setCurrentRow(getStateIndexRowByName(ui->newStateName->text()));//this also updates the preview image
   }
}

void StateManager::on_loadState_clicked(){
   if(ui->states->currentItem()){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = emu->getStatePath() + "/" + ui->states->currentItem()->text();

      parent->emu.loadState(ui->states->currentItem()->text());
   }
}

void StateManager::on_deleteState_clicked(){
   if(ui->states->currentItem()){
      QString statePath = emu->getStatePath() + "/" + ui->states->currentItem()->text();

      QFile(statePath + ".state").remove();
      QFile(statePath + ".png").remove();
      updateStateList();

      ui->states->setCurrentRow(0);//pick first valid entry since the old one is no longer valid
   }
}

void StateManager::on_states_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous){
   updateStatePreview();
   ui->states->repaint();
   ui->statePreview->repaint();
}
