#include "desktopenvironment_kde.h"

#ifdef Q_OS_LINUX
    #include <QtDBus/QDBusInterface>
    #include <QtDBus/QDBusMessage>
#endif

QString desktopenvironment_kde::s_cachedKdeGroup = "";

bool desktopenvironment_kde::setKdeWallpaper(const QString &image) {
    if (s_cachedKdeGroup.isEmpty()) {
        s_cachedKdeGroup = probeKdeGroup();
    }

    int version = getKdeMajorVersion();
    QString script = getKdeScriptTemplate(version, image);

    return !executeKdeScript(script).isNull();
}

int desktopenvironment_kde::getKdeMajorVersion() {
    QByteArray kVersion = qgetenv("KDE_SESSION_VERSION");
    if (!kVersion.isEmpty()) {
        return kVersion.toInt();
    }

    QDBusInterface interface("org.kde.plasmashell", "/MainApplication", "org.qtproject.Qt.LibraryInfo");
    if (interface.isValid()) {
        QString v = interface.property("version").toString();
        if (!v.isEmpty()) {
            return v.section('.', 0, 0).toInt();
        }
    }

    return 5;
}

QString desktopenvironment_kde::getKdeScriptTemplate(int version, const QString &image) {
    if (version >= 6) {
        // Modern ECMAScript (Plasma 6)
        return QString(
                   "desktops().forEach(d => {"
                   "    d.currentConfigGroup = ['%1', 'org.kde.image', 'General'];"
                   "    d.writeConfig('Image', 'file://%2');"
                   "    d.reloadConfig();"
                   "})"
                   ).arg(s_cachedKdeGroup, image);
    } else {
        // Legacy Javascript (Plasma 5)
        return QString(
                   "var allDesktops = desktops();"
                   "for (var i = 0; i < allDesktops.length; i++) {"
                   "    var d = allDesktops[i];"
                   "    d.currentConfigGroup = ['%1', 'org.kde.image', 'General'];"
                   "    d.writeConfig('Image', 'file://%2');"
                   "    d.reloadConfig();"
                   "}"
                   ).arg(s_cachedKdeGroup, image);
    }
}

QString desktopenvironment_kde::executeKdeScript(const QString &script) {
    QDBusInterface remoteApp("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    if (!remoteApp.isValid()) return "";

    QDBusMessage reply = remoteApp.call("evaluateScript", script);

    const QVariantList args = reply.arguments();
    QString result = args.isEmpty() ? "" : args.first().toString();

    return result;
}

QString desktopenvironment_kde::probeKdeGroup() {
    QString probeScript =
        "var d = desktops()[0];"
        "d.currentConfigGroup = ['Wallpapers', 'org.kde.image', 'General'];"
        "if (d.configKeys.length > 0) { 'Wallpapers'; } else { 'Wallpaper'; }";

    QDBusInterface remoteApp("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    QDBusMessage reply = remoteApp.call("evaluateScript", probeScript);

    const QVariantList args = reply.arguments();
    QString result = args.isEmpty() ? "" : args.first().toString();
    return result.isEmpty() ? "Wallpaper" : result;
}

short desktopenvironment_kde::getKdeWallpaperStyle() {
    QString script = "var d = desktops()[0]; "
                     "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General']; "
                     "print(d.wallpaperPlugin + ':' + d.readConfig('FillMode'));";

    QString response = executeKdeScript(script).trimmed();
    QStringList parts = response.split(':');
    QString plugin = parts.value(0);
    QString fillResult = parts.value(1);

    if (plugin == "org.kde.color") return 0; // NoneStyle

    if (fillResult.isEmpty()) return 1; // Scaled and Cropped

    int mode = fillResult.toInt();
    if (mode == 0)      return 2; // Scaled
    else if (mode == 1) return 3; // Scaled, Keep Proportions
    else if (mode == 6) return 4; // Centered
    else if (mode == 3) return 5; // Tiled

    return 1; // Safeguard
}
