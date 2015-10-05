#ifndef DATA_H
#define DATA_H

#include <QObject>
#include <QDir>
#include "mesh.h"
#include "nodecatalog.h"
#include "mainwindow.h"
#include "render/singletonrender.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "node.h"
#include "nodefactory.h"
#include "anisecommunicator.h"
#include <QTimer>

/**
 *  @Author Frederik Lührs
 *
 * Singleton class storing all data in one object (Mesh, nodes, node types,
 * important paths, ...)
 */
class Data : public QObject {
    Q_OBJECT

public:
    /**
    * The Mesh of user is currently working at
    */
    Mesh *mesh;

    /**
   * @brief nodeFactory
   * This will create any kind of Node and Node Templates.
   *
   */
    NodeFactory *nodeFactory;
    /**
    * The Hash Map used for categorizing nodes
    * */

    QHash<QString,Node> categoryWithNodeList;

    /**
    * The instance function returns a pointer to a static variable and thus is
    * declared static
    */
    static Data *instance();

    void initialize(MainWindow *mainWindow);

    /**
   * @brief getNodeCatalog
   * @return the NodeCatalog containing all types of nodes available
   */
    NodeCatalog *getNodeCatalog();

    /**
     * @brief addNodeToMesh inserts given node into the mesh and renders it afterwards
     * @param newNode
     * @return ID of inserted Node
     */
    int addNodeToMesh(Node *newNode);
    /**
     * @brief addConnectionToMesh inserts new Connection to mesh
     * @param newConnection
     * @return
     * @see addNodeToMesh
     */
    int addConnectionToMesh(Connection *newConnection);
    /**
     * @brief sortCircle
     * @see Mesh::sortCircle()
     */
    void sortCircle();
    /**
     * @brief sortRow
     * @see Mesh::sortRow()
     */
    void sortRow();
    /**
     * @brief sortForce
     * @see Mesh::sortForce()
     */
    void sortForce();

    int getFocusedID();

    /**
     * @brief checkConnection verify if a connection is valid
     * @param srcNodeID
     * @param srcGate
     * @param destNodeID
     * @param destGate
     * @return
     */
    bool checkConnection(int srcNodeID, QString srcGate, int destNodeID, QString destGate);

    /**
     * @brief inCatalog checks if given type of node is in the catalog
     * @param type nodetype to be checked
     * @return
     */
    bool inCatalog(const QString &type);
    bool hasChanged();
    void unsetChanged();
    bool mustRepaint();
    void unsetRepaint();
    void setChanged();
    /**
     * @brief getNodeByName searches for node by name
     * @param name
     * @return
     */
    Node *getNodeByName(QString name);
    Node *getNodeByID(int ID);
    QList<Node *> getAllNodes();
    QList<Connection *> getAllConnections();
    QMap<int, Node *> *nodesInMesh();
    QString getValidAlternativeForName(const QString &name);
    /**
     * @brief moveObjectInMesh
     * @param Position
     * @param ID
     * @see moveObjectInMesh(QPoint*,QPoint*,int)
     */
    void moveObjectInMesh(QPoint *Position, int ID);
    /**
     * @brief moveObjectInMesh moves drwan object with given id from start to end
     * @param start start point
     * @param end end point
     * @param ID ID of moving item
     */
    void moveObjectInMesh(QPoint *start, QPoint *end, int ID);
    void finishMesh();
    void newMeshProject();
    void clearMesh();
    Mesh *getMesh();

    MainWindow *getMainWindow() const;
    void setMainWindow(MainWindow *value);
    void setEditMode();
    void setDrawLineMode(QString gateType);
    bool isSimulating();
    QList<Connection *> getConnections(const int &nodeID);

    QString getSaveFile() const;
    void setSaveFile(const QString &value);

    bool isRunning() const;

    void testChangeRun();
    bool isExecutable() const;
    void setExecutable(bool value);

    int getLastExitCode() const;
    void setLastExitCode(int value);

    int getAutosave_interval() const;
    void setAutosave_interval(int value);
    void displayNodeCategory(const QVector<Node> &NodeVektor);
    bool getMoved() const;
    void setMoved(bool value);

signals:

    void runModeChanged();

public slots:
    /**
     * @brief deleteItem deletes the item which is in focus
     * @return
     */
    bool deleteItem();
    /**
     * @brief setFocusMeshObject sets the focus pointer to the specified object
     * @param objectID
     */
    void setFocusMeshObject(int objectID);
    /**
     * @brief updateNode updates the node parmeter belonging to given item
     * @param item item of propertytable that has changed
     */
    void updateNode(QTableWidgetItem *item);
    void startSimulation();
    void stopSimulation();
    void runMesh();
    void onNodeCategoryItemClicked();
private slots:
    /**
     * @brief autosaveMesh saves a backup file of the current mesh
     */
    void autosaveMesh();
private:
    explicit Data(QObject *parent = 0);

    bool runMode;

    /**
    * It is Private so that it can't be called.
    * Only the class function Instance can call the constructor.
    * Public access to the constructor is denied.
    *
    * The constructor, copy constructor and assignment operator are all private
    * to ensure that the programmer using the singleton class can only create a
    * single instance of the class using only the instance() function.
    *
    */

    // copy constructor does not need to be overridden because QObject doesnt has
    // a public copy instruktor

    // assignment constructor
    Data &operator=(Data const &) { return *this; }

    /**
    * Pointer to this instance
    */
    static Data *data;
    MainWindow *mainWindow;
    /**
    * Contains all Node types
    * (Unique list of node objects of all types)
    */
    NodeCatalog *nodeCatalog;
    bool changed = false;
    bool repaint = false;
    bool moved=false;
    bool onSimulation;
    //timer for autobackups
    QTimer *backupTimer;
    //directory for savings
    QDir autosave;
    //location of current save file
    QString saveFile;
    AniseCommunicator *framework;
    //interval to create backup files in ms
    int autosave_interval;
    //location of last made backup
    QString lastBackupFile;
    bool executable = true;
    int lastExitCode = 0;
};

#endif  // DATA_H
