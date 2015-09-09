#ifndef SINGLETONRENDER_H
#define SINGLETONRENDER_H


#include "mainwindow.h"
#include "node.h"
#include "drawobject.h"

/**
  * SingletonRender handles all graphical output.
  *
  *
  */

// The life of the singleton instantiation is for the duration of the
// application.

class SingletonRender : public QObject{

Q_OBJECT

public:
    /**
   * @brief Will return a pointer to the Singleton Instance.
   *
   * Will return a pointer to the existing SingletonRender Instance, if no
   * Instance exists
   * a new instance will be constructed.
   *
   * @return pointer to Instance of Singleton Class
   */
    static SingletonRender *instance();

    /**
    * @brief loads all images in ../Data/Images/ eding in *png
    *
    * @return TRUE if no prolems accured while loading the images.
    *         FALSE if loading a file failed.
    *
    *
    */

    // will load all images in ../Data/Images/ into the allImages Map
    bool loadImages();

    // 16 by 16 Tile images for the node
    /*
   * 0|1|2
   * 3|4|5
   * 6|7|8
   */
    QPixmap nodeTiles[9];

    /**
   *
   * Creates 9 Images from a Sprite and stores them in the array
   *"this.nodeTiles".
   * One Tile is (16x16) pixel
   *
   *
   *
   * @brief creates 9 Tiles from a Sprite.
   *
   * @param Sprite, QPixmap that will be divided into 9 Tiles.
   * @return FALSE if Sprite==NULL
   *          TRUE else
   */

    bool createTilesFromImage(QPixmap *Sprite);

    /**
   *
   * Will Contruct an Image made out of Tiles. Tiles are stored in
   *"this.nodeTiles".
   * Minimum size it will return is 3x3 Tiles.
   *
   *
   * @brief createTiledPixmap will retur a Pixmap made out of Tiles
   * @param x number of tiles in x dimension
   * @param y number of tiles in y dimension
   * @return Contructed QPixma from Tiles
   */
    QPixmap *createTiledPixmap(const int &x, const int &y);

    /**
   *
   *
   * @brief setUi
   * @param ui
   */
    void setUi(Ui::MainWindow *ui);

   /**
   * @brief clearMeshField, will clear the meshfield
   */
    void clearMeshField();

    /**
   *
   *
   * @brief deleteMeshDrawing, delete a drawobject savely
   * @param objectID that will be removed
   * @return true if removed successful
   */

    bool deleteMeshDrawing(const int &objectID);

    /**
   * @brief getChildrenIDs Gets all DrawObject ids of Mesh field.
   * @return Vector filled with Ids
   */
    QVector<int> *getChildrenIDs();

    /**
   * @brief renderConnection reders a conection into the meshfield
   * @param conToRender the connection that will be rendered
   */
    void renderConnection(Connection *conToRender, const int &ID);
    /**
   * @brief renderConnections overloaded function
   * @see renderConnection(Connection *, int);
   */
    void renderConnections();

    /**
   *
   * Creates a DrawOject if this node is hasnt been rendered yet.
   * Redraws the node if node already has been created.
   *
   * @brief renderNode reders a node into the meshfield
   * @param nodeToRender node that will be redered
   * @param nodeID Unique Id of the Node
   */
    void renderNode(Node *nodeToRender, const int &nodeID);

    /**
   * @brief renderNodeType reders one node type in the catalog
   * @param nodeToRender node that will be rendered
   * @param parent parent for the rendered node
   * @param position at which position
   */
    void renderNodeType(Node *nodeToRender, QWidget *parent, const int &position);

    /**
   * @brief renderCatalogContent renderes all nodes in the catalog
   * @param NodeVektor nodes that will be rendered
   */
    void renderCatalogContent(const QVector<Node> &NodeVektor);
    /**
   * @brief drawLine draws a line from start to end
   * @param start_x
   * @param start_y
   * @param end_x
   * @param end_y
   */
    void drawLine(const double &start_x, const double &start_y, const double &end_x, const double &end_y);

    /**
   * @brief drawLine draws a Line
   * @param line as a QLine instance
   */
    void drawLine(const QLine &line);

    /**
   * @brief drawLine draws a line
   * @param start QPoint origin of the line
   * @param end QPoint end of the line
   */
    void drawLine(const QPoint &start, const QPoint &end);

    /**
   * @brief drawLines draws a line conecting all Points in the vector
   * @param pointVector a vector filled with QPoints
   */
    void drawLines(QVector<QPoint> *pointVector);

    /**
   * @brief drawLines draws a line conecting all Points in the vector ending in
   * the point
   * @param pointVector a vector with points
   * @param point the point that will be conected last
   */
    void drawLines(QVector<QPoint> *pointVector, QPoint *point);

    /**
   * @brief getOutputGateDrawOffset
   * @return Offset of the gate
   */
    QPoint getOutputGateDrawOffset() const;

    /**
   * @brief setOutputGateDrawOffset, setter
   * @param value will be the new offset
   */
    void setOutputGateDrawOffset(const QPoint &value);

    void updateConnections(const int &nodeID, const QPoint &offset);
    QPoint getInputGateDrawOffset() const;
    void setInputGateDrawOffset(const QPoint &value);
     void setDrawOffset(const QPoint &value);

    void dehighlightObject(const int &ID);
    void highlightObject(const int &ID);

    QPixmap *getImage(const QString &name);
    void highlightGates(const QString &gateType);
    void dehighlightGates();



    void setNodeName(const int &nodeID, const QString &nodeName);
    void clearAllConnections();
    QMap<int, QVector<DrawObject *> > *getAllConnections();


    void setStatusColor(int nodeID, Node::Status status);
    void setPercentage(int nodeID, int percentage);
    void rerender(Node *nodeToRender, const int &nodeID);
    void showProgressButtons(int nodeID, int buttonID);

public slots:
    void changeProgressView();
    /**
   *
   * @brief renderMesh, Will re-Render the whole Mesh.
   */
    void renderMesh();
private:
    void paintEvent(QPaintEvent *event);

    void clearAll(QWidget *parent);  // Deletes all children of given parent

    QPainter painter;

    // this is the map containing all raw Images.
    // Qstring is the same as the filename
    // QPixmap a .png image
    QMap<QString, QPixmap *> allImages;

    QPoint inputGateDrawOffset;
    QPoint outPutGateDrawOffset;

    // Different lists because we will need to work differently with them
    QMap<int, DrawObject *> allDrawnNodes;
    QMap<int, QVector<DrawObject *> > allConnections;
    QMap<int, QVector<QLine> > allLines;
    QColor lineColor = QColor(30, 30, 35);

    // window of application
    Ui::MainWindow *ui;

    //###### singleton stuff ########

    // Private so that it can  not be called
    // Only the class function Instance can call the constructor. Public access to
    // the constructor is denied.
    SingletonRender();

    // The constructor, copy constructor and assignment operator are all private
    // to ensure that the
    // programmer using the singleton class can only create a single instance of
    // the class using only the Instance() function.

    // copy constructor is private
    SingletonRender(SingletonRender const &) : QObject(){}

    // assignment operator is private
    SingletonRender &operator=(SingletonRender const &) { return *this; }

    // pointer to this instance
    static SingletonRender *m_pInstance;

    void moveJointsOnJoints(Connection *conToRender, const int &ID);

};

#endif  // SINGLETONRENDER_H
