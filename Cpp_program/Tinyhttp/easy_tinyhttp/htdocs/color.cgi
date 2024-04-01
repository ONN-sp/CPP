#!/bin/bash

# 获取颜色参数
color="$QUERY_STRING"

# 输出 HTTP 头部
echo "Content-Type: text/html"
echo

# 输出 HTML 页面
echo "<html><body style=\"background-color: $color;\">"
echo "<center>This is red:</center>"
echo "<center><b>"
date
echo "</b></center>"
echo "</body></html>"
