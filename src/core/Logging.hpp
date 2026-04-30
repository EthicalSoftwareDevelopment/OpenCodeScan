#pragma once
#include <QString>
namespace opencodescan {
class Logger final {
public:
    static void initialize();
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);
    [[nodiscard]] static QString logFilePath();
};
} // namespace opencodescan
