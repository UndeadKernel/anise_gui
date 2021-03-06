#include "nodecatalog.h"
#include <QDebug>

NodeCatalog::NodeCatalog() {
    Content = QMap<QString, Node>();
    /*Node blackbox;
    Gate *in = new Gate(true, "in"), *out = new Gate(false, "out");
    blackbox.addGate(in);
    blackbox.addGate(out);
    blackbox.setType("blackbox");
    blackbox.setDescription("A Blackbox Node for all unknown Nodes");
    blackbox.addParam("boolean", true);
    blackbox.addParam("boolean2" ,false);
    blackbox.addParam("integer", -5);
    blackbox.addParam("String", "Test String");
    Content["blackbox"] = blackbox;*/
}

void NodeCatalog::clearContent()
{
    Content.clear();
}

QMap<QString, Node> NodeCatalog::getContent()
{
    return Content;
}

QVector<Node> NodeCatalog::getContentVector()
{
    return Content.values().toVector();
}

QList<Node> NodeCatalog::getContentList()
{
    return Content.values();
}


void NodeCatalog::insert(Node nodeToInsert) {
    QString type = nodeToInsert.getType();
    this->Content[type] = nodeToInsert;
}

Node NodeCatalog::getNodeOfType(const QString &type){
    Node node;
    QString _class = type;
    node = Content[_class];

    return node;
}

bool NodeCatalog::contains(const QString &_class){
    return Content.contains(_class);
}

const Node *NodeCatalog::getPointerOfType(const QString &type)
{
    Node *n = 0;
    if(Content.contains(type))
        n = &Content[type];
    return n;
}
