#include "data.h"
#include "node.h"
#include <assert.h>

Node::Node() {
    inputGates = QVector<Gate *>();
    outputGates = QVector<Gate *>();
    params = QMap<QString, Parameter>();
    type = QString();
    name = QString();
    status = idle;
    progress = 0;
    logMessage = QStringList();
    logWarning = QStringList();
    logError = QStringList();
}

QMap<QString, Node::Parameter> *Node::getParams() { return &this->params; }

Node::Node(QVector<Gate *> &inputGates, QVector<Gate *> &outputGates,
           QString &type, QString &name, QMap<QString, Parameter> &params){
    this->inputGates = inputGates;
    this->outputGates = outputGates;
    this->type = type;
    this->params = params;
    this->name = name;
    this->status = idle;
    progress = 0;
}

void Node::moveTo(float x, float y) {
    QPoint offset = QPoint(x,y) - QPoint(position_x, position_y);
    this->position_x = x;
    this->position_y = y;
    Data::instance()->setChanged();
    SingletonRender::instance()->updateConnections(ID, offset);
}

void Node::setType(const QString &type) { this->type = type; }

void Node::setName(const QString &name) {
    this->name = Data::instance()->getValidAlternativeForName(name);
    Data::instance()->setChanged();
    SingletonRender::instance()->setNodeName(ID, this->name);
}

QVector<Gate *> *Node::getInputGates() { return &this->inputGates; }

QVector<Gate *> *Node::getOutputGates() { return &this->outputGates; }

QString Node::getName() { return this->name; }

QString Node::getType() { return this->type; }

void Node::addGate(Gate *gate) {
    if (gate->getDirection() == true) {
        this->inputGates << gate;
    } else {
        this->outputGates << gate;
    }
}

void Node::addGates(const QVector<Gate *> &gates, const bool &direction) {
    if (direction == true) foreach (Gate *gate, gates) {
        inputGates << gate;
    }
    else
        foreach (Gate *gate, gates) { outputGates << gate; }
}

/**
 * @brief Node::addParam add a new Parameter to the Node
 * @param descr description
 * @param _key internal key
 * @param name displayed name
 * @param type data type
 * @param _value value to be stored
 * @return true if the parameter was added, false elsewise
 */
bool Node::addParam(const QString &descr, const QString &_key, const QString &name, const QString &type,
                    const QVariant &_value) {
    if (!this->params.contains(_key)) {
        Parameter p{descr, _key, name, type, _value};
        this->params.insert(_key, p);
    } else
        return false;
    return true;
}
QString Node::getErrorMsg() const
{
    return errorMsg;
}

void Node::setErrorMsg(const QString &value)
{
    errorMsg = value;
}

void Node::addErrorMsg(const QString &value)
{
    errorMsg += value;
}

bool Node::hasGate(const QString &gateName)
{
    for(Gate *g : inputGates)
        if(g->getName() == gateName)
            return true;
    for(Gate *g : outputGates)
        if(g->getName() == gateName)
            return true;
    return false;
}

int Node::getProgress() const
{
    return progress;
}

void Node::setProgress(int value)
{
    assert((value >= 0 && value <= 100) || value == -1);
    progress = value;
    SingletonRender::instance()->setPercentage(ID,progress);
}

Node::Status Node::getStatus() const
{
    return status;
}

void Node::setStatus(const Node::Status &value)
{
    status = value;
    SingletonRender::instance()->setStatusColor(ID, status);
}

/**
 * @brief Node::setParam
 * @param key key of parameter to be changed
 * @param _value new value
 * @return true on success, false elsewise
 */
bool Node::setParam(QString key, QVariant _value) {
    if (params.contains(key)) {
        params[key].value = _value;
        Data::instance()->setChanged();
        return true;
    }
    Parameter p;
    p.descr = "unknown parameter";
    p.type = _value.typeName();
    p.key = key;
    p.value = _value;
    p.name = key;
    params[key] = p;

    return false;
}

/**
 * @brief Node::removeParam
 * @param _key key of parameter to remove
 * @return true on success
 */
bool Node::removeParam(QString _key) {
    if (this->params.contains(_key))
        this->params.remove(_key);
    else
        return false;
    return true;
}

Node::Parameter Node::getParamStructByKey(const QString &key) {
    if (params.contains(key))
        return params[key];
    else
        return Parameter{QString(), QString(), QString(), QString(), QVariant()};
}

QVariant Node::getParamByKey(const QString &_key) {
    if (!this->params.contains(_key)) return QString("@@invalid@@");
    return params[_key].value;
}

QString Node::toString() {
    QString out = "";
    out += "class:" + type + "\nname:" + name + "\n\nparameters:\n";
    foreach (QString key, params.keys()) {
        out += "key:" + key + ", " + "value: " + params[key].value.typeName() +
                " " + params[key].value.toString() + "\n";
    }
    out += "position: (" + QString::number(position_x) + "|" +
            QString::number(position_y);
    return out;
}

Gate *Node::getGateByName(const QString &name) {
    foreach (Gate *gate, inputGates + outputGates)
        if (gate->getName() == name) return gate;

    return 0;
}

void Node::addParam(QString key, Node::Parameter p) { params[key] = p; }

float Node::x() const { return position_x; }

float Node::y() const { return position_y; }

void Node::setX(const float &newX) { position_x = newX; }

void Node::setY(const float &newY) { position_y = newY; }

int Node::getID() const { return ID; }

void Node::setID(const int &ID) { this->ID = ID; }

QPoint Node::getPosition() {
    return QPoint(this->position_x, this->position_y);
}

QPoint Node::getGatePosition(QString gateName) {
    QPoint result;
    int gateHeight = SingletonRender::instance()->getImage("gate.png")->height();
    int gateWidth = SingletonRender::instance()->getImage("gate.png")->width();
    int drawObjectWidth = 3 * 16 + 2 * gateWidth;
    int gateOffset = 10;
    int counter = 0;
    foreach (Gate *gate, outputGates) {
        qDebug() << gate->getName();
        if (gate->getName() == gateName) {
            result = (QPoint(drawObjectWidth,
                             counter * (gateHeight + gateOffset) + 5 + gateHeight));
            result.operator+=(this->getPosition());
            qDebug() << result.x() << ";" << result.y();
            return result;
        }
        counter++;
    }
    counter = 0;
    foreach (Gate *gate, inputGates) {
        if (gate->getName() == gateName) {
            result =
                    (QPoint(0, counter * (gateHeight + gateOffset) + 5 + gateHeight));
            result.operator+=(this->getPosition());
            return result;
        }
        counter++;
    }

    // best guess is the position of the node if no gate is found
    return this->getPosition();
}

QString Node::getDescription() { return description; }

void Node::setDescription(const QString &value) { description = value; }

QStringList Node::getLogMessage() const{
    return logMessage;
}

void Node::setLogMessage(const QStringList &value){
    logMessage = value;
    SingletonRender::instance()->showProgressButtons(ID,1);
}

void Node::addLogMessage(const QString &value){
    logMessage += value;
    SingletonRender::instance()->showProgressButtons(ID,1);
}

QStringList Node::getLogWarning() const{
    return logWarning;
}

void Node::setLogWarning(const QStringList &value){
    logWarning = value;
    SingletonRender::instance()->showProgressButtons(ID,2);
}

void Node::addLogWarning(const QString &value){
    logWarning += value;
    SingletonRender::instance()->showProgressButtons(ID,2);
}

QStringList Node::getLogError() const{
    return logError;
}

void Node::setLogError(const QStringList &value){
    logError = value;
    SingletonRender::instance()->showProgressButtons(ID,3);
}

void Node::addLogError(const QString &value){
    logError += value;
    SingletonRender::instance()->showProgressButtons(ID,3);
}
