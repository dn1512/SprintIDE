#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tr_wrap.py — 批量给 C++ 源文件里的中文裸字符串字面量套上 tr()

设计原则：
  - 只处理"最裸"的字符串字面量，例如 QMessageBox::information(this, "提示", "内容")
  - 已经包在 tr() / QStringLiteral() / QString() / 日志函数 里的一律跳过
  - 不改注释、不改字符字面量（'中'）、不改 UTF/raw 前缀字符串（u8".." / R"(..)"）
  - 默认写 .bak 备份，方便回滚

用法：
    # 预览（只打印，不写文件）
    python3 tools/tr_wrap.py --dry-run mainwindow.cpp settings_dialog.cpp

    # 原地修改（.bak 备份）
    python3 tools/tr_wrap.py mainwindow.cpp settings_dialog.cpp

    # 递归处理整个目录下的 .cpp / .h / .hpp
    python3 tools/tr_wrap.py -r src/
"""

import argparse
import os
import re
import shutil
import sys


# ============================================================
#  字符分类
# ============================================================
CJK_RE = re.compile(r'[\u4e00-\u9fff\u3400-\u4dbf\uf900-\ufaff]')


def has_cjk(s: str) -> bool:
    return bool(CJK_RE.search(s))


# ============================================================
#  跳过名单
# ============================================================
# 这些函数内部的字面量不翻译（日志 / 调试 / AI 上下文），因为它们
# 会进入 LogBuffer、错误比对、AI 消息，翻译后反而破坏正则匹配
SKIP_CALL_RE = re.compile(
    r'\b('
    r'logInfo|logWarn|logError|appendLog|'
    r'qDebug|qWarning|qCritical|qInfo|qFatal|'
    r'qCDebug|qCWarning|qCCritical|qCInfo'
    r')\s*\('
)

# 这些标识符紧跟括号时跳过（已经包了一层，外面再包 tr() 会改变语义）
SKIP_IDENTS = {
    'QStringLiteral', 'QString', 'QStringView',
    'fromUtf8', 'fromLocal8Bit', 'fromLatin1', 'fromUtf16',
    'setObjectName', 'setProperty',
}


# ============================================================
#  单行扫描：找出字符串字面量位置
# ============================================================
def find_string_literals(line: str):
    """返回 [(start, end), ...]，含首尾引号。注释、字符字面量会被正确跳过。"""
    results = []
    n = len(line)
    i = 0
    while i < n:
        c = line[i]
        # 行注释 //
        if c == '/' and i + 1 < n and line[i + 1] == '/':
            break
        # 块注释 /* ... */
        if c == '/' and i + 1 < n and line[i + 1] == '*':
            end = line.find('*/', i + 2)
            if end == -1:
                break
            i = end + 2
            continue
        # 字符字面量 '...'
        if c == "'":
            j = i + 1
            while j < n:
                if line[j] == '\\':
                    j += 2
                    continue
                if line[j] == "'":
                    break
                j += 1
            i = j + 1
            continue
        # 字符串字面量 "..."
        if c == '"':
            j = i + 1
            while j < n:
                if line[j] == '\\':
                    j += 2
                    continue
                if line[j] == '"':
                    break
                j += 1
            if j < n:
                results.append((i, j + 1))
                i = j + 1
            else:
                break
            continue
        i += 1
    return results


# ============================================================
#  上下文判断
# ============================================================
def preceding_ident(prefix: str):
    """若 prefix 以 `ident(` 结尾则返回 ident；否则返回 None。"""
    i = len(prefix) - 1
    while i >= 0 and prefix[i] in ' \t':
        i -= 1
    if i < 0 or prefix[i] != '(':
        return None
    i -= 1
    while i >= 0 and prefix[i] in ' \t':
        i -= 1
    end = i + 1
    while i >= 0 and (prefix[i].isalnum() or prefix[i] in '_:'):
        i -= 1
    ident = prefix[i + 1:end]
    return ident if ident else None


def is_tr_call(prefix: str) -> bool:
    ident = preceding_ident(prefix)
    if ident is None:
        return False
    return ident == 'tr' or ident.endswith('::tr')


def has_string_prefix(prefix: str) -> bool:
    """检测 prefix 结尾是否是 u8 / u / U / L / R 前缀。"""
    for pfx in ('u8', 'u', 'U', 'L', 'R'):
        if prefix.endswith(pfx):
            before = len(prefix) - len(pfx)
            if before == 0 or not (
                prefix[before - 1].isalnum() or prefix[before - 1] == '_'
            ):
                return True
    return False


# ============================================================
#  单行处理
# ============================================================
def process_line(line: str, warns=None):
    literals = find_string_literals(line)
    if not literals:
        return line

    result = line
    # 从后往前替换：替换后面的字符串不会影响前面字符串的 start/end
    for start, end in reversed(literals):
        content = line[start:end]
        inner = content[1:-1]

        if not has_cjk(inner):
            continue

        prefix = line[:start]

        # 已经是 tr("...") —— 跳过
        if is_tr_call(prefix):
            continue

        # u8"..." / R"(...)" —— 跳过
        if has_string_prefix(prefix):
            if warns is not None:
                warns.append(f"  [skip] 字符串前缀: {content[:40]}")
            continue

        # QStringLiteral("...") / fromLocal8Bit("...") —— 跳过
        ident = preceding_ident(prefix)
        if ident in SKIP_IDENTS:
            if warns is not None:
                warns.append(f"  [skip] 已被 {ident}(...) 包裹: {content[:40]}")
            continue

        # 处理！包上 tr()
        result = result[:start] + 'tr(' + content + ')' + result[end:]

    return result


# ============================================================
#  单文件处理
# ============================================================
def process_file(path: str, dry_run: bool, use_backup: bool = True) -> bool:
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # 跨行状态：追踪 logInfo( 等多行调用的括号深度
    skip_call = None
    skip_depth = 0

    warnings = []
    modified_lines = []

    for lineno, raw_line in enumerate(lines, 1):
        line_skip = False

        # 状态：本行开始前已经在 logXxx( 内部
        if skip_call is None:
            m = SKIP_CALL_RE.search(raw_line)
            if m:
                skip_call = m.group(1)
                tail = raw_line[m.end():]
                skip_depth = 1 + tail.count('(') - tail.count(')')
                line_skip = True
        else:
            line_skip = True
            skip_depth += raw_line.count('(') - raw_line.count(')')

        if line_skip:
            new_line = raw_line
        else:
            new_line = process_line(raw_line, warnings)

        if new_line != raw_line:
            modified_lines.append((lineno, raw_line, new_line))

        # 括号归零 → 清除 skip 状态
        if skip_call is not None and skip_depth <= 0:
            skip_call = None
            skip_depth = 0

    if not modified_lines:
        return False

    if dry_run:
        print(f"\n===== {path} =====")
        for lineno, old, new in modified_lines:
            print(f"@@ line {lineno}")
            print(f"- {old.rstrip()}")
            print(f"+ {new.rstrip()}")
    else:
        if use_backup:
            bak = path + '.bak'
            if not os.path.exists(bak):
                shutil.copy2(path, bak)
        new_lines = list(lines)
        for lineno, old, new in modified_lines:
            new_lines[lineno - 1] = new
        with open(path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        print(f"[OK] {path}: 修改 {len(modified_lines)} 处")
        if use_backup:
            print(f"     备份: {path}.bak")

    if warnings:
        for w in warnings:
            print(w, file=sys.stderr)

    return True


# ============================================================
#  CLI
# ============================================================
def main():
    ap = argparse.ArgumentParser(
        description='批量给 C++ 源文件里的中文裸字符串套上 tr()'
    )
    ap.add_argument('paths', nargs='+', help='要处理的文件或目录')
    ap.add_argument('--dry-run', action='store_true',
                    help='只打印差异，不修改文件')
    ap.add_argument('-r', '--recursive', action='store_true',
                    help='递归处理目录下的 .cpp/.h/.hpp 等')
    ap.add_argument('--no-backup', action='store_true',
                    help='不写 .bak 备份文件')
    ap.add_argument('--ext', default='.cpp,.h,.hpp,.cc,.cxx,.hh',
                    help='递归时处理的扩展名（逗号分隔）')
    args = ap.parse_args()

    exts = tuple(e.strip() for e in args.ext.split(',') if e.strip())

    targets = []
    for p in args.paths:
        if os.path.isdir(p):
            if not args.recursive:
                print(f"[WARN] 跳过目录 {p}（未指定 --recursive）",
                      file=sys.stderr)
                continue
            for root, _, files in os.walk(p):
                for fn in files:
                    if fn.endswith(exts):
                        targets.append(os.path.join(root, fn))
        elif os.path.isfile(p):
            targets.append(p)
        else:
            print(f"[WARN] 路径不存在: {p}", file=sys.stderr)

    if not targets:
        print("没有匹配的文件。", file=sys.stderr)
        sys.exit(1)

    total_changed = 0
    for path in targets:
        if process_file(path,
                        dry_run=args.dry_run,
                        use_backup=not args.no_backup):
            total_changed += 1

    print(f"\n共处理 {len(targets)} 个文件，修改 {total_changed} 个。")


if __name__ == '__main__':
    main()