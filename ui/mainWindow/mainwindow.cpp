#include "mainwindow.h"
#include "../../../client/networkclient.h"
#include "../../src/audioengine.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent, NetworkClient *networkClient)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      m_audioEngine(new AudioEngine(this)), m_networkClient(networkClient),
      m_localVoicePort(networkClient->getUdpPort()), m_audioCounter(0),
      m_serverIP(networkClient->getServerIP()) {
  ui->setupUi(this);

  // 连接信号
  connect(ui->joinChannelButton, &QPushButton::clicked, this,
          &MainWindow::onJoinChannelClicked);
  connect(ui->leaveChannelButton, &QPushButton::clicked, this,
          &MainWindow::onLeaveChannelClicked);

  // 网络客户端信号
  connect(m_networkClient, &NetworkClient::channelListReceived, this,
          &MainWindow::onChannelListReceived);
  connect(m_networkClient, &NetworkClient::userListReceived, this,
          &MainWindow::onUserListReceived);
  connect(m_networkClient, &NetworkClient::userJoined, this,
          &MainWindow::onUserJoined);
  connect(m_networkClient, &NetworkClient::userLeft, this,
          &MainWindow::onUserLeft);
  connect(m_networkClient, &NetworkClient::joinedChannel, this,
          &MainWindow::onJoinedChannel);
  connect(m_networkClient, &NetworkClient::leftChannel, this,
          &MainWindow::onLeftChannel);
  connect(m_networkClient, &NetworkClient::errorOccurred, this,
          &MainWindow::onNetworkError);

  updateUIState();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::onJoinChannelClicked() {
  QListWidgetItem *item = ui->channelListWidget->currentItem();
  if (!item) {
    QMessageBox::warning(this, "No Channel Selected",
                         "Please select a channel to join.");
    return;
  }

  QString channelName = item->text().split(" (").first(); // 移除用户计数
  m_networkClient->joinChannel(channelName);
}

void MainWindow::onLeaveChannelClicked() {
  if (m_currentChannel.isEmpty())
    return;

  m_audioEngine->stop();
  m_networkClient->leaveChannel();
}

void MainWindow::onChannelListReceived(const QJsonObject &data) {
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

void MainWindow::onUserListReceived(const QJsonObject &data) {
  ui->userListWidget->clear();

  QJsonArray users = data["users"].toArray();
  for (const QJsonValue &val : users) {
    ui->userListWidget->addItem(val.toString());
  }
}

void MainWindow::onUserJoined(const QString &username) {
  ui->userListWidget->addItem(username);
  ui->statusLabel->setText(
      QString("Status: %1 joined the channel").arg(username));
}

void MainWindow::onUserLeft(const QString &username) {
  for (int i = 0; i < ui->userListWidget->count(); ++i) {
    if (ui->userListWidget->item(i)->text() == username) {
      delete ui->userListWidget->takeItem(i);
      break;
    }
  }
  ui->statusLabel->setText(
      QString("Status: %1 left the channel").arg(username));
}

void MainWindow::onJoinedChannel(const QString &channel) {
  m_currentChannel = channel;
  m_audioCounter = 0; // 重置音频计数器
  ui->statusLabel->setText(
      QString("Status: Joined channel '%1' - Voice connected").arg(channel));

  // 设置频道加密密钥到音频引擎
  QByteArray channelKey = m_networkClient->getChannelKey();
  if (!channelKey.isEmpty()) {
    m_audioEngine->setEncryptionKey(channelKey);
    m_audioEngine->setAudioCounter(&m_audioCounter);
    qInfo() << "Audio encryption enabled for channel";
  }

  // 启动音频引擎
  QString serverIp = this->m_serverIP.trimmed();
  m_audioEngine->start(serverIp, m_localVoicePort, m_localVoicePort);

  updateUIState();
}

void MainWindow::onLeftChannel() {
  ui->statusLabel->setText(
      QString("Status: Left channel '%1'").arg(m_currentChannel));
  m_currentChannel.clear();
  ui->userListWidget->clear();
  m_audioEngine->stop();

  // 刷新频道列表
  m_networkClient->requestChannelList();
  updateUIState();
}

void MainWindow::onNetworkError(const QString &error) {
  ui->statusLabel->setText("Status: Error - " + error);

  // 如果是认证相关错误（注册失败或登录失败），断开连接以允许重试
  if (error.contains("Registration failed") ||
      error.contains("Authentication failed")) {
    QMessageBox::critical(this, "Error", error);
    m_networkClient->disconnect();
  } else {
    QMessageBox::critical(this, "Network Error", error);
  }
}

void MainWindow::updateUIState() {
  bool connected = m_networkClient->isConnected();
  bool authenticated = m_networkClient->isAuthenticated();
  bool inChannel = !m_currentChannel.isEmpty();

  ui->channelListWidget->setEnabled(authenticated && !inChannel);
  ui->joinChannelButton->setEnabled(authenticated && !inChannel);
  ui->leaveChannelButton->setEnabled(authenticated && inChannel);
}
