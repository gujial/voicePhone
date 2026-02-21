#include "../client/networkclient.h"
#include "loginWindow/loginDlg.h"
#include "mainWindow/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  NetworkClient *client = new NetworkClient();
  QApplication a(argc, argv);
  LoginDlg loginDlg(nullptr, client);
  if (loginDlg.exec() == QDialog::Accepted) {
    MainWindow w(nullptr, client);
    w.show();
    return a.exec();
  }
  return 0;
}
