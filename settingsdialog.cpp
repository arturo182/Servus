#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "settings.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent) :
	QDialog(parent),
	m_ui(new Ui::SettingsDialog),
	m_settings(settings)
{
	m_ui->setupUi(this);

	m_ui->startOnStartupCheckBox->setChecked(m_settings->value(Settings::startOnStartupKey).toBool());
	m_ui->clearLogsOnStartCheckBox->setChecked(m_settings->value(Settings::clearLogsOnStartKey).toBool());

	const QString currentLocale = m_settings->value(Settings::languageKey).toString();

	m_ui->languageComboBox->addItem(tr("System"), QString());
	m_ui->languageComboBox->addItem("English", "C");

	if(currentLocale == "C")
		m_ui->languageComboBox->setCurrentIndex(m_ui->languageComboBox->count() - 1);

	const QString translationPath = qApp->applicationDirPath() + "/data/locales/";
	const QFileInfoList translationFiles = QDir(translationPath).entryInfoList(QStringList() << "servus_*.qm");

	foreach(const QFileInfo &fileInfo, translationFiles) {
		const QString locale = fileInfo.baseName().remove("servus_");

		m_ui->languageComboBox->addItem(QLocale::languageToString(QLocale(locale).language()), locale);
		if(locale == currentLocale) {
			m_ui->languageComboBox->setCurrentIndex(m_ui->languageComboBox->count() - 1);
		}
	}
}

SettingsDialog::~SettingsDialog()
{
	delete m_ui;
}

void SettingsDialog::on_buttonBox_accepted()
{
	m_settings->setValue(Settings::startOnStartupKey, m_ui->startOnStartupCheckBox->isChecked());
	m_settings->setValue(Settings::clearLogsOnStartKey, m_ui->clearLogsOnStartCheckBox->isChecked());

	const int languageIdx = m_ui->languageComboBox->currentIndex();
	QString locale = m_ui->languageComboBox->itemData(languageIdx).toString();
	if(locale.isEmpty()) {
		m_settings->remove(Settings::languageKey);
	} else {
		m_settings->setValue(Settings::languageKey, locale);
	}

	accept();
}
