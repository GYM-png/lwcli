#!/bin/bash

# lwcli Doxygen 文档生成脚本
# 作者: GYM
# 版本: V0.0.4

# 严格模式，但允许警告
set -eo pipefail

echo "========================================"
echo "   lwcli Doxygen 文档生成工具"
echo "========================================"
echo ""

# 检查 doxygen 是否安装
if ! command -v doxygen &> /dev/null; then
    echo "❌ 错误: doxygen 未安装"
    echo ""
    echo "请使用以下命令安装:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "  CentOS/RHEL:   sudo yum install doxygen graphviz"
    echo "  macOS:         brew install doxygen graphviz"
    echo ""
    exit 1
fi

DOXYGEN_VERSION=$(doxygen --version)
echo "✅ 检测到 doxygen 版本: $DOXYGEN_VERSION"

# 检查 graphviz (dot) 是否安装
if ! command -v dot &> /dev/null; then
    echo "⚠️  警告: graphviz (dot) 未安装，图形输出将被禁用"
    echo "  建议安装: sudo apt-get install graphviz"
    HAS_DOT=NO
else
    echo "✅ 检测到 graphviz"
    HAS_DOT=YES
fi

echo ""

# 清理旧文档
echo "📁 清理旧文档..."
if [ -d "docs" ]; then
    rm -rf docs
    echo "   已删除旧文档目录"
fi

# 生成文档
echo ""
echo "📝 正在生成文档..."
echo "   这可能需要几秒钟..."
echo ""

# 创建临时配置文件以过滤警告
TEMP_CONFIG=$(mktemp)

if [ "$HAS_DOT" = "NO" ]; then
    # 临时禁用 dot
    sed 's/^HAVE_DOT.*$/HAVE_DOT = NO/' Doxyfile > "$TEMP_CONFIG"
else
    cp Doxyfile "$TEMP_CONFIG"
fi

# 过滤掉已废弃标签的警告，只保留重要信息
if doxygen "$TEMP_CONFIG" 2>&1 | grep -v "^warning: Tag.*has become obsolete" | grep -v "^warning: The selected output language" | tee /tmp/doxygen_output.log; then
    echo ""
    echo "========================================"
    echo "   ✅ 文档生成成功!"
    echo "========================================"
    echo ""
else
    echo ""
    echo "========================================"
    echo "   ❌ 文档生成失败!"
    echo "========================================"
    echo ""
    echo "错误信息:"
    cat /tmp/doxygen_output.log | grep -i "error\|fatal" || echo "   未知错误"
    rm -f "$TEMP_CONFIG"
    exit 1
fi

rm -f "$TEMP_CONFIG"

# 统计生成的文件
FILE_COUNT=$(find docs/html -type f 2>/dev/null | wc -l)
echo "📊 生成了 $FILE_COUNT 个文件"
echo ""
echo "📂 文档位置: docs/html/index.html"
echo ""
echo "🌐 查看方式:"
echo ""
echo "   方法1 - 直接打开:"
echo "     xdg-open docs/html/index.html"
echo ""
echo "   方法2 - 本地服务器:"
echo "     cd docs/html && python3 -m http.server 8080"
echo "     浏览器访问: http://localhost:8080"
echo ""
echo "   方法3 - 命令行浏览器:"
echo "     lynx docs/html/index.html"
echo ""

# 检查是否有重要警告
WARN_COUNT=$(grep -c "^warning:" /tmp/doxygen_output.log 2>/dev/null || true)
WARN_COUNT=${WARN_COUNT:-0}
if [ "$WARN_COUNT" -gt 0 ]; then
    echo "⚠️  提示: 发现 $WARN_COUNT 个警告 (不影响使用)"
    echo "   如需查看警告: cat /tmp/doxygen_output.log"
    echo ""
fi

rm -f /tmp/doxygen_output.log
