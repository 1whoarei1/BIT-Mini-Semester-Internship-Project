# 简易财务计算器（项目 1）

这是北京理工大学小学期实训的第一个 Qt 6 练习项目。程序在 Ubuntu 22.04 中开发和验证，使用 Qt Widgets、Qt Designer UI、Qt 资源系统、QSS、信号与槽以及鼠标拖拽事件。

## 已实现功能

- 加、减、乘、除，支持小数、括号和运算优先级。
- 清除、退格、回车计算和错误提示。
- 单利与复利计算。
- 财务参数设置对话框，通过自定义信号与槽把利率、年限传回主窗口。
- 数字与运算符支持点击输入，也支持拖拽到表达式输入框。
- 主窗口和设置窗口均由 Qt Designer 的 `.ui` 文件定义。
- QSS 和 SVG 图标由 `.qrc` 资源文件加载。
- 计算逻辑与界面代码分离，并提供 Qt Test 自动化测试。

## 单独构建与测试

```bash
cd project1-financial-calculator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=offscreen ./build/financial_calculator --smoke-test
```

## 在 Qt Creator 中运行

1. 在 Ubuntu 桌面打开 Qt Creator。
2. 选择“文件 → 打开文件或项目”。
3. 打开 `project1-financial-calculator/CMakeLists.txt` 或 `FinancialCalculator.pro`。
4. Kit 选择桌面 Qt 6.2.4，构建后运行 `financial_calculator`。

## 手工验收建议

1. 点击输入 `10 + 2 × 3`，结果应为 `16`。
2. 将数字和符号拖入输入框，释放后应追加字符。
3. 输入 `8 ÷ 0`，应看到错误提示且程序不崩溃。
4. 本金 10000、年利率 3%、3 年：单利应为 10900，复利应为 10927.27。
5. 打开“财务参数设置”，修改参数并确定，主窗口参数应同步更新。
