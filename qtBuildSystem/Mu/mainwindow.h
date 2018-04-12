#pragma once

#include <QMainWindow>
#include <string>
#include <stdint.h>

extern uint32_t screenWidth;
extern uint32_t screenHeight;

enum{
	FRONTEND_ERR_NONE,
	FRONTEND_FILE_DOESNT_EXIST,
	FRONTEND_FILE_EMPTY_PATH,
	FRONTEND_FILE_UNFINISHED,
	FRONTEND_FILE_MODIFYED,
	FRONTEND_FILE_INVALID_TYPE,
	FRONTEND_FILE_PROTECTED,
	FRONTEND_OUT_OF_MEMORY
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
	void loadRom();
	void selectRom();
    void on_install_pressed();

	//display
	void updateDisplay();
	void on_display_destroyed();

	//palm buttons
	void on_power_pressed();
	void on_power_released();
	void on_calender_pressed();
	void on_calender_released();
	void on_addressBook_pressed();
	void on_addressBook_released();
	void on_todo_pressed();
	void on_todo_released();
	void on_notes_pressed();
	void on_notes_released();

	//ui buttons
	void on_mainLeft_clicked();
	void on_mainRight_clicked();
	void on_settingsLeft_clicked();
	void on_settingsRight_clicked();
	void on_joyLeft_clicked();
	void on_joyRight_clicked();
	void on_settings_clicked();

private:
    Ui::MainWindow *ui;
};
