#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSettings;

namespace Ui
{
	class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit SettingsDialog(QSettings *settings, QWidget *parent = 0);
		~SettingsDialog();

	private slots:
		void on_buttonBox_accepted();

	private:
		Ui::SettingsDialog *m_ui;
		QSettings *m_settings;
};

#endif // SETTINGSDIALOG_H
