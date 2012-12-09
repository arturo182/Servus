#ifndef SERVER_H
#define SERVER_H

#include <QStringList>
#include <QIcon>

class Process
{
	public:
		Process() :
			detached(false),
			waitForFinished(false)
		{ }

		QString program;
		QStringList arguments;
		bool detached;
		bool waitForFinished;
};

class Action : public QObject
{
	Q_OBJECT

	public:
		enum Category
		{
			Server = 0,
			Tools,
			Directories
		};

	public:
		Action() :
			server(0)
		{ }

		virtual int type() const = 0;

		class Server *server;
		Category category;
		QString text;
		QIcon icon;
};

class ProcessAction : public Action
{
	public:
		enum { Type = 0 };

		int type() const { return Type; }

		Process process;
};

class FileAction : public Action
{
	public:
		enum { Type = 1 };

		int type() const { return Type; }

		QString filePath;
};

class Server: public QObject
{
	Q_OBJECT

	public:
		QString name;
		QIcon icon;
		QString workingDirectory;
		QStringList configFiles;
		QStringList logFiles;

		Process onStart;
		Process onStop;
		QList<Action*> actions;
};

#endif // SERVER_H
