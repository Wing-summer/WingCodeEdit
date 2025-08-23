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

#ifndef WINGSYMBOLCENTER_H
#define WINGSYMBOLCENTER_H

#include <QHash>
#include <QPixmap>
#include <QString>

class WingSymbolCenter {
    Q_DISABLE_COPY_MOVE(WingSymbolCenter)
public:
    static WingSymbolCenter &instance();

public:
    bool existSymbol(const QString &id);

    QPixmap symbolFromName(const QString &id);

    void regsiterSymbol(const QString &id, const QPixmap &symbol);

private:
    WingSymbolCenter();

private:
    QHash<QString, QPixmap> _caches;
};

#endif // WINGSYMBOLCENTER_H
