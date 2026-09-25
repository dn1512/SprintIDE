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
# 6.5 内嵌 cmake + ninja 到 Frameworks/toolchain
# ------------------------------------------------------------
info "内嵌 cmake + ninja 到 app bundle..."

TOOLCHAIN_DIR="$BUNDLE_APP/Contents/Frameworks/toolchain"
rm -rf "$TOOLCHAIN_DIR"
mkdir -p "$TOOLCHAIN_DIR/bin" "$TOOLCHAIN_DIR/share"

# --- ninja（单文件，无第三方依赖）---
NINJA_SRC=""
for P in \
    "$BREW_PREFIX/bin/ninja" \
    "/usr/local/bin/ninja" \
    "/opt/homebrew/bin/ninja" \
    "/usr/bin/ninja"
do
    if [ -x "$P" ]; then
        NINJA_SRC="$P"
        break
    fi
done

if [ -z "$NINJA_SRC" ] && command -v ninja >/dev/null 2>&1; then
    NINJA_SRC="$(command -v ninja)"
fi

if [ -z "$NINJA_SRC" ]; then
    err "未找到 ninja"
    err "安装：brew install ninja"
    exit 1
fi
info "ninja: $NINJA_SRC"

# 跟随符号链接，直接拷真实文件（ninja 本身是静态单文件）
cp -L "$NINJA_SRC" "$TOOLCHAIN_DIR/bin/ninja"
chmod +x "$TOOLCHAIN_DIR/bin/ninja"

NINJA_VER="$("$TOOLCHAIN_DIR/bin/ninja" --version 2>&1 | head -1)"
NINJA_SIZE="$(du -sh "$TOOLCHAIN_DIR/bin/ninja" | cut -f1)"
info "  ninja 版本: $NINJA_VER  大小: $NINJA_SIZE"

# --- cmake（二进制 + share/cmake 数据目录）---
# 优先使用 brew 的独立命令行 formula（/usr/local/opt/cmake），
# 不要用 /Applications/CMake.app（GUI bundle，结构复杂且升级不受 brew 控制）
CMAKE_SRC=""
for P in \
    "$BREW_PREFIX/opt/cmake/bin/cmake" \
    "/usr/local/opt/cmake/bin/cmake" \
    "/opt/homebrew/opt/cmake/bin/cmake"
do
    if [ -x "$P" ]; then
        CMAKE_SRC="$P"
        break
    fi
done

if [ -z "$CMAKE_SRC" ]; then
    err "未找到 brew 版 cmake"
    err "安装：brew install cmake"
    err "（不要用 /Applications/CMake.app 内嵌，升级不可控）"
    exit 1
fi
info "cmake: $CMAKE_SRC"

cp -L "$CMAKE_SRC" "$TOOLCHAIN_DIR/bin/cmake"
chmod +x "$TOOLCHAIN_DIR/bin/cmake"

# --- cmake 数据目录 ---
# cmake 会从 <binary_dir>/../share/cmake 推断 CMAKE_ROOT，
# 所以内嵌结构必须保持 bin/ 与 share/ 同级。
CMAKE_SHARE=""
for CAND in \
    "$BREW_PREFIX/opt/cmake/share/cmake" \
    "/usr/local/opt/cmake/share/cmake" \
    "/opt/homebrew/opt/cmake/share/cmake"
do
    if [ -d "$CAND" ]; then
        CMAKE_SHARE="$CAND"
        break
    fi
done

if [ -z "$CMAKE_SHARE" ]; then
    err "找不到 cmake 的 share/cmake 数据目录"
    err "已尝试：\$BREW_PREFIX/opt/cmake/share/cmake 等"
    exit 1
fi
info "cmake share: $CMAKE_SHARE"

# 实测大小（交接文档留的待办）
CMAKE_SHARE_SIZE_RAW="$(du -sh "$CMAKE_SHARE" | cut -f1)"
info "  share/cmake 大小: $CMAKE_SHARE_SIZE_RAW"

cp -R "$CMAKE_SHARE" "$TOOLCHAIN_DIR/share/cmake"

# 只保留 cmake 一个可执行，ccmake/cpack/ctest 省略（省约 45 MB）
# （本脚本本来就只拷了 cmake）

# --- 验证内嵌结果 ---
if ! "$TOOLCHAIN_DIR/bin/cmake" --version >/dev/null 2>&1; then
    err "内嵌 cmake 无法运行（CMAKE_ROOT 推断可能失败）"
    err "检查：ls $TOOLCHAIN_DIR/share/cmake/Modules"
    exit 1
fi

CMAKE_IN_BUNDLE_VER="$("$TOOLCHAIN_DIR/bin/cmake" --version | head -1)"
info "  $CMAKE_IN_BUNDLE_VER"

TOOLCHAIN_SIZE="$(du -sh "$TOOLCHAIN_DIR" | cut -f1)"
info "  toolchain 总大小: $TOOLCHAIN_SIZE"
info "  结构:"
(cd "$TOOLCHAIN_DIR" && find . -maxdepth 2 -type d | sed 's|^|    |' | head -10)

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