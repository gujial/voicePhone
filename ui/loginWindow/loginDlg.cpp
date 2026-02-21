#include "loginDlg.h"
#include "../../../client/networkclient.h"
#include "../../src/audioengine.h"
#include "ui_loginDlg.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimer>

LoginDlg::LoginDlg(QWidget *parent, NetworkClient *networkClient)
    : QDialog(parent), ui(new Ui::LoginDlg),
      m_audioEngine(new AudioEngine(this)), m_networkClient(networkClient),
      m_localVoicePort(0), m_audioCounter(0) {
  ui->setupUi(this);

  // 加载保存的连接设置
  loadConnectionSettings();

  // 连接信号
  connect(ui->connectButton, &QPushButton::clicked, this,
          &LoginDlg::onConnectClicked);
  connect(ui->disconnectButton, &QPushButton::clicked, this,
          &LoginDlg::onDisconnectClicked);

  // 网络客户端信号
  connect(m_networkClient, &NetworkClient::connected, this,
          &LoginDlg::onServerConnected);
  connect(m_networkClient, &NetworkClient::disconnected, this,
          &LoginDlg::onServerDisconnected);
  connect(m_networkClient, &NetworkClient::registrationSuccess, this,
          &LoginDlg::onRegistrationSuccess);
  connect(m_networkClient, &NetworkClient::loginSuccess, this,
          &LoginDlg::onLoginSuccess);
  connect(m_networkClient, &NetworkClient::errorOccurred, this,
          &LoginDlg::onNetworkError);

  updateUIState();
}

LoginDlg::~LoginDlg() { delete ui; }

void LoginDlg::onConnectClicked() {
  QString serverIp = ui->serverIpEdit->text().trimmed();
  quint16 serverPort = ui->serverPortEdit->text().toUShort();
  QString username = ui->usernameEdit->text().trimmed();
  QString password = ui->passwordEdit->text();

  if (serverIp.isEmpty() || serverPort == 0 || username.isEmpty() ||
      password.isEmpty()) {
    QMessageBox::warning(this, "Invalid Input", "Please fill in all fields.");
    return;
  }

  // 如果已连接但未认证，先断开连接
  if (m_networkClient->isConnected() && !m_networkClient->isAuthenticated()) {
    m_networkClient->disconnect();
    // 使用短延迟后重新连接
    QTimer::singleShot(100, this, [this, serverIp, serverPort]() {
      m_localVoicePort = getRandomPort();
      ui->statusLabel->setText("Connecting to server...");
      m_networkClient->connectToServer(serverIp, serverPort);
    });
    return;
  }

  m_localVoicePort = getRandomPort();
  ui->statusLabel->setText("Connecting to server...");
  m_networkClient->connectToServer(serverIp, serverPort);
}

void LoginDlg::onDisconnectClicked() {
  m_audioEngine->stop();
  m_networkClient->disconnect();
  ui->statusLabel->setText("Status: Disconnected");
  updateUIState();
}

void LoginDlg::onServerConnected() {
  ui->statusLabel->setText("Status: Connected - Authenticating...");

  QString username = ui->usernameEdit->text().trimmed();
  QString password = ui->passwordEdit->text();
  QString localIp = "0.0.0.0"; // 服务器会使用客户端的实际 IP

  // 检查是否是注册模式
  if (ui->registerCheckbox->isChecked()) {
    m_networkClient->registerUser(username, password);
    // 注册成功后会自动触发登录
  } else {
    m_networkClient->login(username, password, localIp, m_localVoicePort);
  }
}

void LoginDlg::onRegistrationSuccess() {
  ui->statusLabel->setText("Status: Registration successful - Logging in...");
  QMessageBox::information(this, "Success",
                           "Registration successful! Now logging in...");

  // 注册成功后自动登录
  QString username = ui->usernameEdit->text().trimmed();
  QString password = ui->passwordEdit->text();
  QString localIp = "0.0.0.0";

  m_networkClient->login(username, password, localIp, m_localVoicePort);
}

void LoginDlg::onServerDisconnected() {
  ui->statusLabel->setText("Status: Disconnected from server");
  m_currentChannel.clear();
  updateUIState();
}

void LoginDlg::onLoginSuccess(quint16 voicePort) {
  ui->statusLabel->setText("Status: Logged in - Select a channel");

  // 保存连接设置
  saveConnectionSettings();

  // 设置音频引擎的加密密钥
  QByteArray sessionKey = m_networkClient->getSessionKey();
  if (!sessionKey.isEmpty()) {
    m_audioEngine->setEncryptionKey(sessionKey);
    m_audioEngine->setAudioCounter(&m_audioCounter);
    qInfo() << "Encryption enabled for audio";
  }

  m_networkClient->requestChannelList();
  updateUIState();
  this->accept();
}

void LoginDlg::onNetworkError(const QString &error) {
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

void LoginDlg::updateUIState() {
  bool connected = m_networkClient->isConnected();
  bool authenticated = m_networkClient->isAuthenticated();
  bool inChannel = !m_currentChannel.isEmpty();

  ui->connectButton->setEnabled(!connected);
  ui->disconnectButton->setEnabled(connected);
  ui->serverIpEdit->setEnabled(!connected);
  ui->serverPortEdit->setEnabled(!connected);
  ui->usernameEdit->setEnabled(!connected);
  ui->passwordEdit->setEnabled(!connected);
  ui->registerCheckbox->setEnabled(!connected);
}

quint16 LoginDlg::getRandomPort() {
  // 生成 49152-65535 之间的随机端口
  return 49152 + QRandomGenerator::global()->bounded(16384);
}

void LoginDlg::saveConnectionSettings() {
  QSettings settings("VoicePhone", "VoicePhone");

  settings.beginGroup("Connection");
  settings.setValue("serverIp", ui->serverIpEdit->text());
  settings.setValue("serverPort", ui->serverPortEdit->text());
  settings.setValue("username", ui->usernameEdit->text());
  settings.endGroup();

  qInfo() << "Connection settings saved";
}

void LoginDlg::loadConnectionSettings() {
  QSettings settings("VoicePhone", "VoicePhone");

  settings.beginGroup("Connection");
  QString serverIp = settings.value("serverIp", "").toString();
  QString serverPort = settings.value("serverPort", "8888").toString();
  QString username = settings.value("username", "").toString();
  settings.endGroup();

  // 填充UI输入框
  ui->serverIpEdit->setText(serverIp);
  ui->serverPortEdit->setText(serverPort);
  ui->usernameEdit->setText(username);

  if (!serverIp.isEmpty() && !username.isEmpty()) {
    qInfo() << "Connection settings loaded for user:" << username;
  }
}
