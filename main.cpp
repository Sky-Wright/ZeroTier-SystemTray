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

#include <QNetworkInterface>

class ZeroTierTray : public QObject {
    Q_OBJECT

public:
    ZeroTierTray() {
        trayIcon = new QSystemTrayIcon(this);
        menu = new QMenu();

        // Create Actions
        toggleAction = new QAction(this);
        connect(toggleAction, &QAction::triggered, this, &ZeroTierTray::toggleService);
        menu->addAction(toggleAction);

        QAction* killExitAction = new QAction("Kill Daemon & Exit", this);
        connect(killExitAction, &QAction::triggered, this, &ZeroTierTray::killDaemonAndExit);
        menu->addAction(killExitAction);

        menu->addSeparator();

        QAction* quitAction = new QAction("Exit Tray Only", this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
        menu->addAction(quitAction);

        trayIcon->setContextMenu(menu);

        // Timer to check status periodically
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ZeroTierTray::checkStatus);
        timer->start(2000); // Check every 2 seconds

        // Initial check
        checkStatus();
        trayIcon->show();
    }

private slots:
    void checkStatus() {
        bool serviceActive = false;
        bool networkConnected = false;

        // 1. Check if the systemd service is active
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
            // 2. Check if there are any active ZeroTier interfaces with an assigned IP
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

        // Update UI based on status
        updateIcon(serviceActive, networkConnected);
        updateMenu(serviceActive);
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

    void killDaemonAndExit() {
        QProcess proc;
        proc.start("systemctl", QStringList() << "stop" << "zerotier-one");
        proc.waitForFinished();
        qApp->quit();
    }

private:
    QSystemTrayIcon* trayIcon;
    QMenu* menu;
    QAction* toggleAction;

    bool isServiceActive() {
        QProcess systemctl;
        systemctl.start("systemctl", QStringList() << "is-active" << "zerotier-one");
        systemctl.waitForFinished();
        return systemctl.exitCode() == 0 && 
               QString::fromUtf8(systemctl.readAllStandardOutput()).trimmed() == "active";
    }

    void updateIcon(bool active, bool connected) {
        // Draw a clean, modern status circle
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        if (!active) {
            // Inactive / Stopped: Red circle (or we can hide it if desired)
            painter.setBrush(QBrush(QColor("#FF3333"))); // Soft red
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Stopped");
        } else if (connected) {
            // Running & Connected: Blue circle
            painter.setBrush(QBrush(QColor("#0080FF"))); // Vibrant blue
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Connected (WaldorfNet)");
        } else {
            // Running but Disconnected: Yellow/Orange circle (or grey)
            painter.setBrush(QBrush(QColor("#FFA500"))); // Orange
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(4, 4, 24, 24);
            trayIcon->setToolTip("ZeroTier: Disconnected / Requesting Config");
        }

        painter.end();
        trayIcon->setIcon(QIcon(pixmap));
    }

    void updateMenu(bool active) {
        if (active) {
            toggleAction->setText("Stop ZeroTier");
        } else {
            toggleAction->setText("Start ZeroTier");
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    ZeroTierTray tray;

    return app.exec();
}

#include "main.moc"
