#include <QApplication>
#include "app/ApplicationBootstrap.hpp"
int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OpenCodeScan"));
    QApplication::setApplicationDisplayName(QStringLiteral("OpenCodeScan"));
    QApplication::setOrganizationName(QStringLiteral("OpenCodeScan"));
    QApplication::setOrganizationDomain(QStringLiteral("opencodescan.local"));
    ApplicationBootstrap bootstrap(application);
    return bootstrap.run();
}
