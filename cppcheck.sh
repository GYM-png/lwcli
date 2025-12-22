#!/bin/bash
# run_cppcheck_with_html.sh - 生成 HTML 报告版本

PROJECT_ROOT="$(pwd)"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_DIR="${PROJECT_ROOT}/cppcheck_reports_${TIMESTAMP}"
HTML_DIR="${REPORT_DIR}/html"
XML_FILE="${REPORT_DIR}/report.xml"
TEXT_FILE="${REPORT_DIR}/report.txt"

mkdir -p "$REPORT_DIR"
mkdir -p "$HTML_DIR"

echo "开始分析并生成 HTML 报告..."

# 1. 生成 XML 报告
cppcheck \
  --enable=warning,performance,portability,style \
  --inconclusive \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  -i "example" \
  -I . \
  -I src \
  -I inc \
  --std=c99 \
  --platform=unix32 \
  --language=c \
  --force \
  --xml \
  --xml-version=2 \
  src \
  inc \
  2> "$XML_FILE"

# 2. 生成文本报告
cppcheck \
  --enable=warning,performance,portability,style \
  --inconclusive \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  -i "example" \
  -I . \
  -I src \
  -I inc \
  --std=c99 \
  --platform=unix32 \
  --language=c \
  --force \
  --template='[{file}:{line}]: ({severity}) {message}' \
  src \
  inc \
  2> "$TEXT_FILE"

# 3. 生成 HTML 报告（如果安装了 cppcheck-htmlreport）
if command -v cppcheck-htmlreport > /dev/null 2>&1; then
    echo "生成 HTML 报告..."
    cppcheck-htmlreport \
        --title="m3c-2.0 静态分析报告" \
        --file="$XML_FILE" \
        --report-dir="$HTML_DIR"
    
    echo "✅ HTML 报告已生成: $HTML_DIR/index.html"
else
    echo "⚠️  未安装 cppcheck-htmlreport，跳过 HTML 报告生成"
    echo "   安装命令: pip install cppcheck-htmlreport"
fi

echo ""
echo "报告文件:"
echo "  - 文本报告: $TEXT_FILE"
echo "  - XML 报告: $XML_FILE"
if [ -f "$HTML_DIR/index.html" ]; then
    echo "  - HTML 报告: $HTML_DIR/index.html"
fi