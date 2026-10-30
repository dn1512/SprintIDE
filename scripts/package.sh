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
# 7.5 打包 DMG
# ------------------------------------------------------------
APP_NAME_FOR_DMG="SprintIDE"
APP_VERSION="1.0.0"                      # 与 CMakeLists 保持一致
DMG_NAME="${APP_NAME_FOR_DMG}-${APP_VERSION}.dmg"
DMG_PATH="$PROJECT_ROOT/build/$DMG_NAME"
DMG_TMP="$PROJECT_ROOT/build/${APP_NAME_FOR_DMG}-${APP_VERSION}.tmp.dmg"
STAGING="$PROJECT_ROOT/build/dmg-staging"
MOUNT_POINT="/Volumes/$APP_NAME_FOR_DMG"

info "准备 DMG 打包..."
info "  产物: $DMG_PATH"

# ---- 清理可能残留的挂载 ----
if [ -d "$MOUNT_POINT" ]; then
    info "  发现残留挂载点，先卸载: $MOUNT_POINT"
    hdiutil detach "$MOUNT_POINT" -force -quiet 2>/dev/null || true
fi

# ---- staging ----
rm -rf "$STAGING"
mkdir -p "$STAGING"

# 必须 cp -R：保留 .app 内 Framework 的符号链接
cp -R "$BUNDLE_APP" "$STAGING/"

# Applications 快捷方式（拖拽安装）
ln -s /Applications "$STAGING/Applications"

# 清理 macOS 元数据
find "$STAGING" -name '.DS_Store' -delete 2>/dev/null || true
find "$STAGING" -name '._*'       -delete 2>/dev/null || true

# ---- 生成可读写临时 dmg ----
rm -f "$DMG_TMP"
info "  生成临时 DMG..."
hdiutil create \
    -volname "$APP_NAME_FOR_DMG" \
    -srcfolder "$STAGING" \
    -ov -format UDRW \
    -quiet \
    "$DMG_TMP"

# ---- Finder 窗口布局（CI 里设 SKIP_DMG_LAYOUT=1 跳过）----
if [ "${SKIP_DMG_LAYOUT:-0}" != "1" ]; then
    info "  挂载并设置窗口布局..."

    if hdiutil attach "$DMG_TMP" -mountpoint "$MOUNT_POINT" -nobrowse -quiet; then
        osascript <<EOT || info "  (AppleScript 布局跳过：可能无 Finder 会话)"
tell application "Finder"
    tell disk "$APP_NAME_FOR_DMG"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 900, 420}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 128
        set position of item "${APP_NAME_FOR_DMG}.app" of container window to {150, 170}
        set position of item "Applications"            of container window to {400, 170}
        close
        open
        update without registering applications
        delay 1
    end tell
end tell
EOT
        sync

        # detach 有时 busy 重试 3 次，最后强制
        for i in 1 2 3; do
            if hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null; then
                break
            fi
            info "  卸载失败，重试 ($i/3)..."
            sleep 2
        done
        if [ -d "$MOUNT_POINT" ]; then
            hdiutil detach "$MOUNT_POINT" -force -quiet || true
        fi
    else
        info "  attach 失败，跳过布局（dmg 内容仍正常）"
    fi
else
    info "  跳过 Finder 布局（SKIP_DMG_LAYOUT=1）"
fi

# ---- 压缩为只读 dmg ----
info "  压缩为只读 DMG（zlib-level=9）..."
rm -f "$DMG_PATH"
hdiutil convert "$DMG_TMP" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -o "$DMG_PATH" \
    -quiet

# ---- 清理 ----
rm -f "$DMG_TMP"
rm -rf "$STAGING"

if [ -f "$DMG_PATH" ]; then
    DMG_SIZE="$(du -sh "$DMG_PATH" | cut -f1)"
    info "  DMG 已生成: $DMG_PATH  大小: $DMG_SIZE"
else
    err "DMG 生成失败"
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
echo "[package]  DMG:    $DMG_PATH"
echo "[package]  DMG 大小: $DMG_SIZE"
echo "[package]  架构:   $ARCH"
echo "[package]  LLVM:   $LLVM_VERSION"
echo "[package] =========================="