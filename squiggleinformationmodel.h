#ifndef SQUIGGLEINFORMATIONMODEL_H
#define SQUIGGLEINFORMATIONMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <QVector>

class WingCodeEdit;

class SquiggleInformationModel : public QAbstractListModel {
public:
    enum class SeverityLevel { Hint, Information, Warning, Error, MAX_LEVEL };

public:
    explicit SquiggleInformationModel(WingCodeEdit *editor,
                                      QObject *parent = nullptr);
    explicit SquiggleInformationModel(QObject *parent = nullptr);

public:
    WingCodeEdit *editor() const;
    void setEditor(WingCodeEdit *newEditor);

    void setSeverityLevelIcon(SeverityLevel level, const QIcon &icon);
    QIcon severityLevelIcon(SeverityLevel level) const;

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    WingCodeEdit *_editor;
    QVector<QIcon> _icons;
};

#endif // SQUIGGLEINFORMATIONMODEL_H
