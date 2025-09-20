/****************************************************************************
**
** Copyright (C) 2025-2028 WingSummer
**
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the GNU General Public License version 3
** along with this program. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef WINGSQUIGGLEINFOMODEL_H
#define WINGSQUIGGLEINFOMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <QVector>

class WingCodeEdit;

class WingSquiggleInfoModel : public QAbstractListModel {
public:
    enum class SeverityLevel { Hint, Information, Warning, Error, MAX_LEVEL };

public:
    explicit WingSquiggleInfoModel(WingCodeEdit *editor,
                                   QObject *parent = nullptr);
    explicit WingSquiggleInfoModel(QObject *parent = nullptr);

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

#endif // WINGSQUIGGLEINFOMODEL_H
