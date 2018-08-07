#include "statemanager.h"
#include "ui_statemanager.h"

#include <QPixmap>
#include <QString>
#include <QObject>
#include <QEvent>
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
   QString saveDirPath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates";
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
      QString statePath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->states->currentItem()->text();
      QPixmap previewPicture;

      previewPicture.load(statePath + ".png");
      ui->statePreview->setPixmap(previewPicture.scaled(ui->statePreview->width() * 0.98, ui->statePreview->height() * 0.98, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      ui->statePreview->update();
   }
}

void StateManager::on_saveState_clicked(){
   if(ui->newStateName->text() != ""){
      QString statePath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->newStateName->text();

      ((MainWindow*)parentWidget())->emu.saveState(statePath + ".state");
      ((MainWindow*)parentWidget())->emu.getFramebuffer().save(statePath + ".png");
      updateStateList();
   }
}

void StateManager::on_loadState_clicked(){
   if(ui->states->currentItem()){
      QString statePath = ((MainWindow*)parentWidget())->settings.value("resourceDirectory", "").toString() + "/saveStates/" + ui->states->currentItem()->text();

      ((MainWindow*)parentWidget())->emu.loadState(statePath + ".state");
   }
}

void StateManager::on_states_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous){
   updateStatePreview();
}
