# VoicePhone (Qt6) — Discord-like Voice Chat

这是一个用 C++ 和 Qt6 实现的类似 Discord 的语音聊天应用，采用前后端分离架构。

## 功能特性

- 🎙️ **实时语音通话** - 基于 UDP 的低延迟音频传输
- 🏠 **多频道支持** - 类似 Discord，支持多个语音频道
- 👥 **用户管理** - 显示在线用户列表，实时更新加入/离开状态
- 🔌 **前后端分离** - 独立的服务器和客户端程序
- 🌐 **多客户端支持** - 服务器可同时处理多个客户端连接

## 架构设计

### 后端服务器 (voicephone-server)
- **控制端口 (TCP)**: 处理客户端连接、频道管理、用户状态
- **语音端口 (UDP)**: 转发音频数据包到同频道的其他用户
- **默认端口**: 控制端口 8888, 语音端口 8889

### 前端客户端 (voicephone)
- **Qt Widgets UI**: 提供图形界面
- **网络客户端**: 通过 TCP 与服务器通信
- **音频引擎**: 使用 QAudioSource/QAudioSink 捕获和播放音频
- **UDP 通信**: 发送/接收音频数据

## 构建与运行

### 依赖项:

- Qt 6 (Core, Widgets, Multimedia, Network)
- Opus 音频编解码库
- CMake 3.16+
- C++17 编译器

### 安装依赖 (Ubuntu/Debian):

```bash
sudo apt-get install qt6-base-dev qt6-multimedia-dev libopus-dev cmake
```

### 使用系统已安装的 Qt6:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 运行服务器:

```bash
# 使用默认端口
./bin/voicephone-server

# 或指定端口
./bin/voicephone-server --control-port 8888 --voice-port 8889
```

### 运行客户端:

```bash
./bin/voicephone
```

## 使用方法

1. **启动服务器** - 在一台主机上运行服务器
2. **连接客户端** - 在一个或多个客户端输入服务器 IP 和端口，填写用户名，点击"Connect"
3. **加入频道** - 从频道列表中选择一个频道，点击"Join"
4. **开始通话** - 加入同一频道的用户可以互相通话
5. **离开频道** - 点击"Leave"退出当前频道

## 技术栈

- **语言**: C++17
- **框架**: Qt 6 (Core, Widgets, Multimedia, Network)
- **音频编解码**: Opus (24kbps, 48kHz)
- **音频格式**: PCM, 48kHz, 单声道, 16-bit (编码前/解码后)
- **通信协议**: 
  - TCP (控制消息，JSON 格式)
  - UDP (Opus编码的音频数据)

## 消息协议

### 客户端 → 服务器

```json
// 登录
{"type": "login", "username": "用户名", "udp_ip": "IP", "udp_port": 端口}

// 加入频道
{"type": "join_channel", "channel": "频道名"}

// 离开频道
{"type": "leave_channel"}

// 获取频道列表
{"type": "get_channels"}
```

### 服务器 → 客户端

```json
// 登录成功
{"type": "login_success", "voice_port": 端口}

// 频道列表
{"type": "channel_list", "channels": [{"name": "频道名", "user_count": 数量}]}

// 用户列表
{"type": "user_list", "channel": "频道名", "users": ["用户1", "用户2"]}

// 用户加入
{"type": "user_joined", "username": "用户名"}

// 用户离开
{"type": "user_left", "username": "用户名"}
```

## 目录结构

```
voicePhone/
├── server/              # 服务器代码
│   ├── main.cpp        # 服务器主程序
│   ├── server.h        # 服务器类头文件
│   └── server.cpp      # 服务器实现
├── client/             # 客户端网络层
│   ├── networkclient.h # 网络客户端头文件
│   └── networkclient.cpp
├── src/                # 客户端 UI 和音频
│   ├── main.cpp        # 客户端主程序
│   ├── mainwindow.h/cpp/ui # 主窗口
│   ├── audioengine.h/cpp   # 音频引擎
│   └── opuscodec.h/cpp     # Opus编解码器
├── CMakeLists.txt      # CMake 构建配置
└── README.md           # 本文件
```

## 注意事项

⚠️ 这是一个演示级别的实现，未包含以下生产环境所需特性：

- 抖动缓冲（jitter buffer）
- 带宽控制和自适应比特率
- NAT 穿透
- 回声消除
- 加密和身份验证
- 持久化存储

### 已实现特性

✅ **音频编解码** - 使用 Opus 编解码器，提供高质量低延迟的音频压缩（24kbps @ 48kHz）

## 许可证

本项目仅供学习和演示使用。
