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

#include "wingcodepopup.h"

#include "wingcodeedit.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QStyleFactory>

constexpr auto PADDING = 3;

WingCodePopup::WingCodePopup(WingCodeEdit *editor) : QListView(editor) {
    // Changes listview properties
    setBatchSize(10);
    setMovement(Static);
    setFlow(TopToBottom);
    setLayoutMode(Batched);
    setUniformItemSizes(true);
    setEditTriggers(NoEditTriggers);
    setSelectionMode(SingleSelection);
    setCursor(Qt::ArrowCursor);
    setSelectionBehavior(SelectRows);
    setFocusPolicy(Qt::StrongFocus);

    auto fontMetric = QFontMetrics(font());
    setIconSize(QSize(fontMetric.height(), fontMetric.height()));

    this->setPalette(editor->palette());
    setStyleSheet(QStringLiteral("QListView{outline: 0;}"));
    setFixedWidth(200);

    connect(editor, &WingCodeEdit::themeChanged, this,
            [this, editor] { this->setPalette(editor->palette()); });
}

void WingCodePopup::showEvent(QShowEvent *e) {
    QListView::showEvent(e);

    verticalScrollBar()->setValue(0);
    horizontalScrollBar()->setValue(0);
}
