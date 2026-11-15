class VTermScreenBridge {
public:
    static VTermScreenCallbacks callbacks();

    // 关键回调
    static int onDamage(VTermRect, void*);       // → model->markDirty(rect)
    static int onMoveCursor(VTermPos, VTermPos, int visible, void*);
    static int onSetTermProp(VTermProp, VTermValue*, void*);  // ALT_SCREEN / 标题
    static int onBell(void*);                    // → 系统通知（macOS Notification Center）
    static int onSbPushLine(int cols, const VTermScreenCell*, void*);  // 推入 scrollback
    static int onResize(int rows, int cols, void*);
};