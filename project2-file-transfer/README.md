# Socket 文件传输工具（项目 2）

本项目使用 Qt 6 Widgets 与 Qt Network，实现一个可切换“客户端/服务端”角色的 TCP 文件传输工具。客户端和服务端运行同一个程序，通过连接设置对话框选择角色、IP、端口和接收目录。

## 功能

- `QTcpServer` 监听与 `QTcpSocket` 异步连接。
- 文本及 PNG/JPG/BMP 等二进制文件传输。
- 自定义协议头：魔数、版本、消息类型、UTF-8 文件名长度、文件大小和文件数据。
- 接收端按状态机增量解析，正确处理 TCP 分包与粘包，大文件不一次性读入内存。
- 发送端通过 `bytesWritten` 和限流缓冲区分块发送。
- 发送/接收进度、连接状态、文件名、文件大小和错误日志。
- Qt Designer `.ui` 主窗口与连接设置对话框，参数通过自定义信号传回主窗口。
- 拖拽区重写 `dragEnterEvent`、`dragLeaveEvent`、`dropEvent`，拖入文件后自动发送。
- QSS 样式和 SVG 图标通过 `.qrc` 资源加载。
- Qt Test 自动完成本机客户端/服务端大文件完整性测试。

## 构建与测试

从小学期实训仓库根目录构建两个项目：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

只构建本项目：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=offscreen ./build/file_transfer_tool --smoke-test
```

也可在 Qt Creator 中直接打开 `FileTransferTool.pro`。

## 双端运行

1. 启动第一个程序，在“连接设置”中选择“服务端”，端口保留 `45454`，设置接收目录，点击“开始监听”。
2. 启动第二个程序，选择“客户端”，IP 填写服务端地址（同机测试用 `127.0.0.1`），端口保持一致，点击“连接服务器”。
3. 连接成功后选择文件并发送，或把文件拖进虚线框。接收文件会以原文件名保存到设置目录。

> 当前版本每个服务端实例同时服务一个客户端；收到第二个连接时会拒绝并写入日志。
