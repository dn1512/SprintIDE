#!/bin/bash
# ============================================================
# SprintIDE 内嵌 lldb + 签名脚本
#
# 前置：先手动执行 make -j4 和 make deploy
# 作用：把 lldb（缓存中的预处理产物）内嵌进 SprintIDE.app
#       然后重新签名
#
# 使用：
#   ./scripts/package.sh
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUNDLE_APP="$PROJECT_ROOT/build/SprintIDE.app"

ARCH="$(uname -m)"
OS="$(uname -s)"

info() { echo "[package] $*"; }
err()  { echo "[package] ERROR: $*" >&2; }

# ------------------------------------------------------------
# 1. 检查 app bundle
# ------------------------------------------------------------
if [ ! -d "$BUNDLE_APP" ]; then
    err "找不到 $BUNDLE_APP"
    err "请先在 build/ 目录里执行 make 和 make deploy"
    exit 1
fi

# ------------------------------------------------------------
# 2. 探测 Homebrew 前缀
# ------------------------------------------------------------
if command -v brew >/dev/null 2>&1; then
    BREW_PREFIX="$(brew --prefix)"
else
    if [ "$ARCH" = "arm64" ]; then
        BREW_PREFIX="/opt/homebrew"
    else
        BREW_PREFIX="/usr/local"
    fi
fi
info "Homebrew: $BREW_PREFIX"

# ------------------------------------------------------------
# 3. 探测 lldb-dap
# ------------------------------------------------------------
LLDB_DAP=""

for P in \
    "$BREW_PREFIX/opt/lldb/bin/lldb-dap" \
    "$BREW_PREFIX/opt/llvm/bin/lldb-dap" \
    "$BREW_PREFIX/opt/llvm/bin/lldb-vscode" \
    "$BREW_PREFIX/bin/lldb-dap"
do
    if [ -x "$P" ]; then
        LLDB_DAP="$P"
        break
    fi
done

if [ -z "$LLDB_DAP" ] && command -v lldb-dap >/dev/null 2>&1; then
    LLDB_DAP="$(command -v lldb-dap)"
fi

if [ -z "$LLDB_DAP" ]; then
    err "未找到 lldb-dap"
    err "安装：brew install lldb"
    err "或检查 /usr/local/opt/lldb 是否被改名（如 lldb.disabled）"
    exit 1
fi
info "lldb-dap: $LLDB_DAP"

# ------------------------------------------------------------
# 4. 提取 LLVM 版本号
# ------------------------------------------------------------
LLVM_VERSION="$("$LLDB_DAP" --version 2>&1 \
    | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' \
    | head -1)"

if [ -z "$LLVM_VERSION" ]; then
    LLVM_VERSION="unknown"
fi
info "LLVM 版本: $LLVM_VERSION"

# ------------------------------------------------------------
# 5. 缓存目录
# ------------------------------------------------------------
CACHE_DIR="$HOME/Library/Caches/SprintIDE/toolchains/lldb-${OS}-${ARCH}-${LLVM_VERSION}"

if [ -x "$CACHE_DIR/bin/lldb-dap" ]; then
    info "缓存命中: $CACHE_DIR"
else
    info "缓存未命中，正在生成（首次约 3-5 分钟）..."
    info "  目标: $CACHE_DIR"

    if ! command -v dylibbundler >/dev/null 2>&1; then
        err "未找到 dylibbundler，安装：brew install dylibbundler"
        exit 1
    fi

    rm -rf "$CACHE_DIR"
    mkdir -p "$CACHE_DIR/bin" "$CACHE_DIR/lib"
    cp "$LLDB_DAP" "$CACHE_DIR/bin/lldb-dap"
    chmod +x "$CACHE_DIR/bin/lldb-dap"

    SEARCH_ARGS=()
    for SUB in lldb llvm xz z3 zstd; do
        if [ -d "$BREW_PREFIX/opt/$SUB/lib" ]; then
            SEARCH_ARGS+=("-s" "$BREW_PREFIX/opt/$SUB/lib")
        fi
    done

    LOGFILE="$(mktemp /tmp/dylibbundler.XXXXXX.log)"

    if ! dylibbundler -of -b \
        -x "$CACHE_DIR/bin/lldb-dap" \
        -d "$CACHE_DIR/lib" \
        -p "@loader_path/../lib" \
        "${SEARCH_ARGS[@]}" \
        -ns \
        > "$LOGFILE" 2>&1
    then
        err "dylibbundler 失败，日志："
        cat "$LOGFILE"
        rm -f "$LOGFILE"
        exit 1
    fi
    rm -f "$LOGFILE"

    {
        echo "LLVM version: $LLVM_VERSION"
        echo "Architecture: $ARCH"
        echo "OS:           $OS"
        echo "Prepared:     $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Source:       $LLDB_DAP"
    } > "$CACHE_DIR/VERSION.txt"

    info "缓存已生成"
fi

# ------------------------------------------------------------
# 6. 内嵌到 app bundle
# ------------------------------------------------------------
info "内嵌 lldb 到 app bundle..."
rm -rf "$BUNDLE_APP/Contents/Frameworks/lldb"
mkdir -p "$BUNDLE_APP/Contents/Frameworks"
cp -R "$CACHE_DIR" "$BUNDLE_APP/Contents/Frameworks/lldb"
chmod +x "$BUNDLE_APP/Contents/Frameworks/lldb/bin/lldb-dap"

LIB_COUNT="$(ls -1 "$BUNDLE_APP/Contents/Frameworks/lldb/lib"/*.dylib 2>/dev/null | wc -l | tr -d ' ')"
LIB_SIZE="$(du -sh "$BUNDLE_APP/Contents/Frameworks/lldb/lib" 2>/dev/null | cut -f1)"
info "  已内嵌 $LIB_COUNT 个 dylib，共 $LIB_SIZE"

# ------------------------------------------------------------
# 7. 签名
# ------------------------------------------------------------
info "清理扩展属性..."
xattr -cr "$BUNDLE_APP" 2>/dev/null || true

info "重新签名（30-60 秒）..."
codesign --force --deep --sign - "$BUNDLE_APP"

info "验证签名..."
if codesign --verify --deep --strict "$BUNDLE_APP"; then
    echo "[package] 签名验证通过"
else
    err "签名验证失败"
    exit 1
fi

# ------------------------------------------------------------
# 8. 完成
# ------------------------------------------------------------
TOTAL_SIZE="$(du -sh "$BUNDLE_APP" | cut -f1)"

echo ""
echo "[package] ========== 完成 =========="
echo "[package]  产物:   $BUNDLE_APP"
echo "[package]  体积:   $TOTAL_SIZE"
echo "[package]  架构:   $ARCH"
echo "[package]  LLVM:   $LLVM_VERSION"
echo "[package] =========================="