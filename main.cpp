/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>
#include <QIcon>
#include <QMessageBox>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "ARIA_RDK_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			a.installTranslator(&translator);
			break;
		}
	}

	// set style
	qApp->setStyle(QStyleFactory::create("Fusion"));
	// increase font size for better reading
	QFont defaultFont = QApplication::font();
	//defaultFont.setPointSize(defaultFont.pointSize());
	qApp->setFont(defaultFont);
	// modify palette to dark
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window,QColor(53,53,53));
	darkPalette.setColor(QPalette::WindowText,Qt::white);
	darkPalette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
	darkPalette.setColor(QPalette::Base,QColor(42,42,42));
	darkPalette.setColor(QPalette::AlternateBase,QColor(66,66,66));
	darkPalette.setColor(QPalette::ToolTipBase,Qt::white);
	darkPalette.setColor(QPalette::ToolTipText,Qt::white);
	darkPalette.setColor(QPalette::Text,Qt::white);
	darkPalette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
	darkPalette.setColor(QPalette::Dark,QColor(35,35,35));
	darkPalette.setColor(QPalette::Shadow,QColor(20,20,20));
	darkPalette.setColor(QPalette::Button,QColor(53,53,53));
	darkPalette.setColor(QPalette::ButtonText,Qt::white);
	darkPalette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
	darkPalette.setColor(QPalette::BrightText,Qt::red);
	darkPalette.setColor(QPalette::Link,QColor(42,130,218));
	darkPalette.setColor(QPalette::Highlight,QColor(42,130,218));
	darkPalette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
	darkPalette.setColor(QPalette::HighlightedText,Qt::white);
	darkPalette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));

	qApp->setPalette(darkPalette);

#ifdef WIN32
	QString strPath(getenv("PATH"));
	QString currentPath = QDir::currentPath();
	QString	octavePath  = QFileInfo(currentPath, QString("../share/octave/9.4.0/m")).absolutePath();

	QStringList all_dirs;
	all_dirs << octavePath;
	QDirIterator directories(octavePath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	strPath.append(QString(";")+octavePath);
	while(directories.hasNext()){
		directories.next();
		all_dirs << directories.filePath();
		strPath.append(QString(";")+directories.filePath());
	}
	_putenv_s("PATH",strPath.toStdString().c_str());

#endif




	MainWindow w;

	w.showMaximized();
	// Setup QMessageCatch
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	qInstallMessageHandler(MainWindow::QMessageOutput);
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	QIcon icon(":/icons/ARIA_Logo.ico");

	a.setWindowIcon(icon);



	return a.exec();
}
