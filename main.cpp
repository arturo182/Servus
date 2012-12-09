#include <QApplication>
#include <QStringList>
#include <QTranslator>
#include <QLocale>

#include "jsonsettings.h"
#include "serverloader.h"
#include "settings.h"
#include "trayicon.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);

	//settings
	const QString settingsPath = app.applicationDirPath() + "/settings.json";
	JsonSettings settings(settingsPath);

	//locale
	QTranslator translator;
	QTranslator qtTranslator;
	QStringList uiLanguages = QLocale::system().uiLanguages();

	const QString language = settings.value(Settings::languageKey).toString();
	if(!language.isEmpty())
		uiLanguages.prepend(language);

	const QString translationPath = app.applicationDirPath() + "/data/locales/";
	foreach(QString locale, uiLanguages) {
		locale.replace('-', '_');

		if(translator.load("servus_" + locale, translationPath)) {
			const QString qtTrFile = "qt_" + locale;

			if(qtTranslator.load(qtTrFile, translationPath)) {
				app.installTranslator(&translator);
				app.installTranslator(&qtTranslator);
				app.setProperty("locale", locale);

				break;
			}

			translator.load(QString());
		} else if((locale == "C") || locale.startsWith("en")) {
			break;
		}
	}

	//load profiles
	const QString dataDir = app.applicationDirPath() + "/data/profiles/";
	ServerLoader loader(dataDir);
	loader.load();

	//start Servus
	TrayIcon trayIcon(&loader, &settings);
	trayIcon.show();

	return app.exec();
}

