#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonObject>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class AudioEngine;
class NetworkClient;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr, NetworkClient *networkClient = nullptr,
             QString *serverIP = nullptr);
  ~MainWindow();

private slots:
  void onJoinChannelClicked();
  void onLeaveChannelClicked();

  void onChannelListReceived(const QJsonObject &data);
  void onUserListReceived(const QJsonObject &data);
  void onUserJoined(const QString &username);
  void onUserLeft(const QString &username);
  void onJoinedChannel(const QString &channel);
  void onLeftChannel();
  void onNetworkError(const QString &error);

private:
  void updateUIState();

  Ui::MainWindow *ui;
  AudioEngine *m_audioEngine;
  NetworkClient *m_networkClient;
  QString m_currentChannel;
  quint16 m_localVoicePort;
  quint64 m_audioCounter; // 用于音频加密的计数器
  QString *m_serverIP;
};

#endif // MAINWINDOW_H
