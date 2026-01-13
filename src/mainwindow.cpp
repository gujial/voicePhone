#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audioengine.h"
#include "../client/networkclient.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_audioEngine(new AudioEngine(this))
    , m_networkClient(new NetworkClient(this))
    , m_localVoicePort(0)
{
    ui->setupUi(this);

    // 设置默认值
    ui->serverIpEdit->setText("127.0.0.1");
    ui->serverPortEdit->setText("8888");
    ui->usernameEdit->setText("User" + QString::number(QRandomGenerator::global()->bounded(1000, 9999)));

    // 连接信号
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->joinChannelButton, &QPushButton::clicked, this, &MainWindow::onJoinChannelClicked);
    connect(ui->leaveChannelButton, &QPushButton::clicked, this, &MainWindow::onLeaveChannelClicked);

    // 网络客户端信号
    connect(m_networkClient, &NetworkClient::connected, this, &MainWindow::onServerConnected);
    connect(m_networkClient, &NetworkClient::disconnected, this, &MainWindow::onServerDisconnected);
    connect(m_networkClient, &NetworkClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_networkClient, &NetworkClient::channelListReceived, this, &MainWindow::onChannelListReceived);
    connect(m_networkClient, &NetworkClient::userListReceived, this, &MainWindow::onUserListReceived);
    connect(m_networkClient, &NetworkClient::userJoined, this, &MainWindow::onUserJoined);
    connect(m_networkClient, &NetworkClient::userLeft, this, &MainWindow::onUserLeft);
    connect(m_networkClient, &NetworkClient::joinedChannel, this, &MainWindow::onJoinedChannel);
    connect(m_networkClient, &NetworkClient::leftChannel, this, &MainWindow::onLeftChannel);
    connect(m_networkClient, &NetworkClient::errorOccurred, this, &MainWindow::onNetworkError);

    updateUIState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectClicked()
{
    QString serverIp = ui->serverIpEdit->text().trimmed();
    quint16 serverPort = ui->serverPortEdit->text().toUShort();
    QString username = ui->usernameEdit->text().trimmed();

    if (serverIp.isEmpty() || serverPort == 0 || username.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please fill in all fields.");
        return;
    }

    m_localVoicePort = getRandomPort();
    ui->statusLabel->setText("Connecting to server...");
    m_networkClient->connectToServer(serverIp, serverPort);
}

void MainWindow::onDisconnectClicked()
{
    m_audioEngine->stop();
    m_networkClient->disconnect();
    ui->statusLabel->setText("Status: Disconnected");
    updateUIState();
}

void MainWindow::onJoinChannelClicked()
{
    QListWidgetItem *item = ui->channelListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "No Channel Selected", "Please select a channel to join.");
        return;
    }

    QString channelName = item->text().split(" (").first(); // 移除用户计数
    m_networkClient->joinChannel(channelName);
}

void MainWindow::onLeaveChannelClicked()
{
    if (m_currentChannel.isEmpty()) return;
    
    m_audioEngine->stop();
    m_networkClient->leaveChannel();
}

void MainWindow::onServerConnected()
{
    ui->statusLabel->setText("Status: Connected - Logging in...");
    
    QString username = ui->usernameEdit->text().trimmed();
    QString localIp = "0.0.0.0"; // 服务器会使用客户端的实际 IP
    
    m_networkClient->login(username, localIp, m_localVoicePort);
}

void MainWindow::onServerDisconnected()
{
    ui->statusLabel->setText("Status: Disconnected from server");
    m_currentChannel.clear();
    ui->channelListWidget->clear();
    ui->userListWidget->clear();
    updateUIState();
}

void MainWindow::onLoginSuccess(quint16 voicePort)
{
    ui->statusLabel->setText("Status: Logged in - Select a channel");
    m_networkClient->requestChannelList();
    updateUIState();
}

void MainWindow::onChannelListReceived(const QJsonObject &data)
{
    ui->channelListWidget->clear();
    
    QJsonArray channels = data["channels"].toArray();
    for (const QJsonValue &val : channels) {
        QJsonObject ch = val.toObject();
        QString name = ch["name"].toString();
        int userCount = ch["user_count"].toInt();
        
        QString displayText = QString("%1 (%2 users)").arg(name).arg(userCount);
        ui->channelListWidget->addItem(displayText);
    }
}

void MainWindow::onUserListReceived(const QJsonObject &data)
{
    ui->userListWidget->clear();
    
    QJsonArray users = data["users"].toArray();
    for (const QJsonValue &val : users) {
        ui->userListWidget->addItem(val.toString());
    }
}

void MainWindow::onUserJoined(const QString &username)
{
    ui->userListWidget->addItem(username);
    ui->statusLabel->setText(QString("Status: %1 joined the channel").arg(username));
}

void MainWindow::onUserLeft(const QString &username)
{
    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        if (ui->userListWidget->item(i)->text() == username) {
            delete ui->userListWidget->takeItem(i);
            break;
        }
    }
    ui->statusLabel->setText(QString("Status: %1 left the channel").arg(username));
}

void MainWindow::onJoinedChannel(const QString &channel)
{
    m_currentChannel = channel;
    ui->statusLabel->setText(QString("Status: Joined channel '%1' - Voice connected").arg(channel));
    
    // 启动音频引擎
    QString serverIp = ui->serverIpEdit->text().trimmed();
    m_audioEngine->start(serverIp, m_localVoicePort, m_localVoicePort);
    
    updateUIState();
}

void MainWindow::onLeftChannel()
{
    ui->statusLabel->setText(QString("Status: Left channel '%1'").arg(m_currentChannel));
    m_currentChannel.clear();
    ui->userListWidget->clear();
    m_audioEngine->stop();
    
    // 刷新频道列表
    m_networkClient->requestChannelList();
    updateUIState();
}

void MainWindow::onNetworkError(const QString &error)
{
    ui->statusLabel->setText("Status: Error - " + error);
    QMessageBox::critical(this, "Network Error", error);
}

void MainWindow::updateUIState()
{
    bool connected = m_networkClient->isConnected();
    bool inChannel = !m_currentChannel.isEmpty();
    
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    ui->serverIpEdit->setEnabled(!connected);
    ui->serverPortEdit->setEnabled(!connected);
    ui->usernameEdit->setEnabled(!connected);
    
    ui->channelListWidget->setEnabled(connected && !inChannel);
    ui->joinChannelButton->setEnabled(connected && !inChannel);
    ui->leaveChannelButton->setEnabled(connected && inChannel);
}

quint16 MainWindow::getRandomPort()
{
    // 生成 49152-65535 之间的随机端口
    return 49152 + QRandomGenerator::global()->bounded(16384);
}
