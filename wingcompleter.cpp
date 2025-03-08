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

#include "wingcompleter.h"

#include "wingcodeedit.h"
#include "wingcodepopup.h"

WingCompleter::WingCompleter(WingCodeEdit *editor) : QCompleter(editor) {
    setCompletionMode(QCompleter::PopupCompletion);
    setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setCaseSensitivity(Qt::CaseInsensitive);
    setFilterMode(Qt::MatchContains);

    if (editor) {
        editor->setCompleter(this);
    }

    m_popUp = new WingCodePopup(editor);
    setPopup(m_popUp);
}

WingCompleter::~WingCompleter() { m_popUp->deleteLater(); }
