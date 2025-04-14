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

    // Create a palette for a dark theme
    QPalette darkPalette;

    // Customize the color palette for the interface elements
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(2, 2, 2));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Dark, QColor(12, 12, 12));
    //
    QFont font("Roboto");
    font.setStyleHint(QFont::Monospace);
    font.setWeight(QFont::Weight(24));
    font.setPointSize(11);

    qApp->setFont(font);

    // Install this palette
    qApp->setPalette(darkPalette);


	QString strPath(getenv("PATH"));
	QString currentPath = QDir::currentPath();
	QString	octavePath  = QFileInfo(currentPath, QString("../share/octave/9.4.0/m")).absolutePath();
#ifdef WIN32
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
