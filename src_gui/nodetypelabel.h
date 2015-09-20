#ifndef NODETYPELABEL_H
#define NODETYPELABEL_H

#include <QLabel>

class NodeTypeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit NodeTypeLabel(QWidget *parent = 0);

    QString getType() const;
    void setType(const QString &value);

    void mousePressEvent(QMouseEvent *ev);
    QString getCategory() const;
    void setCategory(const QString &value);

signals:

public slots:

private:
    QString type;
    QString category;

};

#endif // NODETYPELABEL_H
