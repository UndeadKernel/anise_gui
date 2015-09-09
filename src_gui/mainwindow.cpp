#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>
#include <QVector>
#include <QCursor>
#include <QShortcut>
#include <QSpinBox>
#include <limits>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "anisecommunicator.h"
#include "settingshandler.h"
#include "qdebugstreamredirector.h"
#include "render/singletonrender.h"
#include "nodecatalog.h"
#include "data.h"
#include <QJsonObject>

int const MainWindow::EXIT_CODE_REBOOT = -123456789;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    // Important to do first! SingletonRender is used in initialize GUI
    SingletonRender::instance()->setUi(this->ui);
    initializeGUI();

    // activates deleteItem when deletebutton is pressed
    // deleteItem handles itself whether object is focussed or not nothing is
    // deleted
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(shortcut, SIGNAL(activated()), Data::instance(), SLOT(deleteItem()));
    connect(ui->delete_button, SIGNAL(clicked()), Data::instance(),
            SLOT(deleteItem()));
    connect(ui->actionDelete, SIGNAL(triggered()), Data::instance(),
            SLOT(deleteItem()));
}

void MainWindow::initializeGUI() {
    // general initialization of the mainwindow
    ui->setupUi(this);                   // generate gui from xml
    menuBar()->setNativeMenuBar(false);  // schaltet natives ubuntu menu aus
    // new Q_DebugStream(std::cout, ui->qDebug_out); // leitet qDebug ins
    // logfenster
    // Q_DebugStream::registerQDebugMessageHandler();

    // make the mesh editor accept drops
    ui->mesh_edt_area->setAcceptDrops(true);

    // setup viewports
    ui->Node_Catalog->setupViewport(ui->nodeCatalogContent);
    ui->mesh_edt_area->setupViewport(ui->meshField);

    // show the tab-widget
    ui->tabWidget->show();

    ui->start_button->hide();
    ui->stop_button->hide();
    ui->delete_button->hide();

    // prepare propertytable
    ui->tableWidget->hide();
    ui->tableWidget->setColumnCount(2);
    // initialize all data content
    Data::instance()->initialize(this);
    //if gui is rebooting
    if(Data::instance()->getLastExitCode() == EXIT_CODE_REBOOT){
        //load tmp file (last mesh)
        QJsonObject *obj =
        JsonFileHandler::readFile("/tmp/tmp_anise_gui.mesh");
        JsonFileHandler::extractNodesAndConnections(*obj);

        //unset focus
        Data::instance()->setFocusMeshObject(-1);

        //load last save file
        QFile file("/tmp/tmp_anise_gui_savefile");
        file.open(QFile::ReadOnly);
        Data::instance()->setSaveFile(file.readAll());
        file.close();

        //unset changed flag
        Data::instance()->unsetChanged();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    bool accept_exit = Data::instance()->hasChanged();
    // has the mesh changed?
    if (accept_exit) {
        // ask to save or not
        int choice = QMessageBox::question(
                    this, "Save old project?",
                    "Do you want to save befor exiting?",
                    (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
                    QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            QString path = Data::instance()->getSaveFile();
            if(path == "") path = saveDialog();
            // check if a file was chosen
            accept_exit = path != ".mesh";
            // save to file
            if (accept_exit) saveFile(path);
        } else if (choice == QMessageBox::No)
            // if user does not want to save, exit is ok
            accept_exit = true;
    } else accept_exit = true;
    // if exit action is still ok, accept it.
    if (accept_exit)
        event->accept();
    else
        event->ignore();
}

void MainWindow::saveFile(QString &path) {
    JsonFileHandler::saveMesh(path);
    Data::instance()->unsetChanged();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_actionLoad_triggered() {
    // clear old mesh
    this->on_actionNew_triggered();

    // get filename
    QString fileName = QFileDialog::getOpenFileName(
                this, "Load previously saved mesh", "",
                "Mesh-Files (*.mesh *.json);; All Files (*.*)");

    if (fileName == "") return;
    // get file content as JsonObject
    qDebug() << "Trying to load \"" + fileName + "\"";
    QJsonObject *obj = JsonFileHandler::readFile(fileName);
    // parse file
    bool success = JsonFileHandler::extractNodesAndConnections(*obj);
    if(success)
        Data::instance()->setSaveFile(fileName);
    else
        Data::instance()->setSaveFile("");
    Data::instance()->unsetChanged();
}

void MainWindow::on_actionSet_framework_path_triggered() {
    QString old = SettingsHandler::getSetting("frameworkpath");
    QString fileName =
            QFileDialog::getOpenFileName(this, "Set your framework path", "", "");

    SettingsHandler::storeSetting("frameworkpath", fileName);
    AniseCommunicator::setFrameworkPath(fileName);
    if (old != fileName) {
        pathChanged = true;
        int choice = QMessageBox::warning(
                    this, "Path to Framework changed",
                    "You changed the Path to the ANISE-Framework. To apply the changes a "
                    "restart is required.\nDo you want to restart now?",
                    QMessageBox::Yes, QMessageBox::Cancel);
        if (choice == QMessageBox::Yes) emit ui->actionLoad_Catalog->triggered();
    }
}

void MainWindow::on_actionNew_triggered() {
    bool accept_exit = Data::instance()->hasChanged();
    // has the mesh changed?
    if (accept_exit) {
        // ask to save or not
        int choice = QMessageBox::question(
                    this, "Save old project?",
                    "You attemted creating a new project.\n Do you want to save your "
                    "current project?",
                    (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
                    QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            QString path = Data::instance()->getSaveFile();
            if(path == "") path = saveDialog();
            // check if a file was chosen
            accept_exit = path != ".mesh";
            // save to file
            if (accept_exit) saveFile(path);
        } else if (choice == QMessageBox::No)
            // if user does not want to save, exit is ok
            accept_exit = true;
    } else accept_exit = true;
    if (accept_exit) Data::instance()->newMeshProject();
}

void MainWindow::on_actionSort_Mesh_triggered() {
    Data::instance()->sortForce();
    SingletonRender::instance()->renderMesh();
}

void MainWindow::on_actionLoad_Catalog_triggered() {
    // the old ndoes should be deleted!
    JsonFileHandler::saveMesh("/tmp/tmp_anise_gui.mesh");
    QFile file("/tmp/tmp_anise_gui_savefile");
    file.open(QFile::WriteOnly);
    file.write(Data::instance()->getSaveFile().toUtf8());
    file.close();
    QString out = AniseCommunicator::getAllNodeTypes();
    // Data::instance()->getNodeCatalog()->Content.clear();
    JsonFileHandler::parseNodeTypesFromAnise(out);
    Data::instance()->newMeshProject();
    // SingletonRender::instance()->renderCatalogContent(Data::instance()->getNodeCatalog()->Content.values().toVector());

    qDebug() << "perfoming reboot...";
    Data::instance()->setLastExitCode(EXIT_CODE_REBOOT);
    qApp->exit(EXIT_CODE_REBOOT);
}

QString MainWindow::saveDialog() {
    QString fileName =
            QFileDialog::getSaveFileName(this, "Save current project to...", "",
                                         "Mesh-Files (*.mesh *.json);;All Files(*)");

    if (!(fileName.endsWith(".json", Qt::CaseInsensitive) ||
          fileName.endsWith(".mesh", Qt::CaseInsensitive)))
        fileName += ".mesh";
    return fileName;
}

void MainWindow::on_actionSave_as_triggered() {
    QString fileName = saveDialog();
    saveFile(fileName);
}

void MainWindow::on_actionSave_triggered() {
    QString fileName = Data::instance()->getSaveFile();
    if (fileName == "") {
        fileName = saveDialog();
        if (fileName != "")
            Data::instance()->setSaveFile(fileName);
        else
            return;
    }
    saveFile(fileName);
}

void MainWindow::updatePropertyTable(int nodeID) {
    if(nodeID == -2)
        nodeID = Data::instance()->getFocusedID();
    QTableWidget *table = ui->tableWidget;
    if (nodeID >= 0 && Data::instance()->nodesInMesh()->contains(nodeID)) {
        deleteTable();
        oldfocus = nodeID;
        Node *n = Data::instance()->getNodeByID(nodeID);
        QMap<QString, Node::Parameter> *map = n->getParams();
        table->setRowCount(map->size() + 5);
        int i = 0;
        // create the entries all nodes have
        QTableWidgetItem *name = new QTableWidgetItem(),
                *ID = new QTableWidgetItem(),
                *type = new QTableWidgetItem();

        // set the data
        ID->setData(0, nodeID);
        name->setData(0, n->getName());
        type->setData(0, n->getType());

        // asign to table:

        // NodeID
        table->setItem(i, 0, new QTableWidgetItem("NodeID"));
        table->setItem(i, 1, ID);
        table->item(i, 0)->setFlags(table->item(0, 0)->flags() ^
                                    (Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        i++;

        // Node Class
        table->setItem(i, 0, new QTableWidgetItem("Node Class"));
        table->setItem(i, 1, type);
        table->item(i, 0)->setFlags(table->item(i, 0)->flags() ^
                                    (Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        i++;

        // input Gates

        bool moreThanOne = n->getInputGates()->size() > 1;
        if (!n->getInputGates()->isEmpty()) {
            QString types;
            for (Gate *g : *n->getInputGates())
                for (QString s : g->getTypes()) types += s + ", ";
            types.chop(2);
            QTableWidgetItem *in = new QTableWidgetItem();
            in->setText(types);
            in->setFlags(in->flags() ^ (Qt::ItemIsEditable | Qt::ItemIsSelectable));
            table->setItem(i, 0, new QTableWidgetItem(
                               "Input" + QString((moreThanOne ? "s" : ""))));
            table->setItem(i, 1, in);
            table->item(i, 1)->setToolTip(types);
            i++;
        }

        // output gates
        moreThanOne = n->getOutputGates()->size() > 1;
        if (!n->getOutputGates()->isEmpty()) {
            QString types;
            for (Gate *g : *n->getOutputGates())
                for (QString s : g->getTypes()) types += s + ", ";
            types.chop(2);
            QTableWidgetItem *out = new QTableWidgetItem;
            out->setText(types);
            out->setFlags(out->flags() ^ (Qt::ItemIsEditable | Qt::ItemIsSelectable));
            table->setItem(i, 0, new QTableWidgetItem(
                               "Output" + QString((moreThanOne ? "s" : ""))));
            table->setItem(i, 1, out);
            table->item(i, 1)->setToolTip(types);
            i++;
        }
        table->setItem(i, 0, new QTableWidgetItem("Node Name"));
        table->setItem(i, 1, name);
        i++;

        // make name editable
        name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable |
                       Qt::ItemIsSelectable);

        // avoid editing keys, and nodeID/type
        ID->setFlags(ID->flags() ^ (Qt::ItemIsEditable | Qt::ItemIsSelectable |
                                    Qt::ItemIsEnabled));
        type->setFlags(type->flags() ^ (Qt::ItemIsEditable | Qt::ItemIsSelectable |
                                        Qt::ItemIsEnabled));

        for (int j = 0; j < i; j++)
            table->item(j, 0)->setFlags(table->item(j, 0)->flags() ^
                                        (Qt::ItemIsEditable | Qt::ItemIsSelectable));
        QStringList keys = map->keys();
        int count = i;
        for (int j = 0; j < keys.size(); j++) {
            count++;
            // create key item
            QString key = keys.value(j);
            QTableWidgetItem *key_item = new QTableWidgetItem((*map)[key].name);
            key_item->setData(Qt::UserRole, key);
            key_item->setFlags(key_item->flags() ^
                               (Qt::ItemIsEditable | Qt::ItemIsSelectable));
            key_item->setToolTip((*map)[key].descr);
            table->setItem(i + j, 0, key_item);

            // create value item
            QVariant value = map->value(key).value;
            QTableWidgetItem *value_item = new QTableWidgetItem();
            value_item->setToolTip((*map)[key].descr);
            // QSpinBox *spinner;
            switch (value.userType()) {
            case QVariant::Bool:
                // value_item->setData(0, value);
                value_item->setCheckState(value.toBool() ? Qt::Checked
                                                         : Qt::Unchecked);
                table->setItem(i + j, 1, value_item);
                break;
            default:
                value_item->setData(Qt::UserRole, value);
                value_item->setText(value.toString());
                table->setItem(i + j, 1, value_item);
                if (key.contains("file")) {
                    connect(table, SIGNAL(cellDoubleClicked(int, int)), this,
                            SLOT(onFilebuttonClicked(int, int)));
                }
                break;
            }
        }
        table->setRowCount(count);
        connect(table, SIGNAL(itemChanged(QTableWidgetItem *)), Data::instance(),
                SLOT(updateNode(QTableWidgetItem *)));
        table->resizeColumnsToContents();
        if (ui->details->checkState() == Qt::Checked) table->show();
    } else if (table->isVisible()) {
        table->hide();
    }
}

void MainWindow::deleteTable() {
    QTableWidget *table = ui->tableWidget;
    table->disconnect(table, SIGNAL(itemChanged(QTableWidgetItem *)),
                      Data::instance(), SLOT(updateNode(QTableWidgetItem *)));
    table->disconnect(table, SIGNAL(cellDoubleClicked(int, int)), this,
                      SLOT(onFilebuttonClicked(int, int)));
    table->hide();
    // delete all tableitems, because they aren't needed any more
    for (int col = 0; col < table->columnCount(); col++)
        for (int row = 0; row < table->rowCount(); row++)
            delete table->item(row, col);
    table->setRowCount(0);
}

void MainWindow::displayTypeInfo(const QString &type) {
    deleteTable();
    oldfocus = -1;
    QTableWidget *table = ui->tableWidget;
    Node n = *Data::instance()->getNodeCatalog()->getPointerOfType(type);
    // i is to count the rows set up
    int i = 0;
    const QMap<QString, Node::Parameter> *params = n.getParams();

    // init the table
    table->setRowCount(((params->size()) + 4));
    table->setColumnCount(2);

    // print the Node Type
    table->setItem(i, 0, new QTableWidgetItem("Type"));
    table->setItem(i, 1, new QTableWidgetItem(type));
    QFont f1 = table->item(i, 1)->font(), f2 = table->item(i, 0)->font();
    f1.setBold(true);
    f2.setBold(true);
    table->item(i, 0)->setFont(f2);
    table->item(i++, 1)->setFont(f1);

    // print input gates
    QVector<Gate *> *gates = n.getInputGates();
    bool moreThanOneGate = gates->size() > 1;
    QString gateTypes;
    // if there are inputs, print them
    if (!gates->isEmpty()) {
        table->setItem(i, 0, new QTableWidgetItem(
                           "Input" + QString((moreThanOneGate ? "s" : ""))));
        for (Gate *g : *gates)
            for (QString s : g->getTypes()) gateTypes += s + ", ";
        gateTypes.chop(2);
        table->setItem(i++, 1, new QTableWidgetItem(gateTypes));
    }

    // print outputs
    gateTypes = "";
    gates = n.getOutputGates();
    moreThanOneGate = gates->size() > 1;
    if (!gates->isEmpty()) {
        for (Gate *g : *gates)
            for (QString s : g->getTypes()) gateTypes += s + ", ";
        gateTypes.chop(2);
        table->setItem(i, 0, new QTableWidgetItem(
                           "Output" + QString((moreThanOneGate ? "s" : ""))));
        table->setItem(i++, 1, new QTableWidgetItem(gateTypes));
    }

    QString descr = n.getDescription();
    if (descr != "") {
        table->setRowCount(table->rowCount() + 1);
        table->setItem(i, 0, new QTableWidgetItem("Description"));
        table->setItem(i, 1, new QTableWidgetItem(descr));
        table->item(i++, 1)->setToolTip(descr);
    }
    QList<QString> keys = params->keys();
    for (int j = 0; j < keys.size(); j++) {
        table->setItem(i, 0, new QTableWidgetItem((*params)[keys[j]].name));
        table->setItem(i, 1, new QTableWidgetItem((*params)[keys[j]].type));
        table->item(i, 1)->setToolTip((*params)[keys[j]].descr);
        i++;
    }
    table->setRowCount(i);
    for (int col = 0; col < table->columnCount(); col++)
        for (int row = 0; row < table->rowCount(); row++)
            if (table->item(row, col))
                table->item(row, col)->setFlags(Qt::ItemIsEnabled);
    table->resizeColumnToContents(0);
    table->resizeColumnToContents(1);
    for (int col = 0; col < table->columnCount(); col++)
        for (int row = 0; row < table->rowCount(); row++)
            table->item(row, col)->setToolTip(table->item(row, col)->text());
    if (ui->details->checkState() == Qt::Checked) table->show();
}

void MainWindow::on_details_stateChanged(int arg1) {
    if (arg1 && Data::instance()->getFocusedID() != -1)
        ui->tableWidget->show();
    else
        ui->tableWidget->hide();
}

void MainWindow::onFilebuttonClicked(int row, int col) {
    if (col != 1) return;
    QString key = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toString();
    if (!key.contains("file", Qt::CaseInsensitive)) return;
    QString fileName;
    if (key.contains("output", Qt::CaseInsensitive))
        fileName = QFileDialog::getSaveFileName(this, "Select a File...", "~/");
    else
        fileName = QFileDialog::getOpenFileName(this, "Select a File...", "~/");
    if (fileName == "") return;
    ui->tableWidget->item(row, col)->setText(fileName);
}

void MainWindow::on_start_button_clicked() {
    Data::instance()->testChangeRun();
    SingletonRender::instance()->setStatusColor(0, Node::error);
}

void MainWindow::on_stop_button_clicked() { Data::instance()->setEditMode(); }

void MainWindow::on_actionStop_Simulation_triggered() {
    Data::instance()->stopSimulation();
}

void MainWindow::on_actionAbout_triggered() {
    QString aboutText;
    aboutText += "This GUI was developed due to";
    aboutText += "\"Bachelorpraktikum WS14/15\" at the ";
    aboutText += "department of computer science ";
    aboutText += "TU Darmstadt.\nDevelopers:\n";
    aboutText += "\t-Frederik Lührs\n";
    aboutText += "\t-Tobias Lippert\n";
    aboutText += "\t-Artur Fast\n";
    aboutText += "\t-Mehrad Mohammadian\n";
    QMessageBox::information(this, "About Anise GUI", aboutText, QMessageBox::Ok);

}

void MainWindow::on_actionRun_Mesh_triggered(QString line) {

    line = line.mid(line.indexOf("{"), line.lastIndexOf("}") + 1);
    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject obj = doc.object();
    QString message = " ";

    if(obj.contains("log")){
        obj = obj["log"].toObject();
        QString msg = obj["msg"].toString();
        QString source = obj["source"].toString();

        // message comes from a node
        if (source == "node") {
            QString nodeName = obj["src_name"].toString();
            QString status = obj["status"].toString();
            QString currentTime =  obj["time"].toString();
            message = message + currentTime + " " + nodeName + " ";
            if (status == "info") {
                message = message + msg;
            } else if (status == "warning") {
                message = message + msg;
            } else if (status == "error") {
                message = message + msg;
            }
        }
        else if (source == "framework") {
            QString framework = obj["src_name"].toString();
            message = message + "Framework::" + framework + " ";
            QString status = obj["status"].toString();
            QString currentTime =  obj["time"].toString();
            message = message + currentTime + " ";
            if (status == "info") {
                message = message + msg;
            } else if (status == "warning") {
                message = message + msg;
            } else if (status == "error") {
                message = message + msg;
            }
        }
    }else{
        return;
    }

  ui->qDebug_out->append(message);
}
