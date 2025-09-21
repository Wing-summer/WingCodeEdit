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

#include "wingsquiggleinfomodel.h"

#include "wingcodeedit.h"

WingSquiggleInfoModel::WingSquiggleInfoModel(WingCodeEdit *editor,
                                             QObject *parent)
    : QAbstractListModel(parent), _editor(editor) {
    Q_ASSERT(editor);
    static_assert(int(SeverityLevel::Error) ==
                  int(WingCodeEdit::SeverityLevel::Error));
    static_assert(int(SeverityLevel::Hint) ==
                  int(WingCodeEdit::SeverityLevel::Hint));
    static_assert(int(SeverityLevel::Information) ==
                  int(WingCodeEdit::SeverityLevel::Information));
    static_assert(int(SeverityLevel::Warning) ==
                  int(WingCodeEdit::SeverityLevel::Warning));
    _icons.resize(int(SeverityLevel::MAX_LEVEL));
    connect(editor, &WingCodeEdit::squiggleItemChanged, this,
            [this]() { emit layoutChanged(); });
}

WingSquiggleInfoModel::WingSquiggleInfoModel(QObject *parent)
    : QAbstractListModel(parent), _editor(nullptr) {
    _icons.resize(int(SeverityLevel::MAX_LEVEL));
}

void WingSquiggleInfoModel::setSeverityLevelIcon(SeverityLevel level,
                                                 const QIcon &icon) {
    if (level != SeverityLevel::MAX_LEVEL) {
        _icons[int(level)] = icon;
    }
}

QIcon WingSquiggleInfoModel::severityLevelIcon(SeverityLevel level) const {
    if (level == SeverityLevel::MAX_LEVEL) {
        return {};
    }
    return _icons[int(level)];
}

int WingSquiggleInfoModel::rowCount(const QModelIndex &parent) const {
    if (_editor) {
        return _editor->m_squiggles.size();
    } else {
        return 0;
    }
}

QVariant WingSquiggleInfoModel::data(const QModelIndex &index, int role) const {
    if (_editor) {
        auto idx = index.row();
        auto data = _editor->m_squiggles.at(idx);
        switch (role) {
        case Qt::DecorationRole:
            return _icons.at(int(data.level));
        case Qt::DisplayRole:
            return data.tooltip + tr("[row: %1, col: %2]")
                                      .arg(data.start.first)
                                      .arg(data.start.second)
                                      .prepend(' ');
        case Qt::ToolTipRole:
            return data.tooltip;
        }
    }
    return {};
}

WingCodeEdit *WingSquiggleInfoModel::editor() const { return _editor; }

void WingSquiggleInfoModel::setEditor(WingCodeEdit *newEditor) {
    if (_editor == newEditor) {
        return;
    }
    if (_editor) {
        _editor->disconnect(this, nullptr);
    }
    _editor = newEditor;
    if (_editor) {
        connect(_editor, &WingCodeEdit::squiggleItemChanged, this,
                [this]() { emit layoutChanged(); });
        connect(_editor, &WingCodeEdit::destroyed, this, [this]() {
            _editor->disconnect(this, nullptr);
            _editor = nullptr;
        });
    }
    emit layoutChanged();
}
