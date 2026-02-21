#ifndef LOGIN_DLG_H
#define LOGIN_DLG_H

#include <QDialog>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui {
class LoginDlg;
}
QT_END_NAMESPACE

class AudioEngine;
class NetworkClient;

class LoginDlg : public QDialog {
  Q_OBJECT

public:
  LoginDlg(QWidget *parent = nullptr, NetworkClient *networkClient = nullptr);
  ~LoginDlg();

private slots:
  void onConnectClicked();
  void onDisconnectClicked();

  void onServerConnected();
  void onServerDisconnected();
  void onRegistrationSuccess();
  void onLoginSuccess(quint16 voicePort);
  void onNetworkError(const QString &error);

private:
  void updateUIState();
  quint16 getRandomPort();
  void saveConnectionSettings();
  void loadConnectionSettings();

  Ui::LoginDlg *ui;
  AudioEngine *m_audioEngine;
  NetworkClient *m_networkClient;
  QString m_currentChannel;
  quint16 m_localVoicePort;
  quint64 m_audioCounter; // 用于音频加密的计数器
};

#endif // LOGIN_DLG_H
