#ifndef DESKTOPENVIRONMENT_KDE_H
#define DESKTOPENVIRONMENT_KDE_H


class desktopenvironment_kde
{
public:
    desktopenvironment_kde();
    static bool setKdeWallpaper(const QString &image);
    static short getKdeWallpaperStyle();

private:
    static QString s_cachedKdeGroup;
    static QString probeKdeGroup();
    static int getKdeMajorVersion();
    static QString executeKdeScript(const QString &script);
    static QString getKdeScriptTemplate(int version, const QString &image);
};

#endif // DESKTOPENVIRONMENT_KDE_H
