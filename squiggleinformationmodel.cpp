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
    return _editor->m_squiggles.size();
}

QVariant SquiggleInformationModel::data(const QModelIndex &index,
                                        int role) const {
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

    return {};
}
