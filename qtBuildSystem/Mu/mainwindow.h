#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include "palmwrapper.h"

extern emu_config settings;

extern QApplication* thisapp;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	void errdisplay(std::string err);

protected:
    void keyPressEvent(QKeyEvent* ev);
	void keyReleaseEvent(QKeyEvent* ev);

private slots:
    void updatedisplay();

    void on_display_destroyed();

    void on_keyboard_pressed();

    void on_install_pressed();

    void on_mainleft_clicked();

    void on_mainright_clicked();

    void on_settingsleft_clicked();

    void on_settingsright_clicked();

    void on_joyleft_clicked();

    void on_joyright_clicked();

    void on_settings_clicked();

    void on_clock_pressed();

    void on_clock_released();

    void on_controlemulator_clicked();

	void on_exitemulator_pressed();

	void on_exitemulator_released();

	void on_phone_pressed();

	void on_phone_released();

	void on_todo_pressed();

	void on_todo_released();

	void on_notes_pressed();

	void on_notes_released();

	void on_runtest_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
