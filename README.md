# 北京理工大学小学期实训 Qt 项目集合

本仓库统一保存小学期实训的两个 Qt 6 小项目。两个项目相互独立，也可以从仓库根目录一次性构建和测试。

## 项目目录

| 项目 | 目录 | 主要内容 |
| --- | --- | --- |
| 小项目 1：简易财务计算器 | `project1-financial-calculator/` | 四则运算、单利/复利、参数设置对话框、鼠标拖拽输入、Qt Test |
| 小项目 2：Socket 文件传输工具 | `project2-file-transfer/` | TCP 客户端/服务端、文本与图片传输、分包粘包处理、拖拽发送、进度和日志 |

每个项目目录都保留了自己的 `.pro`、`CMakeLists.txt`、`.ui`、`.qrc`、源码、测试和说明文档，可在 Qt Creator 中单独打开。

## Ubuntu 统一构建与测试

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

生成的程序：

```text
build/project1-financial-calculator/financial_calculator
build/project2-file-transfer/file_transfer_tool
```

SSH 无图形环境可执行启动检查：

```bash
QT_QPA_PLATFORM=offscreen ./build/project1-financial-calculator/financial_calculator --smoke-test
QT_QPA_PLATFORM=offscreen ./build/project2-file-transfer/file_transfer_tool --smoke-test
```

## Qt Creator

- 统一构建：打开仓库根目录的 `CMakeLists.txt`。
- 单独打开项目 1：打开 `project1-financial-calculator/FinancialCalculator.pro`。
- 单独打开项目 2：打开 `project2-file-transfer/FileTransferTool.pro`。

详细功能和操作步骤请阅读两个子目录中的 `README.md`。
