#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QProcess>
#include <QPainter>
#include <QPixmap>
#include <QAction>
#include <QStyle>
#include <QDebug>
#include <QInputDialog>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QDir>
#include <QMessageBox>

class ZeroTierTray : public QObject {
    Q_OBJECT

public:
    ZeroTierTray() {
        trayIcon = new QSystemTrayIcon(this);
        menu = new QMenu();

        // Connect the menu to dynamically build itself when clicked
        connect(menu, &QMenu::aboutToShow, this, &ZeroTierTray::populateMenu);
        trayIcon->setContextMenu(menu);

        // Timer to check status periodically for the tray icon color
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ZeroTierTray::checkStatus);
        timer->start(2000); // Check every 2 seconds

        // Initial checks
        checkStatus();
        populateMenu();
        trayIcon->show();
    }

private slots:
    void populateMenu() {
        menu->clear();

        bool active = isServiceActive();

        // 1. Toggle Service Action
        QAction* toggleAction = new QAction(active ? "Stop ZeroTier" : "Start ZeroTier", this);
        connect(toggleAction, &QAction::triggered, this, &ZeroTierTray::toggleService);
        menu->addAction(toggleAction);

        // 2. Join Network Action
        QAction* joinAction = new QAction("Join Network...", this);
        connect(joinAction, &QAction::triggered, this, &ZeroTierTray::joinNetwork);
        menu->addAction(joinAction);

        // 3. Dynamic Networks Submenu (if service is active)
        if (active) {
            QMenu* networksMenu = new QMenu("My Networks", menu);
            
            // Scan /var/lib/zerotier-one/networks.d/ for configured networks
            QDir dir("/var/lib/zerotier-one/networks.d");
            QStringList filters;
            filters << "*.conf";
            QFileInfoList list = dir.entryInfoList(filters, QDir::Files);
            
            if (list.isEmpty()) {
                QAction* emptyAction = new QAction("No networks joined", networksMenu);
                emptyAction->setEnabled(false);
                networksMenu->addAction(emptyAction);
            } else {
                for (const QFileInfo& fileInfo : list) {
                    QString filename = fileInfo.fileName();
                    // Skip any helper configs like .local.conf
                    if (filename.endsWith(".local.conf")) {
                        continue;
                    }
                    
                    QString nwid = fileInfo.baseName(); // The 16-hex ID
                    
                    // Create submenu for this network ID
                    QMenu* netSubMenu = new QMenu(nwid, networksMenu);
                    
                    QAction* leaveAction = new QAction("Leave Network", netSubMenu);
                    connect(leaveAction, &QAction::triggered, this, [this, nwid]() {
                        leaveNetwork(nwid);
                    });
                    netSubMenu->addAction(leaveAction);
                    
                    networksMenu->addMenu(netSubMenu);
                }
            }
            menu->addMenu(networksMenu);
        }

        menu->addSeparator();

        // 4. Kill Daemon & Exit Action
        QAction* killExitAction = new QAction("Kill Daemon & Exit", this);
        connect(killExitAction, &QAction::triggered, this, &ZeroTierTray::killDaemonAndExit);
        menu->addAction(killExitAction);

        // 5. Exit Tray Only Action
        QAction* quitAction = new QAction("Exit Tray Only", this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
        menu->addAction(quitAction);
    }

    void checkStatus() {
        bool serviceActive = false;
        bool networkConnected = false;

        // Check if the systemd service is active
        QProcess systemctl;
        systemctl.start("systemctl", QStringList() << "is-active" << "zerotier-one");
        systemctl.waitForFinished();
        if (systemctl.exitCode() == 0) {
            QString output = QString::fromUtf8(systemctl.readAllStandardOutput()).trimmed();
            if (output == "active") {
                serviceActive = true;
            }
        }

        if (serviceActive) {
            // Check if there are any active ZeroTier interfaces with an assigned IP
            const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
            for (const QNetworkInterface &interface : interfaces) {
                if (interface.name().startsWith("zt") && 
                    (interface.flags() & QNetworkInterface::IsUp) && 
                    !interface.addressEntries().isEmpty()) {
                    networkConnected = true;
                    break;
                }
            }
        }

        updateIcon(serviceActive, networkConnected);
    }

    void toggleService() {
        QProcess proc;
        if (isServiceActive()) {
            proc.start("systemctl", QStringList() << "stop" << "zerotier-one");
        } else {
            proc.start("systemctl", QStringList() << "start" << "zerotier-one");
        }
        proc.waitForFinished();
        checkStatus();
    }

    void joinNetwork() {
        bool ok;
        QString nwid = QInputDialog::getText(nullptr, "Join ZeroTier Network",
                                             "Enter 16-character Network ID:",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !nwid.trimmed().isEmpty()) {
            QProcess proc;
            proc.start("pkexec", QStringList() << "zerotier-cli" << "join" << nwid.trimmed());
            proc.waitForFinished();
            checkStatus();
        }
    }

    void leaveNetwork(const QString& nwid) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Leave Network", 
                                      "Are you sure you want to leave network " + nwid + "?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QProcess proc;
            proc.start("pkexec", QStringList() << "zerotier-cli" << "leave" << nwid);
            proc.waitForFinished();
            checkStatus();
        }
    }

    void killDaemonAndExit() {
        QProcess proc;
        proc.start("systemctl", QStringList() << "stop" << "zerotier-one");
        proc.waitForFinished();
        qApp->quit();
    }

private:
    QSystemTrayIcon* trayIcon;
    QMenu* menu;

    bool isServiceActive() {
        QProcess systemctl;
        systemctl.start("systemctl", QStringList() << "is-active" << "zerotier-one");
        systemctl.waitForFinished();
        return systemctl.exitCode() == 0 && 
               QString::fromUtf8(systemctl.readAllStandardOutput()).trimmed() == "active";
    }

    void updateIcon(bool active, bool connected) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        if (!active) {
            painter.setBrush(QBrush(QColor("#FF3333"))); // Soft red
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Stopped");
        } else if (connected) {
            painter.setBrush(QBrush(QColor("#0080FF"))); // Vibrant blue
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Connected (WaldorfNet)");
        } else {
            painter.setBrush(QBrush(QColor("#FFA500"))); // Orange
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Disconnected");
        }

        painter.end();
        trayIcon->setIcon(QIcon(pixmap));
    }
};

int main(int argc, char *argv[]) {
    // Set style before QApplication initialization to support native styling
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    ZeroTierTray tray;

    return app.exec();
}

#include "main.moc"
