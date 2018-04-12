#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QTouchEvent>
#include <QSettings>
#include <QFont>
#include <QKeyEvent>

#include <new>
#include <atomic>
#include <cstdint>
#include <unistd.h>
#include <sys/stat.h>

#include "src/emulator.h"

uint32_t screenWidth;
uint32_t screenHeight;

static QImage video;
static QTimer* refreshDisplay;
static QSettings settings;
static std::atomic<bool> emuOn;
static std::atomic<bool> emuLock;
static bool emuInited;


void popupErrorDialog(std::string error){
	//add error dialog code
}

uint8_t* getFileBuffer(std::string filePath, size_t& size, uint32_t& error){
	uint8_t* rawData = NULL;
	if(filePath != ""){
		struct stat st;
		int doesntExist = stat(filePath.c_str(), &st);
		if(doesntExist == 0 && st.st_size){
			FILE* dataFile = fopen(filePath.c_str(), "rb");
			if(dataFile != NULL){
				rawData = new (std::nothrow) uint8_t[st.st_size];
				if(rawData){
					size_t bytesRead = fread(rawData, 1, st.st_size, dataFile);
					if(bytesRead == st.st_size){
						size = bytesRead;
						error = FRONTEND_ERR_NONE;
					}
					else{
						error = FRONTEND_FILE_UNFINISHED;
					}
				}
				else{
					error = FRONTEND_OUT_OF_MEMORY;
				}
				fclose(dataFile);
			}
			else{
				error = FRONTEND_FILE_PROTECTED;
			}
		}
		else{
			error = FRONTEND_FILE_DOESNT_EXIST;
		}
	}
	else{
		error = FRONTEND_FILE_EMPTY_PATH;
	}

	if(error != FRONTEND_ERR_NONE){
		if(rawData)
			delete[] rawData;
		rawData = NULL;
		size = 0;
	}

	return rawData;
}


MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
	ui->setupUi(this);
	emuOn = false;
	emuLock = false;
	emuInited = false;
	screenWidth = 160;
	screenHeight = 160;

	loadRom();

	refreshDisplay = new QTimer(this);
	connect(refreshDisplay, SIGNAL(timeout()), this, SLOT(updateDisplay()));
	//update display every 16.67 miliseconds = 60*second
	refreshDisplay->start(16);
}

MainWindow::~MainWindow(){
	while(emuLock)usleep(10);
	emulatorExit();
	delete ui;
}


void MainWindow::loadRom(){
	std::string rom = settings.value("romPath", "").toString().toStdString();
	if(rom == ""){
		//keep the selector open until a valid file is picked
		while(settings.value("romPath", "").toString().toStdString() == "")
			selectRom();
	}

	uint32_t error;
	size_t size;
	uint8_t* romData = getFileBuffer(rom, size, error);
	if(romData){
		size_t romSize = size < ROM_SIZE ? size : ROM_SIZE;
		memcpy(palmRom, romData, romSize);
		delete[] romData;
	}

	if(error != FRONTEND_ERR_NONE){
		popupErrorDialog("Could not open ROM file");
	}
}

void MainWindow::selectRom(){
	std::string rom = QFileDialog::getOpenFileName(this, "Palm OS ROM (palmos41-en-m515.rom)", QDir::root().path(), 0).toStdString();
	uint32_t error;
	size_t size;
	uint8_t* romData = getFileBuffer(rom, size, error);
	if(romData){
		//valid file
		settings.setValue("romPath", QString::fromStdString(rom));
		delete[] romData;
	}


	if(error != FRONTEND_ERR_NONE){
		popupErrorDialog("Could not open ROM file");
	}
}

void MainWindow::on_install_pressed(){
	std::string app = QFileDialog::getOpenFileName(this, "Open Prc/Pdb/Pqa", QDir::root().path(), 0).toStdString();
	uint32_t error;
	size_t size;
	uint8_t* appData = getFileBuffer(app, size, error);
	if(appData){
		error = emulatorInstallPrcPdb(appData, size);
		delete[] appData;
	}

	if(error != FRONTEND_ERR_NONE){
		popupErrorDialog("Could not install app");
	}
}

//display
void MainWindow::updateDisplay(){
	if(emuOn && !emuLock){
		emuLock = true;
		emulateFrame();
		video = QImage((unsigned char*)palmFramebuffer, screenWidth, screenHeight, QImage::Format_RGB16);//16 bit
		ui->display->setPixmap(QPixmap::fromImage(video).scaled(ui->display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
		ui->display->update();
		emuLock = false;
	}
}

void MainWindow::on_display_destroyed(){
	//do nothing
}

//palm buttons
void MainWindow::on_power_pressed(){
	palmInput.buttonPower = true;
}

void MainWindow::on_power_released(){
	palmInput.buttonPower = false;
}

void MainWindow::on_calender_pressed(){
	palmInput.buttonCalender = true;
}

void MainWindow::on_calender_released(){
	palmInput.buttonCalender = false;
}

void MainWindow::on_addressBook_pressed(){
	palmInput.buttonAddress = true;
}

void MainWindow::on_addressBook_released(){
	palmInput.buttonAddress = false;
}

void MainWindow::on_todo_pressed(){
	palmInput.buttonTodo = true;
}

void MainWindow::on_todo_released(){
	palmInput.buttonTodo = false;
}

void MainWindow::on_notes_pressed(){
	palmInput.buttonNotes = true;
}

void MainWindow::on_notes_released(){
	palmInput.buttonNotes = false;
}

//ui buttons
void MainWindow::on_mainLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(2));
}

void MainWindow::on_mainRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(1));
}

void MainWindow::on_settingsLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(0));
}

void MainWindow::on_settingsRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(2));
}

void MainWindow::on_joyLeft_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(1));
}

void MainWindow::on_joyRight_clicked(){
	ui->controlPanel->setCurrentWidget(ui->controlPanel->widget(0));
}

void MainWindow::on_settings_clicked(){
	//setprocess(SETUP);
}
