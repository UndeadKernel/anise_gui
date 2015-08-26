#include <QMimeData>
#include <QDrag>
#include <QToolTip>
#include "mesheditorwidget.h"
#include "data.h"
#include "gatebutton.h"
#include "math.h"
#include "parseerrorbox.h"

MeshEditorWidget::MeshEditorWidget(QWidget *parent) : QWidget(parent) {
    connectSignals();
    this->setMinimumHeight(5000);
    this->setMinimumWidth(5000);
    clearNewLine();
}

void MeshEditorWidget::connectSignals() {
    connect(this, SIGNAL(onWidgetClicked(int)), Data::instance(),
            SLOT(setFocusMeshObject(int)));
    connect(this, SIGNAL(drawLineModeChanged()), this,
            SLOT(changeLineDrawMode()));
}

void MeshEditorWidget::clearNewLine() {
    newLine = NewLine();
    newLine.drawLine = false;
    newLine.srcNodeID = -1;
    newLine.srcGateName = "";
    newLine.destNodeID = -1;
    newLine.destGateName = "";
    newLine.wayPoints = QVector<QPoint>();
    emit drawLineModeChanged();
}

void MeshEditorWidget::resetToEditMode() {
    this->clearNewLine();
    Data::instance()->setEditMode();
    repaint();
}

void MeshEditorWidget::mousePressEvent(QMouseEvent *event) {
    if(Data::instance()->isRunning())
        return;

    this->mouseMoveDistance += 100;

    // ensure a left-mouse-click
    if (!(event->button() == Qt::LeftButton)) {
        this->resetToEditMode();
        return;
    }

    // drawLine Mode
    if (newLine.drawLine) {
        // Check if i clicked on a child
        QObject *child = childAt(event->pos());

        if (!child) {
            // add a way point for the line to draw
            this->newLine.wayPoints.push_back(event->pos());

        } else if (dynamic_cast<QLabel *>(child)) {
            // clicked object is a label

            QLabel *label = dynamic_cast<QLabel *>(child);
            if (dynamic_cast<DrawObject *>(label->parent())) {
                // clicked object is part of a drawobject

                DrawObject *draObj = dynamic_cast<DrawObject *>(label->parent());

                // drawobject has an id and can be compared to source node
                if (draObj->ID == newLine.srcNodeID) {
                    this->resetToEditMode();
                }
            }
        }
    }
    // Normal Mode (currently not drawing a Connection)
    else {
        DrawObject *child = 0;
        QLabel *labelChild = 0;
        // GateButton *buttonChild = 0;

        //---Clicked on Node?----------
        // check of clicking on label (picture)
        labelChild = dynamic_cast<QLabel *>(childAt(event->pos()));
        // if i clicked on a picture...
        if (labelChild)
            // ...then my parent will be drawobject hopefully
            child = dynamic_cast<DrawObject *>(labelChild->parent());

        // If it is still 0, i clicked on the meshfield itself
        if (!child) {
            emit onWidgetClicked(-1);
            // return if no child at mouse position
            return;
        }

        // pressed on a child
        // set focus on it
        emit onWidgetClicked(child->ID);
        // relative point of mouse to child
        QPoint hotSpot = event->pos() - child->pos();
        QByteArray arrayData;
        QDataStream dataStream(&arrayData, QIODevice::WriteOnly);

        // stream data into the datastream. these data can be restored later in the
        // drop event
        // data Stream
        dataStream << QPoint(hotSpot) << child->ID << event->pos();

        QMimeData *mimeData = new QMimeData;
        mimeData->setData("ANISE-GUI/drawobject", arrayData);

        // hides the child so only the drag object at mouse position is shown
        child->hide();

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(child->getPicture());
        drag->setHotSpot(hotSpot);

        if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) ==
                Qt::MoveAction)
            qDebug() << "never happens? IF it does check mesheditorwidget";

        else {
            child->show();
        }
    }
}

void MeshEditorWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("ANISE-GUI/drawobject")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void MeshEditorWidget::mouseMoveEvent(QMouseEvent *event) {
    // update mouse position
    this->mousePositionOld = this->mousePosition;
    this->mousePosition = event->pos();

    // calculate how far the mouse moved
    mouseMoveDistance += abs(((mousePosition.x() - mousePositionOld.x()) ^ 2) +
                             ((mousePosition.y() - mousePositionOld.y()) ^ 2));

    if (newLine.drawLine && mouseMoveDistance >= 10) {
        mouseMoveDistance = 0;
        this->repaint();
    }
}

void MeshEditorWidget::dragMoveEvent(QDragMoveEvent *event) {
    // update mouse position
    this->mousePosition = event->pos();
    if (newLine.drawLine) {
        this->repaint();
    }
}

void MeshEditorWidget::dropEvent(QDropEvent *event) {
    QPoint DropPoint = event->pos();
    QPoint offset;
    int nodeID;

    QByteArray arrayData = event->mimeData()->data("ANISE-GUI/drawobject");
    QDataStream dataStream(&arrayData, QIODevice::ReadOnly);

    // get the offset out of the mime, the offset is the distance from the mouse
    // to the 0,0 position of the label/widget
    dataStream >> offset;

    // the id of the dragged object
    dataStream >> nodeID;

    DropPoint -= offset;

    if (event->source()->objectName() == "nodeCatalogContent") {
        // should create the correct node type and not a "testnode"
        QString _class;
        dataStream >> _class;

        Node *newNode = Data::instance()->nodeFactory->createNode(_class);

        newNode->setName(_class);
        newNode->moveTo(DropPoint.x(), DropPoint.y());
        clearNewLine();
        Data::instance()->addNodeToMesh(newNode);
    }

    else if (event->source()->objectName() == "meshField") {
        // get the start posisiotn of the drop from the mime
        // TODO die start position wird jetzt nur aus dem datastream gelesen wenn
        // wir innerhalb vom meshfield draggen. aber man sollte dafür das
        // application/datathingy benutzen
        QPoint dropstart;
        dataStream >> dropstart;

        // will move the correct node to the new position
        Data::instance()->moveObjectInMesh(&dropstart, &DropPoint, nodeID);
    }
}

void MeshEditorWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    if (newLine.drawLine == true) {
        // this will draw the vector with points as a line
        SingletonRender::instance()->drawLines(&newLine.wayPoints, &mousePosition);
    }
    // draw all connections:
    SingletonRender::instance()->renderConnections();
}

bool MeshEditorWidget::handleGateClick(int nodeID, QString gateName,
                                       QPoint position) {
    // stop connection creation when clicked on an outgate of the source node
    if (nodeID == newLine.srcNodeID && gateName == newLine.srcGateName) {
        this->resetToEditMode();
        return false;
    }

    // Do nothing if you clicked on wrong Gate
    else if (!correctGate(nodeID, gateName)) {
        resetToEditMode();
        return false;
    }

    // That means we are currently constructing a line
    if (newLine.drawLine) {
        newLine.destNodeID = nodeID;
        newLine.destGateName = gateName;
        // delete first wayPoint -> later on it is always calculated when connection
        // is going to be drawn
        newLine.wayPoints.pop_front();
        // call Datastuff to create Connection
        // do this if connection is established
        Data::instance()->addConnectionToMesh(
                    NodeFactory::createConnection(newLine.srcNodeID, newLine.srcGateName,
                                                  newLine.destNodeID, newLine.destGateName,
                                                  newLine.wayPoints));
        this->resetToEditMode();

    }

    // That means we are just starting a new Line
    else {
        Node *node = Data::instance()->getNodeByID(nodeID);
        Gate *srcGate;

        if (!node) return false;

        srcGate = node->getGateByName(gateName);

        if (!srcGate) return false;

        newLine.srcNodeID = nodeID;
        newLine.srcGateName = gateName;
        // this is the OutputGatePosition where Connection starts
        newLine.wayPoints << (position + SingletonRender::instance()->getOutputGateDrawOffset());

        // Do Everything that Changed when Clicking on a gate
        newLine.drawLine = !(newLine.drawLine);

        Data::instance()->setDrawLineMode(srcGate->getType());

        emit drawLineModeChanged();
    }
    return true;
}

void MeshEditorWidget::changeLineDrawMode() {
    if (newLine.drawLine) {
        setCursor(Qt::CrossCursor);
        Data::instance()->getMainWindow()->ui->statusBar->showMessage(
                    "exit line draw mode with right click", 5e3);
        QToolTip::showText(
                    cursor().pos(), QString("exit line draw mode with right click"), this,
                    Data::instance()->getMainWindow()->ui->mesh_edt_area->rect(), 4000);
    } else
        setCursor(Qt::ArrowCursor);
}

bool MeshEditorWidget::correctGate(int nodeID, QString gateName) {
    Node *node = Data::instance()->getNodeByID(nodeID);
    Gate *endGate;

    if (!node) return false;

    endGate = node->getGateByName(gateName);

    if (!endGate) return false;

    if (newLine.drawLine) {
        Gate *srcGate =
                Data::instance()->getNodeByID(newLine.srcNodeID)->getGateByName(
                    newLine.srcGateName);
        return endGate->getDirection() &&
                srcGate->getType() == endGate->getType() &&
                newLine.srcNodeID != nodeID;
    }
    else {
        return !endGate->getDirection();
    }
}
