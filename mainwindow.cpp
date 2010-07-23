#include "mainwindow.h"
#include "scene.h"
#include "ui_toolwidget.h"

#include <QDebug>
#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QPrinter>

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , Ui::MainWindow()
{
    setupUi(this);

    QWidget* toolsWidget = new QWidget();
    Ui::ToolWidget tools;
    tools.setupUi(toolsWidget);
    toolBar->insertWidget(actionDummy, toolsWidget);
    toolBar->removeAction(actionDummy);

    m_statusResolution = new QLabel(statusBar());
    statusBar()->addPermanentWidget(m_statusResolution);
    setStatusResolution(0.2);

    m_statusPosition = new QLabel(statusBar());
    statusBar()->addPermanentWidget(m_statusPosition);
    setStatusPosition(QPoint(0, 0));

    m_scene = new Scene(this, this);
    scrollArea->setWidget(m_scene);

    connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(actionZoomIn, SIGNAL(triggered()), m_scene, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), m_scene, SLOT(zoomOut()));
    connect(actionNew, SIGNAL(triggered()), this, SLOT(newScene()));
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(loadScene()));
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveScene()));
    connect(actionExport, SIGNAL(triggered()), this, SLOT(exportAsPdf()));
    connect(tools.cmbTool, SIGNAL(currentIndexChanged(int)), m_scene, SLOT(selectTool(int)));
    connect(tools.sbRadius, SIGNAL(valueChanged(double)), m_scene, SLOT(setOperationRadius(double)));
}

MainWindow::~MainWindow()
{
}

void MainWindow::setStatusPosition(const QPoint& pos)
{
    m_statusPosition->setText(QString("Cell: %1 x %2").arg(pos.x()).arg(pos.y()));
}

void MainWindow::setStatusResolution(qreal resolution)
{
    m_statusResolution->setText(QString("Cell Resolution: %1m x %2m").arg(resolution).arg(resolution));
}

void MainWindow::newScene()
{
    m_scene->newScene();
    m_sceneFile.clear();
}

void MainWindow::loadScene()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Scene", QString(), "Scenes (*.scene)");
    if (fileName.isEmpty()) {
        return;
    }

    QSettings config(fileName, QSettings::IniFormat);
    QSettings::Status status = config.status();
    if (status == QSettings::AccessError) {
        qWarning() << "An access error occurred (e.g. trying to load a non-readable file):" << fileName;
        return;
    } else if (status == QSettings::FormatError) {
        qWarning() << "A format error in the INI file occurred:" << fileName;
        return;
    }

    const int version = config.value("general/version", -1).toInt();
    if (version != 1) {
        qWarning() << "Unknown version in file, aborting:" << fileName;
        return;
    }

    m_scene->load(config);
    m_sceneFile = fileName;
}

void MainWindow::saveScene()
{
    if (m_sceneFile.isEmpty()) {
        const QString fileName = QFileDialog::getSaveFileName(this, "Save Scene", QString(), "Scenes (*.scene)");
        if (fileName.isEmpty()) {
            return;
        }
        m_sceneFile = fileName;
    }
    if (!m_sceneFile.endsWith(".scene")) {
        m_sceneFile.append(".scene");
    }

    QSettings config(m_sceneFile, QSettings::IniFormat);
    QSettings::Status status = config.status();
    if (status == QSettings::AccessError) {
        qWarning() << "An access error occurred (e.g. trying to write to a read-only file):" << m_sceneFile;
        return;
    } else if (status == QSettings::FormatError) {
        qWarning() << "A format error in the INI file occurred:" << m_sceneFile;
        return;
    }

    config.setValue("general/version", 1);
    m_scene->save(config);
}

void MainWindow::exportAsPdf()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFileName("print.pdf");
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(m_scene->size()/3, QPrinter::Millimeter);
//     printer.setOrientation(QPrinter::Landscape);
//     printer.setFullPage(true);

    m_scene->draw(&printer);
}

// kate: replace-tabs on; indent-width 4;
