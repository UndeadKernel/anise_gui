#include "jsonfilehandler.h"
#include "data.h"
#include <bitset>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include "parseerrorbox.h"
#include <QDateTime>
bool JsonFileHandler::parsing = false;

/**
 * @brief JsonFileHandler::readFile converts a textfile into a
 * QJsonDocument-Object
 * @param path contains path to a text file in json format
 * @return filecontent as QJsonObject
 */
QJsonObject *JsonFileHandler::readFile(const QString &path) {
    // read the file
    QJsonParseError jerror;
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QString content = file.readAll();
    QJsonDocument jdoc = QJsonDocument::fromJson(content.toUtf8(), &jerror);
    file.close();

    // catch error while parsing
    if (jerror.error != QJsonParseError::NoError) {
        QString errorString = jerror.errorString();
        int line = 1;
        int point = 0;
        for(int i = 0; i < jerror.offset; i++){
            if(content[i] == '\n'){
                line++;
                point = 0;
            }
            else point++;
        }
        qWarning() << jerror.errorString();
        QMessageBox::critical(Data::instance()->getMainWindow(),
                              "Error while Parsing", errorString + "\n Line " + QString::number(line) + ":" + QString::number(point));
        return 0;
    }

    // no error
    QJsonObject *obj = new QJsonObject(jdoc.object());
    return obj;
}

/**
 * @brief JsonFileHandler::parseNodeTypesFromAnise fills the nodeCatalog with
 * Node Templates
 * @param output the output of the framework with parameters "--nodes --machine"
 */
void JsonFileHandler::parseNodeTypesFromAnise(QString &output) {
    if (output == "") return;

    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());

    NodeCatalog *catalog = Data::instance()->getNodeCatalog();
    qDebug() << "trying to read all node types. Loading json data...\n";

    QJsonObject obj = doc.object();

    if (!obj["nodes"].isArray()) {
        qWarning() << "no nodes in JSON-Object";
        return;
    }

    foreach (QJsonValue var, obj["nodes"].toArray()) {
        QString type, descr;
        QList<Gate *> input_gates, output_gates;

        QJsonObject localNode = var.toObject(), json_params;
        type = localNode["class"].toString();
        descr = localNode["description"].toString();
        QJsonArray inputs = localNode["input_gates"].toArray(),
                outputs = localNode["output_gates"].toArray();

        if (!inputs.isEmpty()) foreach (QJsonValue value, inputs) {
            QJsonObject localGate = value.toObject();
            QString gateName = localGate["name"].toString(),
                    gateType = localGate["type"].toString();
            Gate *gate = new Gate();
            gate->setDirection(true);
            gate->setName(gateName);
            gate->addType(gateType);
            input_gates << gate;
        }
        if (!outputs.isEmpty()) foreach (QJsonValue value, outputs) {
            QJsonObject localGate = value.toObject();
            QString gateName = localGate["name"].toString(),
                    gateType = localGate["type"].toString();
            Gate *gate = new Gate();
            gate->setDirection(false);
            gate->setName(gateName);
            gate->addType(gateType);
            output_gates << gate;
        }
        Node node;
        node.setType(type);
        node.setDescription(descr);
        node.addGates(input_gates.toVector(), true);
        node.addGates(output_gates.toVector(), false);

        // initilize parameters
        json_params = localNode["parameters"].toObject();
        QJsonArray contents = localNode["parameters"].toArray();
        for (QJsonValue o : contents) {
            QVariantMap parameters = o.toObject().toVariantMap();
            QVariant value(
                        QVariant::nameToType(parameters["type"].toString().toUtf8()));
            value = parameters["default"];
            node.addParam(parameters["description"].toString(),
                    parameters["key"].toString(), parameters["name"].toString(),
                    parameters["type"].toString(), value);
            node.setParam(parameters["key"].toString(), parameters["default"]);
        }
        catalog->insert(node);
    }
    Data::instance()->unsetChanged();
}

/**
 * @brief JsonFileHandler::extractNodesAndConnections Extracts all Nodes and
 * Connections of a QJsonObject
 * @param obj the QJsonObject containing the JSON File
 * @param connectionlist List in which the connections will be written
 */
bool JsonFileHandler::extractNodesAndConnections(const QJsonObject &obj) {
    if(!&obj)
        return false;
    parsing = true;
    QString nodeErrors = "", connectionErrors = "";
    bool hasPositionData = true;
    // error flags: u|c|nc|n|nn
    // u: unknown node found
    // c: error in connection
    // nc: no connections
    // n: error in node
    // nn: no nodes
    std::bitset<5> flags = 0;
    // check if there are any nodes
    int i = 1, j = 1;  // for debug only
    if (!obj["nodes"].isArray()) {
        qWarning() << "no nodes in JSON-Object";
        flags |= 0b0001;
        goto end;
    }

    // for every node (represented as jsonvalue)...
    foreach (QJsonValue var, obj["nodes"].toArray()) {
        //...convert it to json object...
        QJsonObject theNode = var.toObject();

        // check if nodes are declared correctly
        if (!(theNode["class"].isString() && theNode["name"].isString() &&
              theNode["params"].isArray())) {
            nodeErrors += "\n- Syntaxerror in node " +
                    QString(theNode["name"].toString());
            qWarning() << "Error while extracting node:\n" << theNode;
            flags |= 0b0010;
        }
        // node is welldefined:
        else {
            // get name and class(type) of node
            QString type = theNode["class"].toString(),
                    name = theNode["name"].toString();
            qDebug() << "node " << i++ << " parsed:\n name = " << name
                     << " | type = " << type;
            qDebug() << "params:";

            Node *createdNode = NodeFactory::createNode(type);
            if (!createdNode) createdNode = new Node();
            // if node type not in catalog, skip
            if (createdNode->getType() == "") {
                flags |= 0b10010;
                nodeErrors +=
                        "\n- Class \"" + type + "\" is unknown. (Node: \"" + name + "\")";
                createdNode->setType(type);
                // continue;
            }
            createdNode->setName(name);
            // get parameters as array of objects
            foreach (QJsonValue local, theNode["params"].toArray()) {
                // make every object a qvariantmap
                QVariantMap map = local.toObject().toVariantMap();
                // insert all records into params map

                foreach (QString key, map.keys()) {
                    createdNode->setParam(key, map[key]);
                    qDebug() << j++ << ": " << key << " = " << map[key];
                }
            }

            j = 1;  // for debugging only
            qDebug() << "\n";

            // parse the position and other gui_parameters
            if (theNode.contains("gui_params")) {
                QVariantMap p_gui = theNode["gui_params"].toObject().toVariantMap();
                createdNode->moveTo(p_gui["x"].toInt(), p_gui["y"].toInt());
            } else {
                hasPositionData = false;
            }
            // node is complete, so let's insert it

            Data::instance()->addNodeToMesh(createdNode);
        }
    }

    // nodes parsed
    if (!obj["connections"].isArray()) {
        qDebug() << "no connections found";
        flags |= 0b00100;
        goto end;
    } else {
        qDebug() << obj["connections"].toArray().size() << " connections found";
    }

    foreach (QJsonValue var, obj["connections"].toArray()) {
        QJsonObject co = var.toObject();
        QVariantMap theConnection = co.toVariantMap();
        QString src_gate_name = theConnection["src_gate"].toString(),
                dest_gate_name = theConnection["dest_gate"].toString();
        Node *src_node =
                Data::instance()->getNodeByName(theConnection["src_node"].toString());
        Node *dest_node =
                Data::instance()->getNodeByName(theConnection["dest_node"].toString());
        // if nodes not found, skip
        if (!(src_node && dest_node)) {
            flags |= 0b01000;
            continue;
        }
        // if gates were not found, add them
        if (!src_node->hasGate(src_gate_name)) {
            Gate *g = new Gate();
            g->setName(src_gate_name);
            g->addType("unknown");
            g->setDirection(false);
            src_node->addGate(g);
            flags |= 0b10000;
            connectionErrors += "\n- Node \"" + src_node->getName() +
                    "\" has a connection from unknown gate \"" +
                    src_gate_name + "\"";
        }
        if (!dest_node->hasGate(dest_gate_name)) {
            Gate *g = new Gate();
            g->setName(dest_gate_name);
            g->addType("unknown");
            g->setDirection(true);
            dest_node->addGate(g);
            flags |= 0b10000;
            connectionErrors += "\n- Node \"" + dest_node->getName() +
                    "\" has a connection to unknown gate \"" +
                    dest_gate_name + "\"";
        }
        // if connection is invalid, skip
        if (!Data::instance()->checkConnection(
                    src_node->getID(), theConnection["src_gate"].toString(),
                    dest_node->getID(), theConnection["dest_gate"].toString())) {
            flags |= 0b01000;
            connectionErrors += "\n- Connection (" + src_node->getName() + "->" +
                    src_gate_name + "|" + dest_node->getName() + "->" +
                    dest_gate_name + ") is invalid. It will not be added.";
            continue;
        }

        Connection *connection =
                new Connection(src_node, src_node->getGateByName(src_gate_name),
                               dest_node, dest_node->getGateByName(dest_gate_name));

        if (co.contains("gui_params")) {
            QJsonObject json_gui_params = co["gui_params"].toObject();
            QJsonArray way = json_gui_params["waypoints"].toArray();
            QVector<QPoint> waypoints;
            for (QJsonValue v : way) {
                QJsonObject jJoints = v.toObject();
                if (jJoints.contains("x") && jJoints.contains("y"))
                    waypoints << QPoint(jJoints["x"].toInt(), jJoints["y"].toInt());
            }
            connection->setJoints(waypoints);
        } else {
            // if no gui params are found
            QVector<QPoint> waypoints;
            connection->setJoints(waypoints);
        }
        Data::instance()->addConnectionToMesh(connection);
    }
    // all connections added"
    if (hasPositionData == false) {
        qDebug() << "position data missing";
        Data::instance()->sortForce();
    }
end:
    ;
    if (flags.any()) {
        QString msg;
        msg += "Parser encountered Problems. File may be corrupt.\n\nIssues:";
        if (flags[0]) msg += "\n -No nodes were found";
        if (flags[1]) msg += "\n -Error while parsing node";
        if (flags[2]) msg += "\n -No connections found";
        if (flags[3]) msg += "\n -Error while parsing connection";
        if (flags[4]) {
            msg += "\n -Unknown nodes have been detected";
            Data::instance()->setExecutable(false);
        }
        if(nodeErrors == "" && connectionErrors == "")
        QMessageBox::warning(Data::instance()->getMainWindow(),
                             "Errors while parsing!", msg);
        else {
            ParseErrorBox box;
            box.setHeader(msg);
            box.setErrorText("Errors in parsed nodes:\n" + nodeErrors + "\n\n\nErrors in parsed connections:\n" + connectionErrors);
            box.exec();
        }
    }
    parsing = false;
    SingletonRender::instance()->renderMesh();
    return true;
}

void JsonFileHandler::parseProgress(QString &text, const ParseMode &mode) {
    if (mode == progress)
        parseProgress(text);
    else if (mode == error)
        parseErrors(text);
}

bool JsonFileHandler::isParsing() { return parsing; }

void JsonFileHandler::parseProgress(QString &text) {

    QString currentTime = QDateTime::currentDateTime().toString();
    text = text.mid(text.indexOf("{"), text.lastIndexOf("}") + 1);
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    QJsonObject obj = doc.object();

    if (!(obj.contains("progress") &&
                 obj["progress"].toObject().contains("source"))){
        if (!obj.contains("log")){
            return;
        }
        else{
            parseLog(text);
            ;
        }
    }

    obj = obj["progress"].toObject();
    QString source = obj["source"].toString();
    // message comes from a node
    if (source == "node") {
        QString nodeName = obj["name"].toString();
        Node *node = Data::instance()->getNodeByName(nodeName);
        QString msg = obj["msg"].toString();
        if (msg == "start") {
            if (obj["state"].toString() == "init")
                node->setStatus(Node::initializing);
            else if (obj["state"].toString() == "processing")
                node->setStatus(Node::processing);
        } else if (msg == "stop"){
            node->setStatus(Node::idle);
        }
        else if (msg == "percentage") {
            int progress = obj["info"].toInt() ;
            node->setProgress(progress);
        } else if (msg == "error") {
            node->setStatus(Node::error);
            node->addErrorMsg(currentTime+" "+"- " + obj["info"].toString() + "\n");
       } else if (msg == "warning") {
            ;
        }
    }
    // message comes from framework itself
    else if (source == "framework") {
        QString msg = obj["msg"].toString(), state = obj["state"].toString();
        if (msg == "start") {
            // starting simulation
            ;
        } else if (msg == "stop" && state == "processing") {
            // simulation finished
            Data::instance()->stopSimulation();
        } else if (msg == "error") {
            // error occured
        ;

        }
    }
}

void JsonFileHandler::parseErrors(const QString &text) { Q_UNUSED(text); }

/**
 * @brief JsonFileHandler::writeFile writes a string into a given file
 * @param path path to file
 * @param fileContent the content written to the specified file
 */
void JsonFileHandler::saveMesh(const QString &path) {
    QFile file(path);

    file.open(QIODevice::WriteOnly);
    // QByteArray data = *JsonFileHandler::meshToJson(theMesh);
    file.write(meshToJson().toUtf8());
    file.close();
}

QString JsonFileHandler::meshToJson() {
    QJsonArray nodes, connections;
    foreach (Node *n, Data::instance()->getAllNodes()) {
        QJsonObject theNode;
        theNode["class"] = n->getType();
        theNode["name"] = n->getName();
        QJsonArray params;
        QMap<QString, Node::Parameter> *map = n->getParams();
        foreach (QString key, map->keys()) {
            QJsonObject param;
            QVariant var = map->value(key).value;
            var.convert(QVariant::nameToType(map->value(key).type.toUtf8().constData()));
            param[key] = QJsonValue::fromVariant(var);
            params.push_back(param);
        }
        theNode["params"] = params;
        QJsonObject gui_params;
        gui_params["x"] = n->getPosition().x();
        gui_params["y"] = n->getPosition().y();
        theNode["gui_params"] = gui_params;
        nodes.push_back(theNode);
    }
    foreach (Connection *c, Data::instance()->getAllConnections()) {
        QJsonObject theConnection;
        theConnection["src_node"] = c->getSrcNode()->getName();
        theConnection["src_gate"] = c->getSrcGate()->getName();
        theConnection["dest_node"] = c->getDestNode()->getName();
        theConnection["dest_gate"] = c->getDestGate()->getName();
        QJsonObject gui_params;
        QJsonArray way;
        QVector<QPoint> points = *c->getJoints();
        for (int i = 0; i < points.size(); i++) {
            QPoint p = points.at(i);
            QJsonObject point;
            point["x"] = p.x();
            point["y"] = p.y();
            way << (point);
        }
        gui_params["waypoints"] = way;
        theConnection["gui_params"] = gui_params;
        connections << theConnection;
    }
    QJsonObject obj;
    obj.insert("nodes", nodes);
    obj.insert("connections", connections);
    QJsonDocument doc;
    doc.setObject(obj);
    return doc.toJson(QJsonDocument::Indented);
}

void JsonFileHandler::parseLog(QString &text) {
    text = text.mid(text.indexOf("{"), text.lastIndexOf("}") + 1);
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    QJsonObject obj = doc.object();

    if (!obj.contains("log"))
        return;

    obj = obj["log"].toObject();
    QString source = obj["source"].toString();
    // message comes from a node
    if (source == "node") {
        QString nodeName = obj["src_name"].toString();
        Node *node = Data::instance()->getNodeByName(nodeName);
        QString currentTime = obj["time"].toString();
        QString status = obj["status"].toString();
        QString msg = obj["msg"].toString();
        if (status == "info") {
            node->addLogMessage(currentTime + " " + nodeName + " " + msg);
        } else if (status == "warning"){
            node->addLogWarning(currentTime + " " + nodeName + " " + msg);
        } else if (status == "error") {
            node->addLogError(currentTime + " " + nodeName + " " + msg);
        }
    }
    else if (source == "framework") {
    // message comes from framework itself
        QString framework = obj["src_name"].toString();
        QString message=" ";
         QString msg = obj["msg"].toString();
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


     Data::instance()->getMainWindow()->ui->qDebug_out->append(message);

    }
}
