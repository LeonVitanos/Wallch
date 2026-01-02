
#ifndef DESKTOPENVIRONMENT_H
#define DESKTOPENVIRONMENT_H

#include <QString>
#include <QStringList>

#ifdef Q_OS_LINUX
    struct DE {
        enum Value {
            Gnome, XFCE, LXDE, Mate, KDE
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

#ifdef Q_OS_LINUX
    static void setCurrentDE();
    static DE::Value detectCurrentDe();
    static QString getCurrentDEprettyName();
    static QStringList runCommand(
        const QString &command,
        const QStringList &parameters = QStringList(),
        bool replaceNewLine = false,
        const QString &split = "\n"
    );

    // Gnome, Mate
    static QString gsettingsGet(const QString &schema, const QString &key);
    static void gsettingsSet(const QString &schema, const QString &key, const QString &value);
    static QString getPictureUriName();

    // LXDE
    static QString getPcManFmValue(const QString &key);
    static void setPcManFmValue(const QString &key, const QString &value);
    static bool runPcManFm(QStringList args);

    // XFCE
    static QStringList runXfCommand(bool backdrop = true,
                                    const QStringList &parameters = QStringList(),
                                    bool replaceNewLine = false);
    static void processXfconfQuery(const QStringList &keywordsToFind,
                                  std::function<bool(const QString&)> processor);
    static bool runXfconf(QStringList args);

    // KDE
    static bool setKdeWallpaper(const QString &image);
    static short getKdeWallpaperStyle();
#endif
};

#endif // DESKTOPENVIRONMENT_H
