#include "useractivitydialog.h"
#include <QDebug>

UserActivityDialog::UserActivityDialog(IUserActivity *AUserActivity,
									   const QMap<QString, ActivityData> &AActivityCatalog,
									   Jid &AStreamJid, QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Set activity"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_USERACTIVITY, 0, 0, "windowIcon");

	FUserActivity = AUserActivity;
	FActivityCatalog = AActivityCatalog;
	FStreamJid = AStreamJid;

	QMapIterator<QString, ActivityData> i(FActivityCatalog);
	while (i.hasNext())
	{
		i.next();
		if(i.key() == i.value().general)
		{
			ui.cmbGeneral->addItem(i.value().icon, i.value().locname, i.key());
		}
	}
	ui.cmbGeneral->model()->sort(Qt::AscendingOrder);

	ui.cmbGeneral->removeItem(ui.cmbGeneral->findData(ACTIVITY_NULL));
	ui.cmbGeneral->insertItem(0, FUserActivity->activityIcon(ACTIVITY_NULL), FUserActivity->activityName(ACTIVITY_NULL), ACTIVITY_NULL);
	ui.cmbGeneral->insertSeparator(1);

	int pos;
	pos = ui.cmbGeneral->findData(FUserActivity->contactActivityGeneralKey(FStreamJid));
	if (pos != -1)
	{
		ui.cmbGeneral->setCurrentIndex(pos);
		ui.pteText->setPlainText(FUserActivity->contactActivityText(FStreamJid));
	}
	else
		ui.cmbGeneral->setCurrentIndex(0);

	connect(ui.cmbGeneral, SIGNAL(currentIndexChanged(const int &)), SLOT(loadSpecific(const int &)));
	connect(ui.dbbButtons, SIGNAL(accepted()), SLOT(onDialogAccepted()));
	connect(ui.dbbButtons, SIGNAL(rejected()), SLOT(reject()));
}

void UserActivityDialog::loadSpecific(const int &itemKey)
{
	QString itemName = ui.cmbGeneral->itemData(itemKey).toString();
	ui.cmbSpecific->clear();
	if (itemName == ACTIVITY_NULL)
		return;
	else
	{
		QMapIterator<QString, ActivityData> i(FActivityCatalog);
		while (i.hasNext())
		{
			i.next();
			if(itemName == i.value().general)
			{
				ui.cmbSpecific->addItem(i.value().icon, i.value().locname, i.key());
			}
		}

		ui.cmbSpecific->model()->sort(Qt::AscendingOrder);

		ui.cmbSpecific->removeItem(ui.cmbSpecific->findData(ACTIVITY_NULL));
		ui.cmbSpecific->insertItem(0, FUserActivity->activityIcon(ACTIVITY_NULL), FUserActivity->activityName(ACTIVITY_NULL), ACTIVITY_NULL);
		ui.cmbSpecific->insertSeparator(1);

		int pos;
		pos = ui.cmbSpecific->findData(FUserActivity->contactActivityKey(FStreamJid));
		if (pos != -1)
		{
			ui.cmbSpecific->setCurrentIndex(pos);
			//ui.pteText->setPlainText(FUserActivity->contactActivityText(FStreamJid));
		}
		else
			ui.cmbSpecific->setCurrentIndex(0);
	}
}

void UserActivityDialog::onDialogAccepted()
{
	ActivityTempData tempData;
	tempData.activityGeneral = ui.cmbGeneral->itemData(ui.cmbGeneral->currentIndex()).toString();
	tempData.activitySpecific = ui.cmbSpecific->itemData(ui.cmbSpecific->currentIndex()).toString();
	tempData.activityText = ui.pteText->toPlainText();
	FUserActivity->setActivity(FStreamJid, tempData);
	accept();
}

UserActivityDialog::~UserActivityDialog()
{

}
