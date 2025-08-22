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

#include <QScrollBar>

WingCompleter::WingCompleter(WingCodeEdit *editor)
    : QCompleter(editor), m_triggerAmount(3) {
    setCompletionMode(QCompleter::PopupCompletion);
    setCaseSensitivity(Qt::CaseInsensitive);
    setCompletionRole(Qt::DisplayRole);
    setFilterMode(Qt::MatchStartsWith);

    if (editor) {
        editor->setCompleter(this);
    }

    m_popUp = new WingCodePopup(editor);
    setPopup(m_popUp);
}

WingCompleter::~WingCompleter() { m_popUp->deleteLater(); }

QStringList WingCompleter::triggerList() const { return m_triggerList; }

void WingCompleter::setTriggerList(const QStringList &triggers) {
    m_triggerList = triggers;
}

void WingCompleter::trigger(const QString &trigger, const QString &content,
                            const QRect &cursorRect) {
    if (processTrigger(trigger, content)) {
        auto cr = cursorRect;
        cr.setWidth(m_popUp->sizeHintForColumn(0) +
                    m_popUp->verticalScrollBar()->sizeHint().width());
        auto idx = completionModel()->index(0, 0);
        m_popUp->setCurrentIndex(idx);
        complete(cr); // popup it up!
        m_popUp->showTooltip(idx);
    }
}

bool WingCompleter::processTrigger(const QString &trigger,
                                   const QString &content) {
    Q_UNUSED(trigger);
    auto seps = wordSeperators();
    auto r =
        std::find_if(content.crbegin(), content.crend(),
                     [seps](const QChar &ch) { return seps.contains(ch); });
    auto idx = std::distance(r, content.crend());
    setCompletionPrefix(content.sliced(idx));
    return true;
}

QString WingCompleter::wordSeperators() const {
    static QString eow(QStringLiteral("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="));
    return eow;
}

qint32 WingCompleter::triggerAmount() const { return m_triggerAmount; }

void WingCompleter::setTriggerAmount(qint32 newTriggerAmount) {
    m_triggerAmount = newTriggerAmount;
}
