#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QTouchEvent>
#include <QSettings>
#include <QFont>
#include <QKeyEvent>


QImage video;
QTimer* refreshDisplay;
QApplication* thisapp;
bool emuLock;


MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
	ui->setupUi(this);
	output = this;
	emuLock = false;
	refreshDisplay = new QTimer(this);
	connect(refreshdisplay, SIGNAL(timeout()), this, SLOT(updatedisplay()));
	//update display every 16.67 miliseconds = 60*second
	refreshdisplay->start(16);
	ui->exitemulator->setEnabled(false);
}

MainWindow::~MainWindow(){
	emu_end();
	delete ui;
}

void MainWindow::on_keyboard_pressed(){
}

void MainWindow::on_install_pressed(){
	std::string app = QFileDialog::getOpenFileName(this, "Open Prc/Pdb/Pqa",
	             QDir::root().path(), 0).toStdString();

	if(app != ""){
		//install here
	}
}

uint16_t formattedgfxbuffer[320 * 480];

void MainWindow::updatedisplay(){
	if(!emu_started()){
		return;
	}

	emu_get_framebuffer(formattedgfxbuffer);
	video = QImage((unsigned char*)formattedgfxbuffer, 160, 160, QImage::Format_RGB16);//16 bit
	ui->display->setPixmap(QPixmap::fromImage(video).scaled(
	                           ui->display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui->display->update();
}

void MainWindow::errdisplay(string err){
	ui->display->setText(QString::fromStdString(err));
}

void MainWindow::on_display_destroyed(){
}

void MainWindow::on_mainLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(2));
}

void MainWindow::on_mainRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(1));
}

void MainWindow::on_settingsLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(0));
}

void MainWindow::on_settingsRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(2));
}

void MainWindow::on_joyLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(1));
}

void MainWindow::on_joyRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlpanel->widget(0));
}

void MainWindow::on_settings_clicked(){
	//setprocess(SETUP);
}

void MainWindow::keyPressEvent(QKeyEvent* ev){
	//emu_sendkeyboardchar(ev->key(), true);
}

void MainWindow::keyReleaseEvent(QKeyEvent* ev){
	//emu_sendkeyboardchar(ev->key(), false);
}

void MainWindow::on_power_pressed(){
	emu_sendbutton(BTN_Calender, true);
}

void MainWindow::on_power_released(){
	emu_sendbutton(BTN_Calender, false);
}

void MainWindow::on_calender_pressed(){
	emu_sendbutton(BTN_Calender, true);
}

void MainWindow::on_calender_released(){
	emu_sendbutton(BTN_Calender, false);
}

void MainWindow::on_phone_pressed(){
	emu_sendbutton(BTN_Contacts, true);
}

void MainWindow::on_phone_released(){
	emu_sendbutton(BTN_Contacts, false);
}

void MainWindow::on_todo_pressed(){
	emu_sendbutton(BTN_Todo, true);
}

void MainWindow::on_todo_released(){
	emu_sendbutton(BTN_Todo, false);
}

void MainWindow::on_notes_pressed(){
	emu_sendbutton(BTN_Notes, true);
}

void MainWindow::on_notes_released(){
	emu_sendbutton(BTN_Notes, false);
}
