#ifndef SERVERLOADER_H
#define SERVERLOADER_H

#include <QString>
#include <QList>

class Server;

class ServerLoader
{
	public:
		ServerLoader(const QString &dataDir);

		void load();

		QList<Server*> servers() const;

	private:
		QString m_dataDir;
		QList<Server*> m_servers;
};

#endif // SERVERLOADER_H
