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

#ifndef WINGCOMPLETER_H
#define WINGCOMPLETER_H

#include <QCompleter>
#include <QSortFilterProxyModel>
#include <QTextCursor>

class WingCodeEdit;
class WingCodePopup;

class WingCompleter : public QCompleter {
    Q_OBJECT
public:
    explicit WingCompleter(WingCodeEdit *editor);
    virtual ~WingCompleter();

public:
    QStringList triggerList() const;

    qint32 triggerAmount() const;
    void setTriggerAmount(qint32 newTriggerAmount);

    virtual QString wordSeperators() const;

public slots:
    void setTriggerList(const QStringList &triggers);

    void trigger(const QString &trigger, const QString &content,
                 const QRect &cursorRect);

protected:
    virtual bool processTrigger(const QString &trigger, const QString &content);

private:
    WingCodePopup *m_popUp;

    QStringList m_triggerList;
    qint32 m_triggerAmount;
};

#endif // WINGCOMPLETER_H
