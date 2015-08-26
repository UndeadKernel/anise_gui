#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMouseEvent>
#include <QMainWindow>
#include <QTableWidgetItem>
#include <QAbstractButton>
#include "jsonfilehandler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
friend class Data;
public:
    explicit MainWindow(QWidget *parent = 0);
    // Ui::MainWindow *getUi();
    ~MainWindow();
    Ui::MainWindow *ui;
     void deleteTable();
    void updatePropertyTable(int nodeID);
    void displayTypeInfo(const QString &type);
    static int const EXIT_CODE_REBOOT;

public slots:

private slots:

    // Action for loading .mesh files
    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void on_actionSort_Mesh_triggered();
    void on_actionSet_framework_path_triggered();
    void on_actionNew_triggered();
    void on_actionLoad_Catalog_triggered();
    void on_details_stateChanged(int arg1);
    void onFilebuttonClicked(int row, int col);
    void on_actionSave_as_triggered();

    void on_start_button_clicked();

    void on_stop_button_clicked();

    void on_actionStop_Simulation_triggered();

    void on_actionAbout_triggered();

    void on_actionRun_Mesh_triggered(QString line);

private:
    void initializeGUI();
    bool pathChanged = false;
    QString saveDialog();
    void saveFile(QString &path);
    int oldfocus = -1;
protected:
    void closeEvent(QCloseEvent *event);
};
#endif
