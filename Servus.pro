QT				 = core gui network widgets
DESTDIR			 = bin
RC_FILE			 = servus.rc
QMAKE_CXXFLAGS	+= -std=c++11


SOURCES			+= main.cpp \
				   settingsdialog.cpp \
				   trayicon.cpp \
				   serverloader.cpp \
				   jsonsettings.cpp

HEADERS			+= settingsdialog.h \
				   server.h \
				   trayicon.h \
				   serverloader.h \
				   jsonsettings.h \
				   settings.h

RESOURCES		+= res.qrc

FORMS			+= settingsdialog.ui

TRANSLATIONS	 = translations/servus_untranslated.ts translations/servus_pl.ts
