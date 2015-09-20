#include <QTime>
#include <QDate>
#include "data.h"
#include "settingshandler.h"
#include "nodefactory.h"
#include "anisecommunicator.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

/**
 * @brief Data::data pointer to instance of this singleton class
 */
Data *Data::data = 0;

/** This function is called to create an instance of the class.
    Calling the constructor publicly is not allowed. The constructor
    is private and is only called by this Instance function.
*/
Data *Data::instance() {
    /**
* Allows only one instance of class to be generated.
*/
    if (!data) data = new Data;
    return data;
}

Data::Data(QObject *parent) : QObject(parent) {
    mesh = new Mesh();
    nodeCatalog = 0;
    nodeFactory = 0;
    saveFile = "";
    framework = new AniseCommunicator();

    // set up the timer for backups
    backupTimer = new QTimer(this);
    connect(backupTimer, SIGNAL(timeout()), this, SLOT(autosaveMesh()));
    if(!SettingsHandler::contains("autosave_interval")){
        autosave_interval = 3e5;
        SettingsHandler::storeSetting("autosave_interval", QString::number(autosave_interval));
    }
    backupTimer->start(autosave_interval);

    // set up directory for backups
    autosave = QDir::current();
    autosave.cd("data/meshes");
    autosave.mkdir("autosave");
    autosave.cd("autosave");

    runMode = false;

    SingletonRender *renderer = SingletonRender::instance();

    connect(this, SIGNAL(runModeChanged()), renderer, SLOT(changeProgressView()));
}
bool Data::getMoved() const
{
    return moved;
}

void Data::setMoved(bool value)
{
    moved = value;
}


int Data::getAutosave_interval() const
{
    return autosave_interval;
}

void Data::setAutosave_interval(int value)
{
    autosave_interval = value;
    SettingsHandler::storeSetting("autosave_interval", QString::number(value));
}

int Data::getLastExitCode() const
{
    return lastExitCode;
}

void Data::setLastExitCode(int value)
{
    lastExitCode = value;
}


void Data::setExecutable(bool value)
{
    executable = value;
}


bool Data::isRunning() const { return runMode; }

void Data::testChangeRun() {
    runMode = true;
    emit runModeChanged();
}

QString Data::getSaveFile() const { return saveFile; }

void Data::setSaveFile(const QString &value) { saveFile = value; }

MainWindow *Data::getMainWindow() const { return mainWindow; }

void Data::setMainWindow(MainWindow *value) { mainWindow = value; }

/**
 * @brief Data::initialize initilize all important containers.
 * @param mainWindow
 */
void Data::initialize(MainWindow *mainWindow) {
    changed = false;
    this->mainWindow = mainWindow;
    // connect start and stop signals
    connect(mainWindow->ui->actionRun_Mesh, SIGNAL(triggered()), this,
            SLOT(runMesh()));
    connect(mainWindow->ui->actionStop_Simulation, SIGNAL(triggered()),this,SLOT(stopSimulation()));
    //connect the on click even of items in node category list
    connect(mainWindow->ui->NodeCategories_List,SIGNAL(itemSelectionChanged()),
            this, SLOT(onNodeCategoryItemClicked()));

    /**
* Create the Nodecatalog
*/
    if (nodeCatalog) delete nodeCatalog;
    nodeCatalog = new NodeCatalog();
    /**
* Create the render object
*/
    SingletonRender::instance();
    /**
* Create the NodeFactory
*/
    if (nodeFactory) delete nodeFactory;
    this->nodeFactory = new NodeFactory();

    /**
* Initialize stored settings
*/
    SettingsHandler::setSettingsPath(QApplication::applicationDirPath() +
                                     "/settings.ini");

    /**
    * Initialize settings from .ini file
    */
    SettingsHandler::initializeSettings();

    /**
* Checks if Framework path is set
*/
    if (!SettingsHandler::contains("frameworkpath")){
        int choice = QMessageBox::information(
                    0, QString("Please, set your framework path"),
                    QString("You haven't set your framework path yet.\nChoose first!"),
                    "Choose", "Ignore");
        if (choice == 1) return;
        QString fileName = QFileDialog::getOpenFileName(
                    mainWindow, "Set your framework path", "", "");

        SettingsHandler::storeSetting("frameworkpath", fileName);
    }
    AniseCommunicator::setFrameworkPath(
                SettingsHandler::getSetting("frameworkpath"));

    /**
* Start loading node types
* Load all available NodeTypes
*/
    QString out = AniseCommunicator::getAllNodeTypes();
    JsonFileHandler::parseNodeTypesFromAnise(out);

    /**
* Render the Nodecatalog depending on Category filled with test nodes
*/
    displayNodeCategory(Data::instance()->getNodeCatalog()->getContentVector());
}

//Display the list of node categories
void Data::displayNodeCategory(const QVector<Node> &NodeVektor) {
    //clear the node catalog content list for the current instance
    categoryWithNodeList.clear();
    foreach(Node node,NodeVektor){
        QString category = node.getCategory();
              categoryWithNodeList.insertMulti(category,node);
    }
    QStringList categories;
    int i=0;
    QListIterator<QString> it(categoryWithNodeList.keys());
    while(it.hasNext()){
        QString tempCategory=it.next();
        if(!categories.contains(tempCategory))
        {
            QTreeWidgetItem *item=new QTreeWidgetItem(mainWindow->ui->NodeCategories_List);
            item->setText(0,tempCategory);
            mainWindow->ui->NodeCategories_List->insertTopLevelItem(i++,item);
            categories.append(tempCategory);
        }
    }


}

// Display node catalog for a category depending on the click event
void Data::onNodeCategoryItemClicked()
{
    QTreeWidgetItem* item;
    QList<QTreeWidgetItem*> itemsSelected=mainWindow->ui->NodeCategories_List->selectedItems();
    for(QTreeWidgetItem* it:itemsSelected){
        if(itemsSelected.length()==1)
            item=it;
    }
    QString category=item->text(0);
    QList<Node> nodeList=categoryWithNodeList.values(category);
    QVector<Node> categorizedNodeList=nodeList.toVector();
    SingletonRender::instance()->renderCatalogContent(categorizedNodeList);
}



int Data::addNodeToMesh(Node *newNode) {
    //insert node into mesh
    int id = this->mesh->addNode(newNode);

    if (id >= 0 && !JsonFileHandler::isParsing()) {
        //repaint the mesh
        SingletonRender::instance()->renderMesh();
        changed = true;
    }

    // A new created Node is always focussed in the beginning
    mesh->setFocusMeshObject(id);

    return id;
}

int Data::addConnectionToMesh(Connection *newConnection) {
    //add the connection to the mesh
    int id = this->mesh->addConnection(newConnection);
    if (id >= 0 && !JsonFileHandler::isParsing()) {
        //repaint the mesh
        SingletonRender::instance()->renderMesh();
        changed = true;
    }
    return id;
}

void Data::sortCircle() {
    if (mesh) {
        mesh->sortCircle();
        changed = true;
    }
}

void Data::sortRow() {
    if (mesh) {
        mesh->sortRow();
        changed = true;
    }
}

void Data::sortForce() {
    if (mesh) {
        mesh->sortForce();
        changed = true;
        moved=true;
    }
}

void Data::runMesh() {
    if(!executable){
        QMessageBox::critical(mainWindow,"Cannot execute", "Current mesh file is not executable!");
        return;
    }
    if (changed) {
        int answer = QMessageBox::question(
                    mainWindow, "Save before executing?",
                    "You must save your changes before executing. Save now?",
                    QMessageBox::No, QMessageBox::Yes);
        if (answer == QMessageBox::Yes) {
            mainWindow->on_actionSave_triggered();
        }
        else return;
    }
    if (saveFile == "") return;
    framework->runMesh();
    runMode = true;
    onSimulation = true;
    emit runModeChanged();
}

bool Data::isExecutable() const
{
    return executable;
}

int Data::getFocusedID() { return mesh->focusObject; }

bool Data::checkConnection(int srcNodeID, QString srcGate, int destNodeID,
                           QString destGate) {
    if (mesh)
        return mesh->checkConnection(srcNodeID, srcGate, destNodeID, destGate);
    else
        return false;
}

bool Data::inCatalog(const QString &type)
{
    return this->nodeCatalog->contains(type);
}

bool Data::hasChanged() { return changed; }

void Data::unsetChanged() { changed = false; }

bool Data::mustRepaint() { return repaint; }

void Data::unsetRepaint() { repaint = false; }

void Data::setChanged() {
    changed = true;
    repaint = true;
}

Node *Data::getNodeByName(QString name) {
    if (mesh) return mesh->getNodeByName(name);
    return 0;
}

Node *Data::getNodeByID(int ID) {
    if (mesh) return mesh->getNodeByID(ID);
    return 0;
}

QList<Node *> Data::getAllNodes() {
    QList<Node *> result;
    if (mesh) result = mesh->getAllNodes();
    return result;
}

QList<Connection *> Data::getAllConnections() {
    QList<Connection *> result;
    if (mesh) result = mesh->getAllConnections();
    return result;
}

QMap<int, Node *> *Data::nodesInMesh() {
    if (mesh) return &mesh->nodesInMesh;
    return 0;
}

QString Data::getValidAlternativeForName(const QString &name) {
    QString result;
    if (mesh) result = mesh->getValidAlternativeForName(name);
    return result;
}

void Data::moveObjectInMesh(QPoint *start, QPoint *end, int ID) {
    // is object a node?
    if (this->mesh->nodesInMesh.contains(ID)) {
        this->moveObjectInMesh(end, ID);
        changed = true;
        moved=true;
    }
    // is it a connection?
    else if (this->mesh->connectionsInMesh.contains(ID)) {
        int joint = this->mesh->getConnectionByID(ID)->getJoint(start);
        // this is just a temp. solution!
        this->mesh->getConnectionByID(ID)->setJoint(joint, end);
        changed = true;
        SingletonRender::instance()->renderMesh();
    }
}

void Data::finishMesh() {
    for (Node *n : mesh->nodesInMesh){
        n->setProgress(0);
    }
}

void Data::clearMesh() {
    for (Node *n : mesh->nodesInMesh){
           n->setProgressButton(-1);
    }
}

void Data::moveObjectInMesh(QPoint *Position, int ID) {
    //check if object is a node
    if (this->mesh->nodesInMesh.contains(ID)) {
        this->mesh->getNodeByID(ID)->moveTo(Position->x(), Position->y());
        changed = true;
        moved=true;
        SingletonRender::instance()->renderMesh();
    }
    //check if object is a connection
    else if (this->mesh->connectionsInMesh.contains(ID)) {
        qDebug() << "cant move a connection joint with onle one QPoint";
        return;
    }
}

Mesh *Data::getMesh() { return this->mesh; }

void Data::newMeshProject() {
    // create new mesh and delete old
    delete mesh;
    mesh = new Mesh();
    setFocusMeshObject(-1);
    // reset file locations
    lastBackupFile = "";
    saveFile = "";
    // set not changed
    changed = false;
    // restart backup timer
    backupTimer->stop();
    backupTimer->start(autosave_interval);
    // clear the mesh field
    SingletonRender::instance()->clearMeshField();
    mainWindow->ui->meshField->connectSignals();
    // render the new mesh
    SingletonRender::instance()->renderMesh();
}

bool Data::deleteItem() {
    bool deleted = this->mesh->deleteItem();
    if (deleted) changed = true;
    return deleted;
}

NodeCatalog *Data::getNodeCatalog() { return nodeCatalog; }

void Data::setDrawLineMode(QString gateType) {
    this->mesh->setFocusMeshObject(-1);
    SingletonRender::instance()->highlightGates(gateType);
}

bool Data::isSimulating() { return onSimulation; }

void Data::setFocusMeshObject(int objectID) {
    if (mesh) mesh->setFocusMeshObject(objectID);
}

void Data::updateNode(QTableWidgetItem *item) {
    if (mesh) mesh->updateNode(item);
}

void Data::startSimulation() { onSimulation = true; }

void Data::stopSimulation() {
    onSimulation = false;
    runMode = false;
    framework->stop();
    emit runModeChanged();
}

void Data::autosaveMesh() {
    if (!autosave.exists() || !changed) return;
    QString time = QTime::currentTime().toString("hhmmss"),
            date = QDate::currentDate().toString("yyyyMMdd");
    QString savename = "backup" + date + time + ".mesh";
    savename = autosave.absolutePath() + "/" + savename;
    JsonFileHandler::saveMesh(savename);
    // if there was a previous backup delete it!
    if (lastBackupFile != "") QFile::remove(lastBackupFile);
    lastBackupFile = savename;
}

void Data::setEditMode() {
    SingletonRender::instance()->dehighlightGates();
    runMode = false;
    emit runModeChanged();
}

QList<Connection *> Data::getConnections(const int &nodeID) {
    QList<Connection *> result;
    for (Connection *c : mesh->connectionsInMesh.values())
        if (c->getSrcNode()->getID() == nodeID ||
                c->getDestNode()->getID() == nodeID)
            result << c;
    return result;
}
