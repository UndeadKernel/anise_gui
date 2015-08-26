
#ifndef FILEHANDLER_H
#define FILEHANDLER_H

/*
 * authors:
 * Tobi Lippert
 * Artur Fast
 * Mehrad Mohammadian
 *
 * Date:
 * 2.12.2014
 *
 */
#include <QString>
#include <QList>
#include <QJsonDocument>
#include "node.h"
#include "mesh.h"

class JsonFileHandler {
public:
    enum ParseMode{
        progress,
        error,
        warning
    };

    /**
     * @brief saveMesh Save current Project to a File
     * @param path the File to be written
     * @param theMesh
     */
    static void saveMesh(const QString &path);

    /**
     * @brief meshToJson
     * @return JSON-String Representation of the current Mesh
     */
    static QString meshToJson();

    /**
     * @brief readFile Reads the given File and returnes its content as QJsonObject
     * @param path path to specific File
     * @return JsonObject representingFile Content
     */
    static QJsonObject *readFile(const QString &path);

    /**
     * @brief parseNodeTypesFromAnise Parses all existing Nodes an puts them into the Catalog
     * @param output Output of calling Framework --nodes --mashine
     */
    static void parseNodeTypesFromAnise(QString & output);

    /**
     * @brief extractNodesAndConnections Load a JSON-Object into the Mesh
     * @param obj JSON-Object containing all Information about the Mesh to be loaded
     */
    static bool extractNodesAndConnections(const QJsonObject &obj);

    /**
     * @brief parseProgress parse the progress of the mesh
     * @param text textline given from framework
     * @param mode distinguish between error parsing or progress parsing
     */
    static void parseProgress(QString &text, const ParseMode &mode);

    static bool isParsing();
private:
    static void parseProgress(QString &text);
    static void parseErrors(const QString &text);
    static void parseLog(QString &text);
    static bool parsing;
};

#endif  // FILEHANDLER_H
