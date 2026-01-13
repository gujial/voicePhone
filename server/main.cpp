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
    parser.setApplicationDescription("VoicePhone Server - Discord-like voice chat server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption controlPortOption(QStringList() << "c" << "control-port",
        "Control port for client connections (default: 8888)", "port", "8888");
    parser.addOption(controlPortOption);

    QCommandLineOption voicePortOption(QStringList() << "v" << "voice-port",
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
