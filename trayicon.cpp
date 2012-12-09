#include "trayicon.h"

#include "settingsdialog.h"
#include "serverloader.h"
#include "settings.h"
#include "server.h"

#include <QNetworkInterface>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QSignalMapper>
#include <QSettings>
#include <QPainter>
#include <QProcess>
#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QDir>
#include <QUrl>

TrayIcon::TrayIcon(ServerLoader *loader, QSettings *settings) :
	m_trayIcon(new QSystemTrayIcon),
	m_settings(settings),
	m_servers(loader->servers()),
	m_startMapper(new QSignalMapper),
	m_stopMapper(new QSignalMapper),
	m_restartMapper(new QSignalMapper),
	m_actionMapper(new QSignalMapper),
	m_trayMenu(new QMenu)
{
	connect(m_startMapper, SIGNAL(mapped(QObject*)), SLOT(startServer(QObject*)));
	connect(m_stopMapper, SIGNAL(mapped(QObject*)), SLOT(stopServer(QObject*)));
	connect(m_restartMapper, SIGNAL(mapped(QObject*)), SLOT(restartServer(QObject*)));
	connect(m_actionMapper, SIGNAL(mapped(QObject*)), SLOT(execAction(QObject*)));

	prepareProcesses();
	createMenus();
	updateStates();

	if(m_settings->value(Settings::startOnStartupKey).toBool())
		startAll();

	m_trayIcon->setContextMenu(m_trayMenu);
}

TrayIcon::~TrayIcon()
{
	stopAll();

	qDeleteAll(m_processes);
	m_processes.clear();

	qDeleteAll(m_servers);
	m_servers.clear();

	delete m_trayIcon;
	delete m_trayMenu;
}

void TrayIcon::show()
{
	m_trayIcon->show();
}

void TrayIcon::updateStates()
{
	bool serverRunning = true;
	bool serverStopped = true;

	foreach(Server *server, m_servers) {
		QProcess *process = m_processes.value(server);
		QMenu *menu = m_menus.value(server);

		bool running = (process->state() == QProcess::Running);

		foreach(QAction *action, menu->actions()) {
			QString role = action->property("role").toString();

			if((role == "restart") || (role == "stop") || (role == "action")) {
				action->setEnabled(running);
			} else if(role == "start") {
				action->setEnabled(!running);
			}
		}

		menu->setIcon(iconOverlay(server->icon, QPixmap(running ? ":/images/overlay_tick.png" : ":/images/overlay_cross.png")));
		serverStopped &= !running;
		serverRunning &= running;
	}

	QIcon serverIcon;
	if(serverRunning) {
		serverIcon = iconOverlay(QIcon(":/images/server.png"), QPixmap(":/images/overlay_tick.png"));
	} else if(serverStopped) {
		serverIcon = iconOverlay(QIcon(":/images/server.png"), QPixmap(":/images/overlay_cross.png"));
	} else {
		serverIcon = iconOverlay(QIcon(":/images/server.png"), QPixmap(":/images/overlay_warning.png"));
	}

	m_trayIcon->setIcon(serverIcon);
	m_allServersMenu->setIcon(serverIcon);
	m_restartAllAction->setEnabled(!serverStopped);
	m_startAllAction->setEnabled(!serverRunning);
	m_stopAllAction->setEnabled(!serverStopped);
}

void TrayIcon::startAll()
{
	QProgressDialog dlg;
	dlg.setWindowTitle(tr("Starting all servers"));
	dlg.setCancelButton(0);
	dlg.setMaximum(m_servers.count());
	dlg.show();

	for(int i = 0; i < m_servers.count(); ++i) {
		qApp->processEvents();
		dlg.setLabelText(tr("Starting %1").arg(m_servers[i]->name));
		startServer(m_servers[i]);
		dlg.setValue(i);
	}

	dlg.close();
}

void TrayIcon::startServer(QObject *object)
{
	if(Server *server = qobject_cast<Server*>(object)) {
		QProcess *process = m_processes.value(server);

		if(process->state() == QProcess::Running)
			return;

		if(m_settings->value(Settings::clearLogsOnStartKey).toBool())
			clearLogFiles(server);

		process->start(process->workingDirectory() + "/" + server->onStart.program, server->onStart.arguments);
	}
}

void TrayIcon::stopAll()
{
	QProgressDialog dlg;
	dlg.setWindowTitle(tr("Stopping all servers"));
	dlg.setCancelButton(0);
	dlg.setMaximum(m_servers.count());
	dlg.show();

	for(int i = 0; i < m_servers.count(); ++i) {
		qApp->processEvents();
		dlg.setLabelText(tr("Stopping %1").arg(m_servers[i]->name));
		stopServer(m_servers[i]);
		dlg.setValue(i);
	}

	dlg.close();
}

void TrayIcon::stopServer(QObject *object)
{
	if(Server *server = qobject_cast<Server*>(object)) {
		QProcess *process = m_processes.value(server);
		if(process->state() != QProcess::Running)
			return;

		if(server->onStop.program.isEmpty()) {
			process->kill();
		} else {
			QProcess killProcess;
			killProcess.setWorkingDirectory(server->workingDirectory);
			killProcess.start(killProcess.workingDirectory() + "/" + server->onStop.program, server->onStop.arguments);

			if(server->onStop.waitForFinished)
				killProcess.waitForFinished();
		}

		process->waitForFinished();
	}
}

void TrayIcon::restartAll()
{
	QProgressDialog dlg;
	dlg.setWindowTitle(tr("Restarting all servers"));
	dlg.setCancelButton(0);
	dlg.setMaximum(m_servers.count());
	dlg.show();

	for(int i = 0; i < m_servers.count(); ++i) {
		qApp->processEvents();
		dlg.setLabelText(tr("Restarting %1").arg(m_servers[i]->name));
		restartServer(m_servers[i]);
		dlg.setValue(i);
	}

	dlg.close();
}

void TrayIcon::restartServer(QObject *object)
{
	stopServer(object);
	startServer(object);
}

void TrayIcon::execAction(QObject *object)
{
	if(Action *action = qobject_cast<Action*>(object)) {
		switch(action->type()) {
			case ProcessAction::Type:
			{
				ProcessAction *processAction = static_cast<ProcessAction*>(action);

				if(processAction->process.detached) {
					QProcess::startDetached(qApp->applicationDirPath() + "/" + processAction->process.program, processAction->process.arguments);
				} else {
					QProcess process;
					process.setWorkingDirectory(action->server->workingDirectory);
					process.start(process.workingDirectory() + "/" + processAction->process.program, processAction->process.arguments);

					if(processAction->process.waitForFinished)
						process.waitForFinished();
				}
			}
			break;

			case FileAction::Type:
			{
				FileAction *fileAction = static_cast<FileAction*>(action);

				QUrl fileUrl(fileAction->filePath);
				if(fileUrl.scheme().isEmpty()) {
					QDesktopServices::openUrl(QUrl(qApp->applicationDirPath() + "/" + fileAction->filePath));
				} else {
					QDesktopServices::openUrl(fileUrl);
				}
			}
			break;
		}
	}
}

void TrayIcon::clearLogFiles(Server *server)
{
	foreach(const QString &logFile, server->logFiles) {
		QFile file(logFile);
		if(!file.open(QFile::ReadWrite | QFile::Truncate)) {
			qDebug() << "Failed to open" << logFile;
			continue;
		}

		file.close();
	}
}

void TrayIcon::openFile()
{
	if(QAction *action = qobject_cast<QAction*>(sender()))
		QDesktopServices::openUrl(QUrl(action->property("filePath").toString()));
}

void TrayIcon::openServerDir()
{
	QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath()));
}

void TrayIcon::showSettings()
{
	SettingsDialog dlg(m_settings);
	dlg.exec();
}

void TrayIcon::prepareProcesses()
{
	foreach(Server *server, m_servers) {
		QProcess *process = new QProcess();
		process->setWorkingDirectory(qApp->applicationDirPath() + "/" + server->workingDirectory);

		connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), SLOT(updateStates()));

		m_processes.insert(server, process);
	}
}

void TrayIcon::createMenus()
{
	//get local IPs
	QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
	foreach(const QNetworkInterface &interface, interfaces) {
		if(interface.flags().testFlag(QNetworkInterface::IsUp) &&
				interface.flags().testFlag(QNetworkInterface::IsRunning) &&
				!interface.flags().testFlag(QNetworkInterface::IsLoopBack))
		{
			foreach(const QNetworkAddressEntry &entry, interface.addressEntries()) {
				QHostAddress address = entry.ip();
				if(address.protocol() == QAbstractSocket::IPv4Protocol) {
					QAction *ipAction = m_trayMenu->addAction("IP: " + address.toString());
					ipAction->setEnabled(false);
				}
			}
		}
	}
	m_trayMenu->addSeparator();

	//servers
	m_allServersMenu = m_trayMenu->addMenu(QIcon(":/images/server.png"), tr("All servers"));
	m_restartAllAction = m_allServersMenu->addAction(QIcon(":/images/tick_cross.png"), tr("Restart"), this, SLOT(restartAll()));
	m_allServersMenu->addSeparator();

	m_startAllAction = m_allServersMenu->addAction(QIcon(":/images/tick.png"), tr("Start"), this, SLOT(startAll()));
	m_stopAllAction = m_allServersMenu->addAction(QIcon(":/images/cross.png"), tr("Stop"), this, SLOT(stopAll()));

	foreach(Server *server, m_servers) {
		QMenu *serverMenu = m_trayMenu->addMenu(server->icon, server->name);

		QAction *restartAction = serverMenu->addAction(QIcon(":/images/tick_cross.png"), tr("Restart"), m_restartMapper, SLOT(map()));
		restartAction->setProperty("role", "restart");
		m_restartMapper->setMapping(restartAction, server);
		serverMenu->addSeparator();

		QAction *startAction = serverMenu->addAction(QIcon(":/images/tick.png"), tr("Start"), m_startMapper, SLOT(map()));
		startAction->setProperty("role", "start");
		m_startMapper->setMapping(startAction, server);

		QAction *stopAction = serverMenu->addAction(QIcon(":/images/cross.png"), tr("Stop"), m_stopMapper, SLOT(map()));
		stopAction->setProperty("role", "stop");
		m_stopMapper->setMapping(stopAction, server);

		bool hasSeparator = false;
		foreach(Action *serverAction, server->actions) {
			if(serverAction->category != Action::Server)
				continue;

			if(!hasSeparator) {
				serverMenu->addSeparator();
				hasSeparator = true;
			}

			QAction *action = serverMenu->addAction(serverAction->icon, serverAction->text, m_actionMapper, SLOT(map()));
			action->setProperty("role", "action");
			m_actionMapper->setMapping(action, serverAction);
		}

		m_menus.insert(server, serverMenu);
	}
	m_trayMenu->addSeparator();

	//tools
	bool hasTools = false;
	foreach(Server *server, m_servers) {
		foreach(Action *serverAction, server->actions) {
			if(serverAction->category != Action::Tools)
				continue;

			if(!hasTools)
				hasTools = true;

			QAction *action = m_trayMenu->addAction(serverAction->icon, serverAction->text, m_actionMapper, SLOT(map()));
			m_actionMapper->setMapping(action, serverAction);
		}
	}

	if(hasTools)
		m_trayMenu->addSeparator();

	//directories
	m_serverDirectoryAction = m_trayMenu->addAction(QIcon(":/images/folder_server.png"), tr("Server directory"), this, SLOT(openServerDir()));

	foreach(Server *server, m_servers) {
		foreach(Action *serverAction, server->actions) {
			if(serverAction->category != Action::Directories)
				continue;

			QAction *action = m_trayMenu->addAction(serverAction->icon, serverAction->text, m_actionMapper, SLOT(map()));
			m_actionMapper->setMapping(action, serverAction);
		}
	}
	m_trayMenu->addSeparator();

	//files
	m_logFilesMenu = m_trayMenu->addMenu(QIcon(":/images/log_files.png"), tr("Log files"));
	foreach(Server *server, m_servers) {
		foreach(const QString &filePath, server->logFiles) {
			QDir appDir(qApp->applicationDirPath());

			QAction *configAction = m_logFilesMenu->addAction(QIcon(":/images/page.png"), appDir.relativeFilePath(filePath), this, SLOT(openFile()));
			configAction->setProperty("filePath", filePath);
		}
	}

	if(m_logFilesMenu->actions().count() == 0) {
		m_logFilesMenu->menuAction()->setVisible(false);
	}

	m_configFilesMenu = m_trayMenu->addMenu(QIcon(":/images/config_files.png"), tr("Config files"));
	foreach(Server *server, m_servers) {
		foreach(const QString &filePath, server->configFiles) {
			QDir appDir(qApp->applicationDirPath());

			QAction *configAction = m_configFilesMenu->addAction(QIcon(":/images/page.png"), appDir.relativeFilePath(filePath), this, SLOT(openFile()));
			configAction->setProperty("filePath", filePath);
		}
	}

	if(m_configFilesMenu->actions().count() == 0) {
		m_configFilesMenu->menuAction()->setVisible(false);
	}

	if(m_configFilesMenu->actions().count() || m_logFilesMenu->actions().count()) {
		m_trayMenu->addSeparator();
	}

	m_settingsAction = m_trayMenu->addAction(QIcon(":/images/wrench.png"), tr("Settings"), this, SLOT(showSettings()));
	m_trayMenu->addSeparator();

	m_exitAction = m_trayMenu->addAction(QIcon(":/images/exit.png"), tr("Exit"), qApp, SLOT(quit()));
}

QIcon TrayIcon::iconOverlay(const QIcon &icon, const QPixmap &overlay)
{
	QPixmap pixmap = icon.pixmap(16, 16);

	QPainter p(&pixmap);
	p.drawPixmap(pixmap.width() - overlay.width(), pixmap.height() - overlay.height(), overlay);

	return QIcon(pixmap);
}

void TrayIcon::retranslateUi()
{
	m_allServersMenu->setTitle(tr("All servers"));
	m_restartAllAction->setText(tr("Restart"));
	m_startAllAction->setText(tr("Start"));
	m_stopAllAction->setText(tr("Stop"));
	m_serverDirectoryAction->setText(tr("Server directory"));
	m_settingsAction->setText(tr("Settings"));
	m_configFilesMenu->setTitle(tr("Config files"));
	m_exitAction->setText(tr("Exit"));
}
