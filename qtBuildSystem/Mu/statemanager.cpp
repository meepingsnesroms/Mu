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
   ui->setupUi(this);

   //this allows resizing the screenshot of the savestate
   ui->statePreview->installEventFilter(this);
   ui->statePreview->setObjectName("statePreview");
}

StateManager::~StateManager(){
   delete ui;
}

void StateManager::updateStateList(){
   MainWindow* parent = (MainWindow*)parentWidget();
   QString saveDirPath = parent->settings.value("resourceDirectory", "").toString() + "/saveStates";
   QDir saveDir(saveDirPath);
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

bool StateManager::eventFilter(QObject* object, QEvent* event){
   if(object->objectName() == "statePreview" && event->type() == QEvent::Resize)
      updateStatePreview();

   return QDialog::eventFilter(object, event);
}

void StateManager::updateStatePreview(){
   if(ui->states->currentItem()){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = parent->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->states->currentItem()->text();

      ui->statePreview->setPixmap(QPixmap(statePath + ".png").scaled(ui->statePreview->width() * 0.98, ui->statePreview->height() * 0.98, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      ui->statePreview->update();
   }
}

void StateManager::on_saveState_clicked(){
   if(ui->newStateName->text() != ""){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = parent->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->newStateName->text();

      parent->emu.saveState(statePath + ".state");
      parent->emu.getFramebuffer().save(statePath + ".png");
      updateStateList();
   }
}

void StateManager::on_loadState_clicked(){
   if(ui->states->currentItem()){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = parent->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->states->currentItem()->text();

      parent->emu.loadState(statePath + ".state");
   }
}

void StateManager::on_deleteState_clicked(){
   if(ui->states->currentItem()){
      MainWindow* parent = (MainWindow*)parentWidget();
      QString statePath = parent->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->states->currentItem()->text();

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
