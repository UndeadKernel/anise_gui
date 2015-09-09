#include <QMessageBox>
#include "drawobject.h"
#include <QShortcut>
#include <QSize>
#include <QDebug>
#include <QStyleOption>
#include <QListWidget>
#include <QObject>
#include <QRegion>
#include "gatebutton.h"
#include "mesheditorwidget.h"
#include "data.h"
#include "propertywidget.h"
#include "parseerrorbox.h"
#include "node.h"
#include <QSignalMapper>

DrawObject::DrawObject(int id, QPoint position, int width, int height,
                       QWidget *parent, int highlightOffset) {
    this->ID = id;
    this->status = Node::edit;
    this->setParent(parent);
    this->highlightWidth = highlightOffset;
    this->nameLabel = 0;
    this->progressBar = 0;
    this->progressButton[3] = {0};
      // We say the constructor which position he has
    this->setGeometry(position.x(), position.y(), width + 2 * highlightOffset,
                      height + 2 * highlightOffset);

    // initialize mainMask with complete Transparency
    mainMaskUnhighlighted = QBitmap(this->size());
    mainMaskUnhighlighted.fill(Qt::color0);

    // initialize the over allRange picture
    this->overAllPicture = QPixmap(this->size());

    // make it transparent
    overAllPicture.fill(QColor(0, 0, 0, 0));
    // set backgroundcolor
    this->setStyleSheet("background-color:yellow;");

    mainMaskUnhighlighted.fill(Qt::color0);
    this->setMask(mainMaskUnhighlighted);

    mainMaskAsImage = mainMaskUnhighlighted.toImage();

    // connect release event
    connect(this, SIGNAL(released(int, QString, QPoint)), this->parent(),
            SLOT(handleGateClick(int, QString, QPoint)));
}

DrawObject::~DrawObject()
{
    if(nameLabel)
        delete nameLabel;
    if(progressBar)
        delete progressBar;
}

/**
 * Has two parameter for the widget.
 * We create label object.
 * we give the label the pixmap and the geometry.
 * Then we show it.
 *
 * @brief TestDrawObject::addPicture
 * @param pic       picture which you want to add
 * @param position  position relativ to top left corner
 */
void DrawObject::addPicture(QPixmap *pic, QPoint position) {
    // safe dimensions
    int height = pic->height();
    int width = pic->width();

    // set transparency to magic pink
    pic->setMask(pic->createMaskFromColor(Qt::magenta));

    // take respect to highlightWidth
    position += QPoint(highlightWidth, highlightWidth);

    // Instanstiate the drawing to be shown
    QLabel *label = new QLabel(this);
    label->setGeometry(position.x(), position.y(), width, height);
    label->setPixmap(*pic);
    this->labelvector << label;
    // Now set the masks to the widgets
    label->setMask(pic->mask());

    // modifies the mainMask inserting the mask of the pic
    this->modifyMask(pic, position);

    // update the over all picture
    this->updateOverAllPicture(pic, position);
}

void DrawObject::nodeNameChanged(QListWidgetItem *itemChanged) {
    itemChanged->setSelected(false);
    Node *node = Data::instance()->getNodeByID(this->ID);

    if (node->getName() != itemChanged->text())
        node->setName(itemChanged->text());

    nameLabel->setStyleSheet("background: transparent;");
}

void DrawObject::setNodeName(QString const &nodeName) {
    if (!nameLabel) {
        nameLabel =
                new QListWidget(dynamic_cast<MeshEditorWidget *>(this->parent()));
        QListWidgetItem *item = new QListWidgetItem(nameLabel);

        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        nameLabel->setFrameStyle(QFrame::NoFrame);
        nameLabel->setStyleSheet("background: transparent;");

        item->setText(nodeName);
        nameLabel->insertItem(0, item);

        int posX = this->pos().x() + this->width() / 2 - nameLabel->width() / 2;
        int posY = this->pos().y() + this->height();
        nameLabel->setGeometry(posX, posY, this->width() * 2, 20);
        connect(nameLabel, SIGNAL(itemChanged(QListWidgetItem *)), this,
                SLOT(nodeNameChanged(QListWidgetItem *)));
        connect(nameLabel, SIGNAL(itemClicked(QListWidgetItem *)), this,
                SLOT(restrictOneClickOneItem(QListWidgetItem *)));
        connect(nameLabel, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this,
                SLOT(deleteItemText(QListWidgetItem *)));
    }
    nameLabel->itemAt(1, 1)->setText(nodeName);
    nameLabel->show();
}

void DrawObject::restrictOneClickOneItem(QListWidgetItem *itemClicked) {
    itemClicked->setSelected(false);
}


void DrawObject::addGateButton(QPixmap *pic, QPoint position, QString gateName,
                               QString gateType, bool direction) {
    // safe dimensions
    int height = pic->height();
    int width = pic->width();

    // set transparency to magic pink
    pic->setMask(pic->createMaskFromColor(Qt::magenta));

    // take respect to highlightWidth
    position += QPoint(highlightWidth, highlightWidth);

    // Instanstiate the drawing to be shown
    GateButton *button = new GateButton(gateName, gateType, ID, direction, this);
    button->setGeometry(position.x(), position.y(), width, height);
    button->setIcon(*pic);
    this->gateVector << button;
    // Now set the masks to the widgets
    button->setMask(pic->mask());

    // modifies the mainMask inserting the mask of the pic
    this->modifyMask(pic, position);

    // update the over all picture
    this->updateOverAllPicture(pic, position);

    // Connect button signal to appropriate slot
    connect(button, SIGNAL(released(QString, QPoint)), this,
            SLOT(releasedOnGate(QString, QPoint)));
}

QPoint DrawObject::getGatePosition(QString gateName) {
    QPoint gatePosition(-1, -1);

    foreach (GateButton *gate, gateVector) {
        if (gate->getGateName() == gateName) {
            gatePosition = gate->pos();
            break;
        }
    }
    return gatePosition;
}

void DrawObject::modifyMask(QPixmap *pic, QPoint const &position,
                            const bool &updateMask) {
    // safe dimensions
    int height = pic->height();
    int width = pic->width();

    // Instanstiate the non-transparent mask with size of pic
    QBitmap picMask = pic->mask();  // This is used to get the mask automatically
    // from the picture but doesnt work all the
    // time

    // If there is no nontransparency, create nontransparentmask manually (no
    // other option so far)
    if (picMask.size().isEmpty()) {
        picMask = QBitmap(width, height);
        picMask.fill(Qt::color1);
    }

    // Include the picture (non-transparent) to the main Mask to get the area of
    // the picture to be visible
    QPainter painter(&mainMaskUnhighlighted);
    painter.setBrush(Qt::color1);
    painter.drawPixmap(position.x(), position.y(), width, height, picMask);

    // update changes to QImage
    mainMaskAsImage =
            mainMaskUnhighlighted.toImage().convertToFormat(QImage::Format_Mono);

    if (updateMask) highlightMask();

    // Now set the mask of the widget
    this->setMask(mainMaskUnhighlighted);
}

void DrawObject::updateOverAllPicture(QPixmap *newPicture,
                                      const QPoint &position) {
    // save the dimension of the new picture
    int height = newPicture->height();
    int width = newPicture->width();

    // create a painter
    QPainter painter(&(this->overAllPicture));
    painter.setBrush(Qt::black);
    painter.drawPixmap(position.x(), position.y(), width, height, *newPicture);
}

QPixmap DrawObject::getPicture() { return this->overAllPicture; }

void DrawObject::dehighlight() { this->setMask(mainMaskUnhighlighted); }

void DrawObject::highlight() { this->setMask(mainMaskHighlighted); }

void DrawObject::releasedOnGate(QString gateName, QPoint position) {
    emit released(this->ID, gateName, this->pos() + position);
}

int DrawObject::getHighlightWidth() const { return highlightWidth; }

void DrawObject::highlightMask() {
    int width = this->width();
    int height = this->height();

    // this highlights left and right side
    for (int y = 0; y < height; y++) {
        for (int x = highlightWidth; x < width - highlightWidth; x++) {
            // left side highlight
            if (getPixel(x, y) && !getPixel(x - 1, y)) {
                for (int highlightOffset = 0; highlightOffset < highlightWidth;
                     highlightOffset++)
                    setPixel(x - highlightOffset - 1, y, 1);
            }

            // the right side of drawobject to highlighting
            if (getPixel(x, y) && !getPixel(x + 1, y)) {
                for (int highlightOffset = 0; highlightOffset < highlightWidth;
                     highlightOffset++)
                    setPixel(x + highlightOffset + 1, y, 1);
                x += 4;
            }
        }
    }
    // this highlight upside and downside
    for (int x = 0; x < width; x++) {
        for (int y = highlightWidth; y < height - highlightWidth; y++) {
            // highlights the upside of drawobject
            if (getPixel(x, y) && !getPixel(x, y - 1)) {
                for (int highlightOffset = 0; highlightOffset < highlightWidth;
                     highlightOffset++)
                    setPixel(x, y - highlightOffset - 1, 1);
            }

            // highlights the downside of drawobject
            if (getPixel(x, y) && !getPixel(x, y + 1)) {
                for (int highlightOffset = 0; highlightOffset < highlightWidth;
                     highlightOffset++)
                    setPixel(x, y + highlightOffset + 1, 1);
                y += 4;
            }
        }
    }
    mainMaskHighlighted = QBitmap::fromImage(mainMaskAsImage);
}

int DrawObject::getPixel(const int &x, const int &y) const {
    const uchar mask = 0x80 >> (x % 8);
    int pixel = mainMaskAsImage.scanLine(y)[x / 8] & mask ? 1 : 0;
    return pixel;
}

void DrawObject::setPixel(const int &x, const int &y, const int &pixel) {
    const uchar mask = 0x80 >> (x % 8);
    if (pixel)
        mainMaskAsImage.scanLine(y)[x / 8] |= mask;
    else
        mainMaskAsImage.scanLine(y)[x / 8] &= ~mask;
}

void DrawObject::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void DrawObject::highlightGates(QString const &gateType) {
    foreach (GateButton *gate, gateVector) {
        if (gate->getDirection() && gate->getGateType() == gateType)
            gate->setHighlightMode(true);
        else
            gate->setHighlightMode(false);
    }
}

void DrawObject::dehighlightGates() {
    foreach (GateButton *gate, gateVector) { gate->resetPicture(); }
}

void DrawObject::move(const int &x, const int &y) {
    QWidget::move(x, y);
    if (progressBar)
        progressBar->move(this->pos().x(), this->pos().y() + this->height() + 20);
    if (!nameLabel) return;
    int nodeNameX = this->pos().x() + this->width() / 2 - nameLabel->width() / 2;
    int nodeNameY = this->pos().y() + this->height();
    nameLabel->move(nodeNameX, nodeNameY);
}

void DrawObject::hide() {
    QWidget::hide();

    if (progressBar) progressBar->hide();

    if (!nameLabel) return;
    nameLabel->hide();
}

void DrawObject::show() {
    QWidget::show();

    if (progressBar) {
        if (Data::instance()->isRunning()){
              progressBar->show();
        }
        else
            progressBar->hide();
    }
    if (!nameLabel) return;
    nameLabel->show();
}

void DrawObject::deleteLater() {
    QObject::deleteLater();
    progressBar->deleteLater();
    if (!nameLabel) return;
    nameLabel->deleteLater();
}


void DrawObject::initializeProgressButton(){
    int posX = this->pos().x()+34;
    int posY = this->pos().y() +5;
    QSignalMapper *mapper=new QSignalMapper(this);
    QObject::connect(mapper, SIGNAL(mapped(int)),this, SLOT(progressButtonClicked(int)));
    char* filename="./data/images/";
    for(int i=0;i<3;i++){
          progressButton[i]=new QPushButton(dynamic_cast<QWidget *>(this->parent()));
          progressButton[i]->setGeometry(posX, posY, this->width()/5, 12);
          progressButton[i]->setStyleSheet({"border: none;"});
         QString filename1=filename;
        filename1.append(QString::number(i)).append(".png");
        qDebug()<<filename1;
          QPixmap pix(filename1);
          QIcon icon(pix);
          progressButton[i]->setIcon(icon);
          progressButton[i]->setIconSize(pix.size());
          progressButton[i]->update();
           posY+=15;
          progressButton[i]->hide();
            mapper->setMapping(progressButton[i],i);
           QObject::connect(progressButton[i], SIGNAL(clicked()),mapper, SLOT(map()));
    }
}

void DrawObject::progressButtonShow(int i)
{
    if(i){
        progressButton[i-1]->show();
    }
}

void DrawObject::progressButtonClicked(int i)
{

    ParseErrorBox box;
    QString logMessage, logWarning, logError;
Node *n=Data::instance()->getNodeByID(this->ID);
    box.setWindowTitle(n->getName());
    switch(i){
        case 0:box.setHeader("Information");
               for(QString line:n->getLogMessage() ){
                   logMessage.append(line);
                   logMessage.append("\n");
               }
               box.setErrorText(logMessage);
        break;
        case 1:box.setHeader("Warning");
               for(QString line:n->getLogWarning() ){
               logWarning.append(line);
               logWarning.append("\n");
               }
               box.setErrorText(logWarning);
        break;
        case 2:box.setHeader("Error");
               for(QString line:n->getLogError() ){
               logError.append(line);
               logError.append("\n");
               }
               box.setErrorText(logError);
        break;
    }
    box.exec();

}

void DrawObject::initializeProgressView() {
    progressBar =
            new QProgressBar(dynamic_cast<MeshEditorWidget *>(this->parent()));
    int posX = this->pos().x();
    int posY = this->pos().y() + this->height() + ((nameLabel) ? nameLabel->height() : 20);

    progressBar->setGeometry(posX, posY, this->width(), 15);
    progressBar->setOrientation(Qt::Horizontal);
    progressBar->setRange(0, 0);
    progressBar->setValue(0);
    progressBar->hide();
}

void DrawObject::changeProgressView() {
    if (Data::instance()->isRunning())
        setProgressView();

    else
        setEditView();
}

void DrawObject::setEditView() {
    progressBar->hide();
    this->dehighlight();
    this->setStyleSheet("background-color:yellow;");
}

void DrawObject::setProgressView() {
    this->setStatus(Node::initializing);
    this->highlight();
    progressBar->show();
}

void DrawObject::mouseDoubleClickEvent(QMouseEvent *e)
{
    if(!Data::instance()->getNodeByID(ID))
        return;
    QWidget::mouseDoubleClickEvent(e);
    if(status == Node::error)
        QMessageBox::critical(this,"Errors on Simulation", "Errors:\n" + Data::instance()->getNodeByID(ID)->getErrorMsg());
    else if(status == Node::edit){
        try{
        PropertyWidget *p = new PropertyWidget(ID);
        p->show();
        p->move(100,100);
        }
        catch(...){
        }

        //p->deleteLater();
    }

}

void DrawObject::deleteItemText(QListWidgetItem *item) {
    Q_UNUSED(item);
    nameLabel->setStyleSheet(
                "QListWidget::item:selected{background: transparent}");
}

void DrawObject::setStatus(const Node::Status &status) {
    this->status = status;
    for(int i=0;i<3;i++){
    this->progressButton[i]->toolTip()=status;}
    switch (status) {
    case Node::edit:
        this->setStyleSheet("background-color:yellow;");
        break;
    case Node::initializing:
        this->setStyleSheet("background-color:grey;");
        progressBar->show();
        break;
    case Node::idle:
        this->setStyleSheet("background-color:#00ff00;");
        progressBar->hide();
        break;
    case Node::processing:
        this->setStyleSheet("background-color:#0000ff;");
        progressBar->show();
        break;
    case Node::error:
        this->setStyleSheet("background-color:#ff0000;");
        break;
    default:
        this->setStyleSheet("background-color:yellow;");
        break;
    }
}

void DrawObject::setProgressValue(int value) {
    if (progressBar) {
        if (value == -1) {
            progressBar->setMaximum(0);
            progressBar->setValue(0);
        } else {
            progressBar->setMaximum(100);
            progressBar->setValue(value);
        }
    }
}
