#include "nodetypelabel.h"
#include "data.h"
NodeTypeLabel::NodeTypeLabel(QWidget *parent) :
    QLabel(parent)
{
}
QString NodeTypeLabel::getType() const
{
    return type;
}

void NodeTypeLabel::setType(const QString &value)
{
    type = value;
}

void NodeTypeLabel::mousePressEvent(QMouseEvent *ev){
    QLabel::mousePressEvent(ev);
    Data::instance()->getMainWindow()->displayTypeInfo(type);
}

QString NodeTypeLabel::getCategory() const
{
    return category;
}

void NodeTypeLabel::setCategory(const QString &value)
{
    category = value;
}
