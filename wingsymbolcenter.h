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
