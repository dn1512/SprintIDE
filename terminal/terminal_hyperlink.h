// terminal_hyperlink.h
class TerminalHyperlink {
    // 正则识别：
    //   https?://[^\s]+
    //   file://[^\s]+
    //   [\w/\.\-]+:\d+(:\d+)?       // path:line:col
    // 悬停下划线 + 手型
    // Cmd/Ctrl+点击 → openUrl / emit openFileAtLine
};