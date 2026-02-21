#include "server.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("VoicePhone Server");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "VoicePhone Server\n"
        "\n用法: voicephone-server [选项]\n"
        "\n可用选项:\n"
        "  -c, --control-port <port>  指定客户端控制连接端口 (默认: 8888)\n"
        "  -p, --voice-port <port>    指定UDP语音端口 (默认: 8889)\n"
        "  -h, --help                 显示本帮助信息\n"
        "  --version                  显示版本信息\n"
        "\n如果未指定参数，服务器将使用默认端口启动。\n"
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption controlPortOption(QStringList() << "c" << "control-port",
        "Control port for client connections (default: 8888)", "port", "8888");
    parser.addOption(controlPortOption);

    QCommandLineOption voicePortOption(QStringList() << "p" << "voice-port",
        "Voice port for UDP audio (default: 8889)", "port", "8889");
    parser.addOption(voicePortOption);
    parser.process(app);

    quint16 controlPort = parser.value(controlPortOption).toUShort();
    quint16 voicePort = parser.value(voicePortOption).toUShort();

    VoiceServer server;
    if (!server.startServer(controlPort, voicePort)) {
        qCritical() << "Failed to start server!";
        return 1;
    }

    qInfo() << "VoicePhone Server running...";
    qInfo() << "Control Port:" << controlPort;
    qInfo() << "Voice Port:" << voicePort;
    qInfo() << "Press Ctrl+C to quit";

    return app.exec();
}
