#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLockFile>

#include "app/AppController.h"

// Application entry point for V1 bootstrap:
// load settings, initialize tray icon, and keep the process resident.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("wordSnap"));
    QCoreApplication::setApplicationName(QStringLiteral("wordSnapV1"));
    QApplication::setQuitOnLastWindowClosed(false);

    QLockFile singleInstanceLock(QDir::tempPath() + QStringLiteral("/wordSnapV1.lock"));
    singleInstanceLock.setStaleLockTime(0);
    if (!singleInstanceLock.tryLock(0)) {
        qWarning() << "wordSnapV1 is already running.";
        return 0;
    }

    AppController controller;
    QString startupWarning;
    if (!controller.initialize(&startupWarning)) {
        qCritical() << "Application startup failed:" << startupWarning;
        return 1;
    }

    if (!startupWarning.isEmpty()) {
        qWarning() << startupWarning;
    }

    return app.exec();
}
