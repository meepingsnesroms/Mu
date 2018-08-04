#pragma once

#include <QDialog>

namespace Ui{
class StateManager;
}

class StateManager : public QDialog{
   Q_OBJECT

public:
   explicit StateManager(QWidget* parent = nullptr);
   ~StateManager();

private:
   Ui::StateManager* ui;
};
