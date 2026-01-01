#include "desktopenvironment.h"
#ifdef Q_OS_LINUX
    #include <QtDBus/QDBusInterface>
    #include <QtDBus/QDBusMessage>
#endif

QString DesktopEnvironment::s_cachedKdeGroup = "";

bool DesktopEnvironment::setKdeWallpaper(const QString &image) {
    if (s_cachedKdeGroup.isEmpty()) {
        s_cachedKdeGroup = probeKdeGroup();
    }

    int version = getKdeMajorVersion();
    QString script = getKdeScriptTemplate(version, image);

    return executeKdeScript(script);
}

int DesktopEnvironment::getKdeMajorVersion() {
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

QString DesktopEnvironment::getKdeScriptTemplate(int version, const QString &image) {
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

bool DesktopEnvironment::executeKdeScript(const QString &script) {
    QDBusInterface remoteApp("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    if (!remoteApp.isValid()) {
        return false;
    }

    QDBusMessage reply = remoteApp.call("evaluateScript", script);
    return reply.errorMessage().isEmpty();
}

QString DesktopEnvironment::probeKdeGroup() {
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
