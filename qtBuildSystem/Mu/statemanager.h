#pragma once

#include <QDialog>
#include <QObject>
#include <QEvent>
#include <QListWidgetItem>
#include <QSettings>

namespace Ui{
class StateManager;
}

class StateManager : public QDialog{
   Q_OBJECT

public:
   explicit StateManager(QWidget* parent = nullptr);
   ~StateManager();

private slots:
   bool eventFilter(QObject* object, QEvent* event);

   void updateStateList();
   void updateStatePreview();
   int getStateIndexRowByName(const QString& name);

   void on_saveState_clicked();
   void on_loadState_clicked();
   void on_deleteState_clicked();

   void on_states_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
   Ui::StateManager* ui;
   QSettings*        settings;
};
