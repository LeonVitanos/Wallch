
#include "desktopenvironment.h"
#include "glob.h"
#include "settingsmanager.h"

#include <QMessageBox>

#ifdef Q_OS_LINUX
    #include <gio/gio.h>
    DE::Value currentDE = DE::Gnome;
    #include "desktopenvironment_kde.h"
#endif

DesktopEnvironment::DesktopEnvironment(){}

double DesktopEnvironment::getOSproductVersion(){
    return QSysInfo::productVersion().toDouble();
}

QString DesktopEnvironment::getOSproductType(){
    return QSysInfo::productType().toLower();
}

QString DesktopEnvironment::getOSprettyName(){
    QString name = QSysInfo::productType().toLower();
    name[0] = name[0].toUpper();
    return name;
}

QString DesktopEnvironment::getOSWallpaperPath(){
#ifdef Q_OS_LINUX
    if(currentDE == DE::LXDE && QDir("/usr/share/lubuntu/wallpapers").exists())
        return "/usr/share/lubuntu/wallpapers";
    if(currentDE == DE::XFCE && QDir("usr/share/xfce4/backdrops").exists())
        return "usr/share/xfce4/backdrops";
    else if(QDir("/usr/share/backgrounds").exists())
        return "/usr/share/backgrounds";
    else if(QDir("/usr/share/wallpapers").exists())
        return "/usr/share/wallpapers";
    //enlightenment(when supported) usr/share/enlightenment/data/backgrounds
#else
# ifdef Q_OS_WIN
    return "C:\\Windows\\Web\\Wallpaper";
# endif
# ifdef Q_OS_MAC
    if (QDir("/System/Library/Desktop Pictures").exists())
        return "/System/Library/Desktop Pictures";
    return "/Library/Desktop Pictures/";
# endif
#endif
    return "";
}

#ifdef Q_OS_LINUX
void DesktopEnvironment::setCurrentDE(){
    if(settings->value("desktopEnvironment", 0).toInt() != 0)
        currentDE = static_cast<DE::Value>(settings->value("desktopEnvironment", 0).toInt()-1);
    else
        currentDE = detectCurrentDe();
}

DE::Value DesktopEnvironment::detectCurrentDe(){
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString xdg = env.value("XDG_CURRENT_DESKTOP");

    if(xdg.contains("LXDE", Qt::CaseInsensitive) || xdg.contains("LXQT", Qt::CaseInsensitive))
        return DE::LXDE;
    else if(xdg.contains("XFCE", Qt::CaseInsensitive))
        return DE::XFCE;
    else if(xdg.contains("MATE", Qt::CaseInsensitive))
        return DE::Mate;
    else if(xdg.contains("KDE", Qt::CaseInsensitive))
        return DE::KDE;
    else if(xdg.contains("GNOME", Qt::CaseInsensitive) || xdg.contains("UNITY", Qt::CaseInsensitive))
        return DE::Gnome;
    else{
        if(QFile::exists("/usr/bin/xfconf-query"))
            return DE::XFCE;
        else if(QFile::exists("/usr/bin/mate-help"))
            return DE::Mate;
        else if(QDir("/etc/xdg/lubuntu").exists())
            return DE::LXDE;
        else
            return DE::Gnome;
    }
}

QString DesktopEnvironment::getCurrentDEprettyName(){
    DE::Value detected = detectCurrentDe();

    switch(detected){
    case DE::Gnome:
        return "Gnome";
    case DE::LXDE:
        return "LXDE";
    case DE::XFCE:
        return "XFCE";
    case DE::Mate:
        return "Mate";
    default:
        return "Unknown";
    }
}

QStringList DesktopEnvironment::runCommand(const QString &command, const QStringList &parameters, bool replaceNewLine, const QString &split){
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command, parameters,QIODevice::ReadWrite);

    if(!process.waitForStarted())
        return QStringList();

    QByteArray data;

    process.waitForFinished(3000);

    data.append(process.readAll());

    QString output = replaceNewLine ? QString(data.data()).replace("\n", "") : data.data();

    return split.isEmpty() ? QStringList() << output : output.split(split);
}

// Gnome, Mate

void DesktopEnvironment::gsettingsSet(const QString &schema, const QString &key, const QString &value){
    GSettings *settings = g_settings_new(schema.toLocal8Bit().data());
    gboolean result = g_settings_set_string (settings, key.toLocal8Bit().data(), value.toLocal8Bit().data());
    if(!result)
        QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Could not apply change. If your Desktop Environment is not listed at \"Preferences->Integration->Current Desktop Environment\", then it is not supported."));

    g_settings_sync ();

    if (settings != NULL)
        g_object_unref (settings);
}

QString DesktopEnvironment::gsettingsGet(const QString &schema, const QString &key){
    GSettings *settings = g_settings_new(schema.toLocal8Bit().data());
    gchar *printed = g_settings_get_string (settings, key.toLocal8Bit().data());
    QString finalSetting = QString(printed);
    g_free (printed);

    if (settings != NULL)
        g_object_unref (settings);

    return finalSetting;
}

QString DesktopEnvironment::getPictureUriName(){
    // From Ubuntu 22.04, there is a different setting for light and dark theme
    if(DesktopEnvironment::getOSproductType() == "ubuntu" && DesktopEnvironment::getOSproductVersion()>=22.04 && DesktopEnvironment::gsettingsGet("org.gnome.desktop.interface", "color-scheme") == "prefer-dark")
        return "picture-uri-dark";
    else
        return "picture-uri";
}

//LXDE

QString DesktopEnvironment::getPcManFmValue(const QString &key){
    QString result;

    QSettings settings("pcmanfm", QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists() ? "lubuntu/pcmanfm" : "default/pcmanfm");
    settings.beginGroup("desktop");
    result=settings.value(key, "").toString();
    settings.endGroup();

    //TODO: CHECK IF LXDE ever returns "0", in order to return 1 on the getCurrentFit

    return result;
}

void DesktopEnvironment::setPcManFmValue(const QString &key, const QString &value){
    bool lubuntuPathExists = QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists();

    QSettings settings("pcmanfm", lubuntuPathExists ? "lubuntu/pcmanfm" : "default/pcmanfm");
    settings.beginGroup("desktop");
    settings.setValue(key, value);
    settings.endGroup();
    settings.sync();

    QStringList pids = runCommand("pidof", QStringList() << "pcmanfm", true, " ");
    Q_FOREACH(QString pid, pids){
        QStringList output = runCommand("ps", QStringList() << "-fp" << pid, false, "");
        if(output.contains("--desktop"))
            QProcess::startDetached("kill", QStringList() << "-9" << pid);
    }
    if(lubuntuPathExists)
        QProcess::startDetached("pcmanfm", QStringList() << "--desktop" << "-p" << "lubuntu");
    else
        QProcess::startDetached("pcmanfm", QStringList() << "--desktop");
}

bool DesktopEnvironment::runPcManFm(QStringList args){
    QProcess process;
    process.start("pcmanfm", args);
    process.waitForFinished(3000);

    if(process.error() == QProcess::FailedToStart)
        return false;
    else
        return process.readAllStandardError().isEmpty(); //TODO: CHECK WITH LXDE
}

// XFCE

QStringList DesktopEnvironment::runXfCommand(bool backdrop, const QStringList &parameters, bool replaceNewLine)
{
    QStringList finalArgs;
    finalArgs << "-c" << "xfce4-desktop" << "-p";

    finalArgs.append(parameters);

    if (backdrop) {
        finalArgs << "/backdrop" << "-l";
    }

    return runCommand("xfconf-query", finalArgs, replaceNewLine);
}

bool DesktopEnvironment::runXfconf(QStringList args){
    QProcess process;
    process.start("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << args);
    process.waitForFinished(3000);

    if(process.error() == QProcess::FailedToStart)
        return false;
    else
        return process.readAllStandardError().isEmpty(); //TODO: CHECK WITH XFCE
}

void DesktopEnvironment::processXfconfQuery(const QStringList &keywordsToFind,
                                            std::function<bool(const QString&)> processor)
{
    const QStringList results = runXfCommand(true);

    for (const QString &entry : results) {
        for (const QString &keyword : keywordsToFind) {
            if (entry.contains(keyword)) {
                if (!processor(entry)) {
                    return;
                }
                break;
            }
        }
    }
}

// KDE

bool DesktopEnvironment::setKdeWallpaper(const QString &image) {
    return desktopenvironment_kde::setKdeWallpaper(image);
}

short DesktopEnvironment::getKdeWallpaperStyle() {
    return desktopenvironment_kde::getKdeWallpaperStyle();
}

bool DesktopEnvironment::setKdeWallpaperStyle(short index) {
    return desktopenvironment_kde::setKdeWallpaperStyle(index);
}

QString DesktopEnvironment::getCurrentWallpaper() {
    return desktopenvironment_kde::getCurrentWallpaper();
}

#endif
