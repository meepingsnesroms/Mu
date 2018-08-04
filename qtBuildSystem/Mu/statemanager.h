#pragma once

#include <QDialog>
#include <QListWidgetItem>

namespace Ui{
class StateManager;
}

class StateManager : public QDialog{
   Q_OBJECT

public:
   explicit StateManager(QWidget* parent = nullptr);
   ~StateManager();

   void updateStateList();

private slots:
   bool eventFilter(QObject* object, QEvent* event);

   void updateStatePreview();

   void on_saveState_clicked();
   void on_loadState_clicked();

   void on_states_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
   Ui::StateManager* ui;
};
