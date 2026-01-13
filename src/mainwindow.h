#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AudioEngine;
class NetworkClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onJoinChannelClicked();
    void onLeaveChannelClicked();
    
    void onServerConnected();
    void onServerDisconnected();
    void onRegistrationSuccess();
    void onLoginSuccess(quint16 voicePort);
    void onChannelListReceived(const QJsonObject &data);
    void onUserListReceived(const QJsonObject &data);
    void onUserJoined(const QString &username);
    void onUserLeft(const QString &username);
    void onJoinedChannel(const QString &channel);
    void onLeftChannel();
    void onNetworkError(const QString &error);

private:
    void updateUIState();
    quint16 getRandomPort();
    
    Ui::MainWindow *ui;
    AudioEngine *m_audioEngine;
    NetworkClient *m_networkClient;
    QString m_currentChannel;
    quint16 m_localVoicePort;
    quint64 m_audioCounter; // 用于音频加密的计数器
};

#endif // MAINWINDOW_H
