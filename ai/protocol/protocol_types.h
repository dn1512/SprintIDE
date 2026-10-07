#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <QString>

namespace SprintIDE {
namespace Protocol {

// v1.1 协议错误码（对应设计文档 4.7 节）
namespace ErrorCode {
inline const QString PathNotFound       = QStringLiteral("PATH_NOT_FOUND");
inline const QString PathExists         = QStringLiteral("PATH_EXISTS");
inline const QString PermissionDenied   = QStringLiteral("PERMISSION_DENIED");
inline const QString AmbiguousSearch    = QStringLiteral("AMBIGUOUS_SEARCH");
inline const QString SearchNotFound     = QStringLiteral("SEARCH_NOT_FOUND");
inline const QString InvalidMode        = QStringLiteral("INVALID_MODE");
inline const QString InvalidFields      = QStringLiteral("INVALID_FIELDS");
inline const QString UnsupportedType    = QStringLiteral("UNSUPPORTED_TYPE");
inline const QString Timeout            = QStringLiteral("TIMEOUT");
inline const QString ExitCodeNonZero    = QStringLiteral("EXIT_CODE_NONZERO");
inline const QString AgentAborted       = QStringLiteral("AGENT_ABORTED");
inline const QString UserRejected       = QStringLiteral("USER_REJECTED");
inline const QString InternalError      = QStringLiteral("INTERNAL_ERROR");
inline const QString RunIdNotFound      = QStringLiteral("RUN_ID_NOT_FOUND");
inline const QString MultipleActiveRuns = QStringLiteral("MULTIPLE_ACTIVE_RUNS");
inline const QString BuildFailed        = QStringLiteral("BUILD_FAILED");
inline const QString NoDefaultTarget    = QStringLiteral("NO_DEFAULT_TARGET");
inline const QString MethodNotFound     = QStringLiteral("METHOD_NOT_FOUND");
inline const QString MethodAmbiguous    = QStringLiteral("METHOD_AMBIGUOUS");
inline const QString FileNotOpen        = QStringLiteral("FILE_NOT_OPEN");
}

} // namespace Protocol
} // namespace SprintIDE

#endif // PROTOCOL_TYPES_H
