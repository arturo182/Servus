#include "serverloader.h"

#include "server.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>

ServerLoader::ServerLoader(const QString &dataDir) :
	m_dataDir(dataDir)
{
}

void ServerLoader::load()
{
	qDeleteAll(m_servers);
	m_servers.clear();

	QDir dataDir(m_dataDir);
	foreach(const QString &fileName, dataDir.entryList(QStringList() << "*.json")) {
		QFile file(dataDir.absoluteFilePath(fileName));
		if(!file.open(QFile::ReadOnly)) {
			qDebug() << "Could not open" << fileName;
			continue;
		}

		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
		file.close();

		if(error.error != QJsonParseError::NoError) {
			qDebug() << "Failed to parse JSON in" << fileName << error.errorString() << "at" << error.offset;
			continue;
		}

		const QJsonObject root = doc.object();
		Server *server = new Server();
		server->name = root.value("name").toString(fileName);
		server->workingDirectory = root.value("workingDirectory").toString();

		//support local and resource images
		const QString iconPath = root.value("icon").toString(":/images/server.png");
		server->icon = QIcon(iconPath.startsWith(":/") ? iconPath : dataDir.absoluteFilePath(iconPath));

		const QJsonArray configFiles = root.value("configFiles").toArray();
		for(int i = 0; i < configFiles.size(); ++i) {
			QFileInfo fileInfo(qApp->applicationDirPath() + "/" + configFiles[i].toString());

			if(fileInfo.exists()) {
				server->configFiles << fileInfo.absoluteFilePath();
			}
		}

		const QJsonArray logFiles = root.value("logFiles").toArray();
		for(int i = 0; i < logFiles.size(); ++i) {
			QFileInfo fileInfo(qApp->applicationDirPath() + "/" + logFiles[i].toString());

			if(fileInfo.exists()) {
				server->logFiles << fileInfo.absoluteFilePath();
			}
		}

		const QJsonObject onStart = root.value("onStart").toObject();
		server->onStart.program = onStart.value("program").toString();
		server->onStart.arguments = onStart.value("arguments").toVariant().toStringList();

		const QJsonObject onStop = root.value("onStop").toObject();
		server->onStop.program = onStop.value("program").toString();
		server->onStop.arguments = onStop.value("arguments").toVariant().toStringList();
		server->onStop.waitForFinished = onStop.value("waitForFinished").toBool();

		const QJsonArray actions = root.value("actions").toArray();
		for(int i = 0; i < actions.size(); ++i) {
			const QJsonObject actionRoot = actions[i].toObject();

			const int type = qRound(actionRoot.value("type").toDouble());
			Action *action = 0;
			switch(type) {
				case ProcessAction::Type:
				{
					const QJsonObject processRoot = actionRoot.value("process").toObject();

					ProcessAction *processAction = new ProcessAction();
					processAction->process.program = processRoot.value("program").toString();
					processAction->process.arguments = processRoot.value("arguments").toVariant().toStringList();
					processAction->process.waitForFinished = processRoot.value("waitForFinished").toBool();
					processAction->process.detached = processRoot.value("detached").toBool();
					action = processAction;
				}
				break;

				case FileAction::Type:
				{
					FileAction *fileAction = new FileAction();
					fileAction->filePath = actionRoot.value("filePath").toString();
					action = fileAction;
				}
				break;
			}

			if(!action)
				continue;

			action->server = server;
			action->category = static_cast<Action::Category>(qRound(actionRoot.value("category").toDouble()));

			QString iconPath = actionRoot.value("icon").toString();
			action->icon = QIcon(iconPath.startsWith(":/") ? iconPath : dataDir.absoluteFilePath(iconPath));

			QString text = actionRoot.value("text").toString();
			if(text.isEmpty()) {
				const QString locale = qApp->property("locale").toString();
				if(locale.isEmpty() || (locale == "en")) {
					text = actionRoot.value("text_en").toString();
				} else {
					text = actionRoot.value("text_" + locale).toString();
				}
			}
			action->text = text;

			server->actions << action;
		}

		m_servers << server;
	}
}

QList<Server*> ServerLoader::servers() const
{
	return m_servers;
}
