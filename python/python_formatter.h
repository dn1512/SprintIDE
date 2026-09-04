#ifndef PYTHON_FORMATTER_H
#define PYTHON_FORMATTER_H

#include <QString>

namespace SprintIDE {

class PythonFormatter {
public:
    static bool formatFile(const QString& filePath, QString& error);
    static bool isAvailable();
};

} // namespace SprintIDE

#endif // PYTHON_FORMATTER_H
