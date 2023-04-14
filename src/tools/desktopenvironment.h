
#ifndef DESKTOPENVIRONMENT_H
#define DESKTOPENVIRONMENT_H

#include <QString>
#include <QStringList>

#ifdef Q_OS_UNIX
struct DE {
    enum Value {
        Gnome, XFCE, LXDE, Mate
    };
};

extern DE::Value currentDE;
#endif

class DesktopEnvironment
{
public:
    DesktopEnvironment();
    static double getOSproductVersion();
    static QString getOSproductType();
    static QString getOSprettyName();
    static QString getOSWallpaperPath();

#ifdef Q_OS_UNIX
    static void setCurrentDE();
    static DE::Value detectCurrentDe();
    static QString getCurrentDEprettyName();

    // Gnome, Mate
    static QString gsettingsGet(const QString &schema, const QString &key);
    static void gsettingsSet(const QString &schema, const QString &key, const QString &value);
    static QString getPictureUriName();

    // LXDE
    static QString getPcManFmValue(const QString &key);
    static void setPcManFmValue(const QString &key, const QString &value);
    static bool runPcManFm(QStringList args);

    // XFCE
    static bool runXfconf(QStringList args);
    static QStringList runCommand(QString command, bool backdrop = false, QStringList parameters = QStringList(),
                                  bool replaceNewLine = false, QString split = "\n");
#endif
};

#endif // DESKTOPENVIRONMENT_H
