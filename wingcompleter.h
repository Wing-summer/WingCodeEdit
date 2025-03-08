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

class WingCodeEdit;
class WingCodePopup;

class WingCompleter : public QCompleter {
    Q_OBJECT
public:
    explicit WingCompleter(WingCodeEdit *editor);
    virtual ~WingCompleter();

private:
    WingCodePopup *m_popUp;
};

#endif // WINGCOMPLETER_H
