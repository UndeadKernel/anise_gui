#ifndef DRAWOBJECT_H
#define DRAWOBJECT_H

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QVector>
#include <QBitmap>
#include <QPushButton>
#include <QListWidgetItem>
#include <QProgressBar>
#include <node.h>

#include "gatebutton.h"

class DrawObject : public QWidget {
    Q_OBJECT

public:
    int ID;
    QVector<QLabel *> labelvector;

    DrawObject(int id, QPoint position, int width, int height,
               QWidget *parent = 0, int highlightWidth = 3);

    QVector<GateButton *> gateVector;
    ~DrawObject();
    QBitmap mainMaskUnhighlighted;
    QBitmap mainMaskHighlighted;
    QImage mainMaskAsImage;
    QPainter painter;
    QPixmap overAllPicture;

    void addPicture(QPixmap *pic, QPoint position);
    void addGateButton(QPixmap *pic, QPoint position, QString gateName,
                       QString gateType, bool direction);
    void addProgressButton(QPixmap *pic, QPoint position, QString gateName,
                       QString gateType, bool direction);

void setProgressButtonOver();
QPoint getGatePosition(QString gateName);

    // Modify the mask of the drawobject inserting the mask of the pixmap at given
    // position
    void modifyMask(QPixmap *pic, const QPoint &position, const bool &updateMask = true);

    QPixmap getPicture();

    void dehighlight();
    void highlight();
    void paintEvent(QPaintEvent *);
    int getHighlightWidth() const;

    void dehighlightGates();
    void highlightGates(const QString &gateType);

    void setNodeName(const QString &nodeName);

    void move(int const &x, int const &y);
    void moveProgressButton(const int &x, const int &y);
    void hide();
    void show();
    void deleteLater();

    void setProgressValue(int value);
    void initializeProgressView();
    void initializeProgressButton();
    void progressButtonShow(int i);

    void changeProgressView();
    void setStatus(const Node::Status &status);
signals:

    void released(int nodeID, QString gateName, QPoint position);

public slots:
    void releasedOnGate(QString gateName, QPoint position);
    void nodeNameChanged(QListWidgetItem *itemChanged);
    void progressButtonClicked(int i);

private slots:
    void restrictOneClickOneItem(QListWidgetItem *itemClicked);
    void deleteItemText(QListWidgetItem *item);

private:

    //Stuff for progess

    QProgressBar *progressBar;
    QPushButton *progressButton[3];

    bool progressMode;

    void setProgressView();
    Node::Status status;
    //rest
    int highlightWidth;
    QListWidget *nameLabel;
    void mouseDoubleClickEvent(QMouseEvent *e);
    void updateOverAllPicture(QPixmap *newPicture, QPoint const &position);
    int getPixel(const int &x, const int &y) const;
    void setPixel(const int &x, const int &y, const int &pixel);
    void highlightMask();
    void setEditView();
};

#endif  // DRAWOBJECT_H
