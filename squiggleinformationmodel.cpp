#include "squiggleinformationmodel.h"

#include "wingcodeedit.h"

SquiggleInformationModel::SquiggleInformationModel(WingCodeEdit *editor,
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

SquiggleInformationModel::SquiggleInformationModel(QObject *parent)
    : QAbstractListModel(parent), _editor(nullptr) {
    _icons.resize(int(SeverityLevel::MAX_LEVEL));
}

void SquiggleInformationModel::setSeverityLevelIcon(SeverityLevel level,
                                                    const QIcon &icon) {
    if (level != SeverityLevel::MAX_LEVEL) {
        _icons[int(level)] = icon;
    }
}

QIcon SquiggleInformationModel::severityLevelIcon(SeverityLevel level) const {
    if (level == SeverityLevel::MAX_LEVEL) {
        return {};
    }
    return _icons[int(level)];
}

int SquiggleInformationModel::rowCount(const QModelIndex &parent) const {
    if (_editor) {
        return _editor->m_squiggles.size();
    } else {
        return 0;
    }
}

QVariant SquiggleInformationModel::data(const QModelIndex &index,
                                        int role) const {
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

WingCodeEdit *SquiggleInformationModel::editor() const { return _editor; }

void SquiggleInformationModel::setEditor(WingCodeEdit *newEditor) {
    if (_editor == newEditor) {
        return;
    }
    if (_editor) {
        _editor->disconnect(this);
    }
    _editor = newEditor;
    if (_editor) {
        connect(_editor, &WingCodeEdit::squiggleItemChanged, this,
                [this]() { emit layoutChanged(); });
    }
    emit layoutChanged();
}
