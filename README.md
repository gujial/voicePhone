# VoicePhone (Qt6)

这是一个用 C++ 和 Qt6 实现的类似 Discord 的语音聊天应用，采用前后端分离架构。

## 功能特性

- 🎙️ **实时语音通话** - 基于 UDP 的低延迟音频传输
- 🏠 **多频道支持** - 类似 Discord，支持多个语音频道
- 👥 **用户管理** - 显示在线用户列表，实时更新加入/离开状态
- 🔌 **前后端分离** - 独立的服务器和客户端程序
- 🌐 **多客户端支持** - 服务器可同时处理多个客户端连接
- 🔐 **加密通信** - AES-256加密保护TCP控制消息和UDP音频数据
- 🔑 **身份验证** - 基于用户名/密码的SHA-256哈希身份验证系统

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
- OpenSSL (用于加密功能)
- CMake 3.16+
- C++17 编译器

### 安装依赖 (Ubuntu/Debian):

```bash
sudo apt-get install qt6-base-dev qt6-multimedia-dev libopus-dev libssl-dev cmake
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

1. **注册或登录** - 首次使用勾选"New User (Register)"注册账号，之后直接登录
   - 默认测试账号: admin/user1/user2, 密码都是 "password"
3. **连接服务器** - 输入服务器 IP 和端口，填写用户名和密码，点击"Connect"
4. **加入频道** - 从频道列表中选择一个频道，点击"Join"
5. **开始通话** - 加入同一频道的用户可以互相通话（所有通信均已加密）
6. **开始通话** - 加入同一频道的用户可以互相通话
5. **离开频道** - 点击"Leave"退出当前频道

## 技术栈

- **语言**: C++17
- **框架**: Qt 6 (Core, Widgets, Multimedia, Network)
- **加密**: 
  - OpenSSL (AES-256-CBC for TCP, AES-256-CTR for UDP)
  - SHA-256 密码哈希
- **通信协议**: 
  - TCP (控制消息，JSON 格式，AES-256-CBC加密)
  - UDP (Opus编码的音频数据，AES-256-CTR加密
  - TCP (控制消息，JSON 格式)
  - UDP (Opus编码的音频数据)

## 消息协议

### 注册新用户

```json
{"type": "register", "username": "用户名", "password_hash": "SHA256哈希"}

// 登录
{"type": "login", "username": "用户名", "password_hash": "SHA256哈希", "udp_ip": "IP", "udp_port": 端口}

// 加入频道 (需要认证，已加密)
{"type": "join_channel", "channel": "频道名"}

// 离开频道 (需要认证，已加密)
{"type": "leave_channel"}

// 获取频道列表 (需要认证，已加密)
{"type": "get_channels"}
```

### 服务器 → 客户端

```json
// 注册成功
{"type": "register_success"}

// 登录成功
{"type": "login_success", "voice_port": 端口, "session_id": "会话ID", "session_key": "AES密钥(hex)"}

// 加入频道成功 (已加密)
{"type": "join_success", "channel": "频道名", "channel_key": "频道加密密钥(hex)"}

// 频道列表 (已加密)
{"type": "channel_list", "channels": [{"name": "频道名", "user_count": 数量}]}

// 用户列表 (已加密)
{"type": "user_list", "channel": "频道名", "users": ["用户1", "用户2"]}

// 用户加入 (已加密)
{"type": "user_joined", "username": "用户名"}

// 用户离开 (已加密)
{"type": "user_left", "username": "用户名"}

// 错误消息
{"type": "error", "message": "错误描述户名"}
```

### 已实现特性

✅ **音频编解码** - 使用 Opus 编解码器，提供高质量低延迟的音频压缩（24kbps @ 48kHz）

✅ **加密和身份验证** - 完整的加密通信系统
- **密码哈希**: SHA-256
- **TCP消息加密**: AES-256-CBC，带随机IV
- **UDP音频加密**: AES-256-CTR端到端加密，每个音频包包含计数器
- **会话管理**: 基于令牌的会话系统
- **用户认证**: 用户名/密码验证
- **频道密钥**: 每个频道独立的加密密钥，实现真正的端到端加密

## 安全特性

### 身份验证流程
1. 客户端连接服务器
2. 客户端发送用户名和SHA-256哈希密码
3. 服务器验证凭据
4. 验证成功后，服务器生成会话令牌和AES-256密钥
5. 后续所有通信使用会话密钥加密

### 加密实现
- **控制消息**: AES-256-CBC模端到端加密
  - 每个频道有独立的加密密钥
  - 每个音频包包含8字节计数器（nonce）
  - 服务器只转发加密数据，无法解密
  - 真正的端到端加密保护
- **密钥大小**: 256位（32字节）
- **会话隔离**: 每个会话使用独立的加密密钥
- **频道隔离**: 每个频道使用独立的音频）
- **会话隔离**: 每个会话使用独立的加密密钥

### 默认测试账号
用于开发测试（密码: "password"）:
- admin
- user1  
- user2
⚠️ 这是一个演示级别的实现，未包含以下生产环境所需特性：

- 抖动缓冲（jitter buffer）
- 带宽控制和自适应比特率
- NAT 穿透
- 回声消除

### 已实现特性

- 音频编解码
- 加密和身份验证
- 持久化存储

## 许可证

本项目仅供学习和演示使用。
