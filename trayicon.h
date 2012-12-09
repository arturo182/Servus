#ifndef TRAYICON_H
#define TRAYICON_H

#include <QPointer>
#include <QObject>
#include <QMap>

class QSystemTrayIcon;
class QSignalMapper;
class QSettings;
class QProcess;
class QAction;
class QPixmap;
class QIcon;
class QMenu;

class ServerLoader;
class Server;

class TrayIcon : public QObject
{
	Q_OBJECT

	public:
		TrayIcon(ServerLoader *loader, QSettings *settings);
		~TrayIcon();

		void show();

	public slots:
		void updateStates();

		void startAll();
		void startServer(QObject *object);
		void stopAll();
		void stopServer(QObject *object);
		void restartAll();
		void restartServer(QObject *object);

		void execAction(QObject *object);
		void clearLogFiles(Server *server);
		void openFile();
		void openServerDir();
		void showSettings();

	private:
		void prepareProcesses();
		void createMenus();
		QIcon iconOverlay(const QIcon &icon, const QPixmap &overlay);
		void retranslateUi();

	private:
		QSystemTrayIcon *m_trayIcon;
		QSettings *m_settings;

		QList<Server*> m_servers;
		QMap<Server*, QProcess*> m_processes;
		QMap<Server*, QMenu*> m_menus;

		QSignalMapper *m_startMapper;
		QSignalMapper *m_stopMapper;
		QSignalMapper *m_restartMapper;
		QSignalMapper *m_actionMapper;

		QMenu *m_trayMenu;
		QMenu *m_allServersMenu;
		QAction *m_restartAllAction;
		QAction *m_startAllAction;
		QAction *m_stopAllAction;
		QAction *m_serverDirectoryAction;
		QAction *m_settingsAction;
		QMenu *m_logFilesMenu;
		QMenu *m_configFilesMenu;
		QAction *m_exitAction;
};

#endif // TRAYICON_H
