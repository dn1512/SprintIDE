#!/bin/bash
# ============================================================
# SprintIDE macOS 打包脚本（通用版 v4）
#
# 特性：
#   - 零硬编码路径（自动探测 Qt / Homebrew / LLVM）
#   - 架构感知（x86_64 / arm64）
#   - 预处理缓存（首次构建 lldb bundle，之后秒拷贝）
#   - 所有关键路径可通过环境变量覆盖
#   - 直接处理 build/SprintIDE.app，不再拷贝到别处
#
# 使用：
#   ./scripts/package.sh
#
# 环境变量（全部可选）：
#   SPRINTIDE_QT_PREFIX      Qt 安装路径
#   SPRINTIDE_LLDB_DAP       lldb-dap 可执行文件路径
#   SPRINTIDE_LLDB_BUNDLE    预处理 lldb 目录（跳过自动构建）
#   SPRINTIDE_SKIP_BUILD     设为 1 跳过 cmake 编译
#   SPRINTIDE_SKIP_DEPLOY    设为 1 跳过 macdeployqt
# ============================================================

set -euo pipefail

# ============================================================
# 1. 基础环境
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APP_NAME="SprintIDE"
APP_BUNDLE_NAME="$APP_NAME.app"

OS="$(uname -s)"
ARCH="$(uname -m)"

# ============================================================
# 2. 彩色日志
# ============================================================
if [ -t 1 ]; then
    C_BLUE="\033[1;34m"; C_GREEN="\033[1;32m"; C_YELLOW="\033[1;33m"
    C_RED="\033[1;31m"; C_RESET="\033[0m"
else
    C_BLUE=""; C_GREEN=""; C_YELLOW=""; C_RED=""; C_RESET=""
fi
info() { echo -e "${C_BLUE}[package]${C_RESET} $*"; }
ok()   { echo -e "${C_GREEN}[package]${C_RESET} $*"; }
warn() { echo -e "${C_YELLOW}[package]${C_RESET} $*"; }
err()  { echo -e "${C_RED}[package]${C_RESET} $*" >&2; }

# ============================================================
# 3. 探测函数
# ============================================================
require_macos() {
    if [ "$OS" != "Darwin" ]; then
        err "此脚本仅支持 macOS（当前: $OS）"
        exit 1
    fi
}

detect_brew_prefix() {
    if command -v brew >/dev/null 2>&1; then
        brew --prefix 2>/dev/null
    else
        [ "$ARCH" = "arm64" ] && echo "/opt/homebrew" || echo "/usr/local"
    fi
}

detect_qt_prefix() {
    if [ -n "${SPRINTIDE_QT_PREFIX:-}" ] && [ -d "$SPRINTIDE_QT_PREFIX" ]; then
        echo "$SPRINTIDE_QT_PREFIX"; return 0
    fi
    if command -v qmake >/dev/null 2>&1; then
        local p
        p="$(qmake -query QT_INSTALL_PREFIX 2>/dev/null || true)"
        [ -n "$p" ] && [ -d "$p" ] && { echo "$p"; return 0; }
    fi
    if command -v macdeployqt >/dev/null 2>&1; then
        local mp
        mp="$(command -v macdeployqt)"
        echo "$(cd "$(dirname "$mp")/.." && pwd)"
        return 0
    fi
    local CANDIDATES=()
    shopt -s nullglob
    CANDIDATES+=("$HOME/Qt"/*/macos)
    CANDIDATES+=("$HOME/Qt"/*/*/macos)
    CANDIDATES+=("/opt/Qt"/*/macos)
    CANDIDATES+=("$HOME/Library/Qt"/*/macos)
    shopt -u nullglob

    for d in "${CANDIDATES[@]}"; do
        if [ -d "$d" ] && [ -x "$d/bin/macdeployqt" ]; then
            echo "$d"; return 0
        fi
    done
    return 1
}

detect_lldb_dap() {
    if [ -n "${SPRINTIDE_LLDB_DAP:-}" ] && [ -x "$SPRINTIDE_LLDB_DAP" ]; then
        echo "$SPRINTIDE_LLDB_DAP"; return 0
    fi
    local brew_prefix
    brew_prefix="$(detect_brew_prefix)"
    for p in \
        "$brew_prefix/opt/lldb/bin/lldb-dap" \
        "$brew_prefix/opt/llvm/bin/lldb-dap" \
        "$brew_prefix/opt/llvm/bin/lldb-vscode" \
        "$brew_prefix/bin/lldb-dap"
    do
        [ -x "$p" ] && { echo "$p"; return 0; }
    done
    command -v lldb-dap >/dev/null 2>&1 && { command -v lldb-dap; return 0; }
    command -v lldb-vscode >/dev/null 2>&1 && { command -v lldb-vscode; return 0; }
    return 1
}

detect_llvm_version() {
    local lldb_dap="$1"
    if [ -x "$lldb_dap" ]; then
        local v
        v="$("$lldb_dap" --version 2>&1 | head -3 \
            | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
        [ -n "$v" ] && { echo "$v"; return; }
    fi
    echo "unknown"
}

get_lldb_bundle_dir() {
    if [ -n "${SPRINTIDE_LLDB_BUNDLE:-}" ]; then
        echo "$SPRINTIDE_LLDB_BUNDLE"
        return
    fi
    echo "$HOME/Library/Caches/SprintIDE/toolchains/lldb-${OS}-${ARCH}-${LLVM_VERSION}"
}

build_lldb_bundle() {
    local target_dir="$1"
    local lldb_dap="$2"
    local brew_prefix="$3"

    info "  构建 lldb 预处理目录（首次约 3-5 分钟）..."

    rm -rf "$target_dir"
    mkdir -p "$target_dir/bin" "$target_dir/lib"

    cp "$lldb_dap" "$target_dir/bin/lldb-dap"
    chmod +x "$target_dir/bin/lldb-dap"

    if ! command -v dylibbundler >/dev/null 2>&1; then
        err "未找到 dylibbundler"
        err "安装: brew install dylibbundler"
        exit 1
    fi

    local S_ARGS=()
    for sub in lldb llvm xz z3 zstd; do
        [ -d "$brew_prefix/opt/$sub/lib" ] && S_ARGS+=("-s" "$brew_prefix/opt/$sub/lib")
    done

    local log_file
    log_file="$(mktemp /tmp/dylibbundler.XXXXXX.log)"

    if ! dylibbundler -of -b \
        -x "$target_dir/bin/lldb-dap" \
        -d "$target_dir/lib" \
        -p "@loader_path/../lib" \
        "${S_ARGS[@]}" \
        -ns \
        > "$log_file" 2>&1; then
        err "dylibbundler 失败，日志："
        cat "$log_file"
        rm -f "$log_file"
        exit 1
    fi
    rm -f "$log_file"

    cat > "$target_dir/VERSION.txt" <<EOF
LLVM version:   $LLVM_VERSION
Architecture:   $ARCH
OS:             $OS
Prepared:       $(date '+%Y-%m-%d %H:%M:%S')
Source:         $lldb_dap
EOF

    ok "  预处理目录: $target_dir"
}

# ============================================================
# 4. 主流程
# ============================================================
require_macos

info "环境探测..."
info "  架构:      $ARCH"
info "  系统:      $OS"

BREW_PREFIX="$(detect_brew_prefix)"
ok "  Homebrew:  $BREW_PREFIX"

QT_PREFIX="$(detect_qt_prefix)" || {
    err "未找到 Qt 安装路径"
    err "请设置环境变量: SPRINTIDE_QT_PREFIX=/path/to/qt"
    exit 1
}
ok "  Qt:        $QT_PREFIX"

LLDB_DAP="$(detect_lldb_dap)" || {
    err "未找到 lldb-dap"
    err "安装: brew install lldb"
    exit 1
}
ok "  lldb-dap:  $LLDB_DAP"

LLVM_VERSION="$(detect_llvm_version "$LLDB_DAP")"
ok "  LLVM:      $LLVM_VERSION"

LLDB_BUNDLE="$(get_lldb_bundle_dir)"
if [ -d "$LLDB_BUNDLE" ] && [ -x "$LLDB_BUNDLE/bin/lldb-dap" ]; then
    ok "  预处理:    $LLDB_BUNDLE （缓存命中）"
else
    warn "  预处理:    未命中缓存，将自动构建"
    build_lldb_bundle "$LLDB_BUNDLE" "$LLDB_DAP" "$BREW_PREFIX"
fi

# ---- 编译 ----
if [ "${SPRINTIDE_SKIP_BUILD:-0}" != "1" ]; then
    info "编译 SprintIDE..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake -DCMAKE_PREFIX_PATH="$QT_PREFIX" "$PROJECT_ROOT"
    make -j"$(sysctl -n hw.ncpu)"
    ok "编译完成"
else
    info "跳过编译（SPRINTIDE_SKIP_BUILD=1）"
    cd "$BUILD_DIR"
fi

BUNDLE_APP="$BUILD_DIR/$APP_BUNDLE_NAME"
if [ ! -d "$BUNDLE_APP" ]; then
    err "未找到 app bundle: $BUNDLE_APP"
    exit 1
fi

# ---- Qt deploy ----
if [ "${SPRINTIDE_SKIP_DEPLOY:-0}" != "1" ]; then
    info "部署 Qt 依赖库..."
    MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
    if [ ! -x "$MACDEPLOYQT" ]; then
        err "未找到 macdeployqt: $MACDEPLOYQT"
        exit 1
    fi
    DEPLOY_LOG="$(mktemp /tmp/macdeployqt.XXXXXX.log)"
    if ! "$MACDEPLOYQT" "$BUNDLE_APP" -always-overwrite -verbose=2 \
        > "$DEPLOY_LOG" 2>&1; then
        err "macdeployqt 失败，日志："
        cat "$DEPLOY_LOG"
        rm -f "$DEPLOY_LOG"
        exit 1
    fi
    rm -f "$DEPLOY_LOG"
    ok "Qt 库已就位"
else
    info "跳过 Qt deploy（SPRINTIDE_SKIP_DEPLOY=1）"
fi

# ---- 内嵌 lldb ----
info "内嵌 lldb..."
rm -rf "$BUNDLE_APP/Contents/Frameworks/lldb"
mkdir -p "$BUNDLE_APP/Contents/Frameworks"
cp -R "$LLDB_BUNDLE" "$BUNDLE_APP/Contents/Frameworks/lldb"
chmod +x "$BUNDLE_APP/Contents/Frameworks/lldb/bin/lldb-dap"
DYLIB_COUNT=$(ls -1 "$BUNDLE_APP/Contents/Frameworks/lldb/lib"/*.dylib 2>/dev/null | wc -l | tr -d ' ')
DYLIB_SIZE=$(du -sh "$BUNDLE_APP/Contents/Frameworks/lldb/lib" 2>/dev/null | cut -f1)
ok "  已内嵌 $DYLIB_COUNT 个 dylib（共 $DYLIB_SIZE）"

# ---- 签名 ----
info "重新签名（30-60 秒）..."
xattr -cr "$BUNDLE_APP" 2>/dev/null || true
if ! codesign --force --deep --sign - "$BUNDLE_APP" 2>&1; then
    err "签名失败"
    exit 1
fi
ok "签名完成"

info "验证签名..."
if codesign --verify --deep --strict "$BUNDLE_APP" 2>&1; then
    ok "验证通过"
else
    err "签名验证失败"
    exit 1
fi

# ---- 完成 ----
SIZE=$(du -sh "$BUNDLE_APP" | cut -f1)
echo ""
ok "========== 打包完成 =========="
ok "  产物:      $BUNDLE_APP"
ok "  体积:      $SIZE"
ok "  架构:      $ARCH"
ok "  LLVM:      $LLVM_VERSION"
ok "  lldb-dap:  $BUNDLE_APP/Contents/Frameworks/lldb/bin/lldb-dap"
ok ""
ok "  启动:      open \"$BUNDLE_APP\""
ok "  分发:      直接把 build/SprintIDE.app 拷出来即可"
ok "=============================="