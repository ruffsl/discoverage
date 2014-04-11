/* This file is part of the DisCoverage project.

   Copyright (C) Dominik Haumann <dhaumann at rtr.tu-darmstadt.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "mainwindow.h"
#include "scene.h"
#include "config.h"
#include "ui_toolwidget.h"
#include "statistics.h"
#include "robotmanager.h"
#include "robotlistview.h"
#include "tikzexport.h"

#include <QDebug>
#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtCore/QSettings>
#include <QMessageBox>

#include <iostream>

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , Ui::MainWindow()
{
    setupUi(this);

    // instantiate singleton
    new RobotManager(this);

    QWidget* toolsWidget = new QWidget();
    m_toolsUi = new Ui::ToolWidget();
    m_toolsUi->setupUi(toolsWidget);
    m_toolsUi->cmbTool->addItem("Place Vehicle");        // 0
    m_toolsUi->cmbTool->insertSeparator(1);              // 1
    m_toolsUi->cmbTool->addItem("Modify Obstacles");     // 2
    m_toolsUi->cmbTool->addItem("Modify Explored Area"); // 3
    m_toolsUi->cmbTool->insertSeparator(4);              // 4
    m_toolsUi->cmbTool->addItem("DisCoverage (Orientation-based)"); // 5
    m_toolsUi->cmbTool->addItem("MinDist");              // 6
    m_toolsUi->cmbTool->addItem("DisCoverage (Frontier Weights)"); // 7
	m_toolsUi->cmbTool->addItem("Random"); // 8
	m_toolsUi->cmbTool->addItem("MaxArea"); // 9
	m_toolsUi->cmbTool->addItem("Ruffins"); // 10
    toolBar->insertWidget(actionDummy, toolsWidget);
    toolBar->removeAction(actionDummy);

    m_stats = new Statistics(this);
    dwStatistics->setWidget(m_stats);
    dwStatistics->setVisible(false);

    m_statusProgress = new QLabel("Explored: 0.00%", statusBar());
    statusBar()->addPermanentWidget(m_statusProgress);

    m_statusResolution = new QLabel(statusBar());
    statusBar()->addPermanentWidget(m_statusResolution);
    setStatusResolution(0.2);

    m_statusPosition = new QLabel(statusBar());
    statusBar()->addPermanentWidget(m_statusPosition);
    setStatusPosition(QPoint(0, 0));

    m_scene = new Scene(this, this);
    scrollArea->setWidget(m_scene);
    scrollArea->installEventFilter(this);

    m_robotListView = new RobotListView(this);
    dwRobotManager->setWidget(m_robotListView);


    connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(actionZoomIn, SIGNAL(triggered()), m_scene, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), m_scene, SLOT(zoomOut()));
    connect(actionNew, SIGNAL(triggered()), this, SLOT(newScene()));
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(openScene()));
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveScene()));
    connect(actionSaveAs, SIGNAL(triggered()), this, SLOT(saveSceneAs()));
    connect(actionPartition, SIGNAL(triggered(bool)), Config::self(), SLOT(setShowPartition(bool)));
    connect(actionDensity, SIGNAL(triggered(bool)), Config::self(), SLOT(setShowDensity(bool)));
    connect(actionVectorField, SIGNAL(triggered(bool)), Config::self(), SLOT(setShowVectorField(bool)));
    connect(actionPreview, SIGNAL(triggered(bool)), Config::self(), SLOT(setShowPreviewTrajectory(bool)));
    connect(actionStatistics, SIGNAL(triggered(bool)), dwStatistics, SLOT(setVisible(bool)));
    connect(actionExport, SIGNAL(triggered()), this, SLOT(exportToTikz()));
    connect(actionReload, SIGNAL(triggered()), this, SLOT(reloadScene()));
	connect(actionStep, SIGNAL(triggered()), this, SLOT(tick()));
    connect(m_toolsUi->cmbTool, SIGNAL(currentIndexChanged(int)), m_scene, SLOT(selectTool(int)));
    connect(m_toolsUi->sbRadius, SIGNAL(valueChanged(double)), m_scene, SLOT(setOperationRadius(double)));

    connect(actionAbout, SIGNAL(triggered()), this, SLOT(helpAbout()));
    connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(helpAboutQt()));
}

MainWindow::~MainWindow()
{
    delete m_toolsUi;
}

void MainWindow::updateExplorationProgress()
{
    double progress = m_scene->map().explorationProgress() * 100.0;
    m_statusProgress->setText(QString("Explored: %1%").arg(progress, 0, 'f', 2));
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

void MainWindow::openScene()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Scene", QString(), "Scenes (*.scene)");
    if (!fileName.isEmpty()) {
        loadScene(fileName);
    }
}

void MainWindow::loadScene(const QString& filename)
{
    QSettings config(filename, QSettings::IniFormat);
    QSettings::Status status = config.status();
    if (status == QSettings::AccessError) {
        qWarning() << "An access error occurred (e.g. trying to load a non-readable file):" << filename;
        return;
    } else if (status == QSettings::FormatError) {
        qWarning() << "A format error in the INI file occurred:" << filename;
        return;
    }

    const int version = config.value("general/version", -1).toInt();
    if (version != 1) {
        qWarning() << "Unknown version in file, aborting:" << filename;
        return;
    }

    m_scene->load(config);

    config.beginGroup("tool-handler");
    m_toolsUi->sbRadius->setValue(ToolHandler::operationRadius());
    m_toolsUi->cmbTool->setCurrentIndex(config.value("tool", 0).toInt());
    config.endGroup();

    Config::self()->load(config);

    m_sceneFile = filename;

    updateActionState();
}

void MainWindow::reloadScene()
{
    if (!m_sceneFile.isEmpty()) {
        loadScene(m_sceneFile);
    } else {
        m_scene->reset();
    }
    m_stats->reset();
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

    Config::self()->save(config);

    config.beginGroup("tool-handler");
    config.setValue("tool", m_toolsUi->cmbTool->currentIndex());
    config.endGroup();

    m_scene->save(config);
}

void MainWindow::saveSceneAs()
{
    const QString fileName = QFileDialog::getSaveFileName(this, "Save Scene As", QString(), "Scenes (*.scene)");
    if (!fileName.isEmpty()) {
        m_sceneFile = fileName;
        saveScene();
    }
}

QString MainWindow::sceneBaseName() const
{
    QString filename("unnamed");
    if (!m_sceneFile.isEmpty()) {
        filename = m_sceneFile;
        if (filename.endsWith(".scene")) {
            filename = filename.left(filename.size() - 6);
        }
    }
    return filename;
}

void MainWindow::exportToTikz()
{
	std::cout << "EXPORT!!!" << std::endl;
    QString filename = sceneBaseName();
    filename += QString("-iteration-%1").arg(m_stats->iteration(), 3, 10, QChar('0'));
    const QString legend = filename + "-legend.tikz";
    const QString plot3 = filename + "-3dplot.tikz";
    filename += ".tikz";

    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream ts(&file);
        QTikzPicture tikzPicture;
        tikzPicture.setStream(&ts);
        m_scene->exportToTikz(tikzPicture);
    }

    file.close();
    file.setFileName(legend);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream ts(&file);
        QTikzPicture tikzPicture;
        tikzPicture.setStream(&ts);
        m_scene->map().exportLegend(tikzPicture);
    }

    file.close();
    file.setFileName(plot3);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream ts(&file);
        ts.setRealNumberPrecision(2);
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        m_scene->toolHandler()->exportObjectiveFunction(ts);
    }
}

Scene* MainWindow::scene() const
{
    return m_scene;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    scene()->toolHandler()->keyPressEvent(event);

    if (!event->isAccepted()) {
        QMainWindow::keyPressEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // pass wheel events on the scroll area to the scene
    if (event->type() == QEvent::Wheel && 
        QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
         QCoreApplication::sendEvent(m_scene, static_cast<QWheelEvent *>(event));
         return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::tick()
{
    m_scene->tick();
    m_stats->tick();

    // record data if wanted
    if (actionRecord->isChecked()) {
        QString filename = sceneBaseName();
        filename += QString("-iteration-%1").arg(m_stats->iteration(), 3, 10, QChar('0'));
        filename += ".png";
		m_scene->saveImage(filename);
    }
}

void MainWindow::updateActionState()
{
    // we connect to QAction::triggered(), which is not emitted when calling setChecked
    actionPartition->setChecked(Config::self()->showPartition());
    actionDensity->setChecked(Config::self()->showDensity());
    actionVectorField->setChecked(Config::self()->showVectorField());
    actionPreview->setChecked(Config::self()->showPreviewTrajectory());
}

void MainWindow::helpAbout()
{
    QMessageBox::about(this, "About DisCoverage",
                       "<p><b>About DisCoverage</b></p>"
                       "<p>DisCoverage is a framework for exploration strategies. "
                       "It supports multiple robots with several types of vehicle dynamics.</p>"
                       "<p>License: GNU General Public Licence (version 2 or 3)</p>"
                       "<p>(c) 2011-2013 by Dominik Haumann</p>");
}

void MainWindow::helpAboutQt()
{
    qApp->aboutQt();
}

void MainWindow::setStrategy(int strategyIdx)
{
    if (strategyIdx >= 0 && strategyIdx < m_toolsUi->cmbTool->count()) {
        if (strategyIndex() != strategyIdx)
            m_toolsUi->cmbTool->setCurrentIndex(strategyIdx);
    }
}

int MainWindow::strategyIndex() const
{
    return m_toolsUi->cmbTool->currentIndex();
}

// kate: replace-tabs on; indent-width 4;
