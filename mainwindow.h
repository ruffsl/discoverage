#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class QPoint;
class QLabel;
class Scene;

class MainWindow : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT
    public:
        MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
        virtual ~MainWindow();
        
        void setStatusPosition(const QPoint& pos);
        void setStatusResolution(qreal resolution);
        
    public slots:
        void newScene();
        void loadScene();
        void saveScene();
        void exportAsPdf();

    private:
        QLabel* m_statusPosition;
        QLabel* m_statusResolution;
        Scene* m_scene;
        QString m_sceneFile;

};

#endif // MAINWINDOW_H

// kate: replace-tabs on; indent-width 4;
