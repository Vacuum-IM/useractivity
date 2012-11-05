#include "useractivitydialog.h"

UserActivityDialog::UserActivityDialog(IUserActivity *AUserActivity,
									   const QHash<QString, ActivityData> &AActivityCatalog,
									   const QList<QString> &AActivityList,
									   Jid &AStreamJid, QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Set activity"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_USERACTIVITY, 0, 0, "windowIcon");

	FUserActivity = AUserActivity;
	FActivityCatalog = AActivityCatalog;
	FStreamJid = AStreamJid;

	QFont itemFont = ui.cmbActivity->font();
	itemFont.setBold(true);

	QList<QString>::const_iterator i;
	for (i = AActivityList.constBegin(); i != AActivityList.constEnd(); ++i)
	{
		ui.cmbActivity->addItem(AActivityCatalog.value(*i).icon, AActivityCatalog.value(*i).locname, *i);
		if(*i == AActivityCatalog.value(*i).general)
			ui.cmbActivity->setItemData(ui.cmbActivity->findData(*i), itemFont, Qt::FontRole);
	}

	ui.cmbActivity->removeItem(ui.cmbActivity->findData(ACTIVITY_NULL));
	ui.cmbActivity->insertItem(0, FUserActivity->activityIcon(ACTIVITY_NULL), FUserActivity->activityName(ACTIVITY_NULL), ACTIVITY_NULL);
	ui.cmbActivity->insertSeparator(1);

	int pos;
	pos = ui.cmbActivity->findData(FUserActivity->contactActivityKey(FStreamJid, FStreamJid));
	if (pos != -1)
	{
		ui.cmbActivity->setCurrentIndex(pos);
		ui.pteText->setPlainText(FUserActivity->contactActivityText(FStreamJid, FStreamJid));
	}
	else
		ui.cmbActivity->setCurrentIndex(0);

	connect(ui.dbbButtons, SIGNAL(accepted()), SLOT(onDialogAccepted()));
	connect(ui.dbbButtons, SIGNAL(rejected()), SLOT(reject()));
}

void UserActivityDialog::onDialogAccepted()
{
	Activity activity;
	QString key = ui.cmbActivity->itemData(ui.cmbActivity->currentIndex()).toString();
	if(key == FActivityCatalog.value(key).general)
		activity.general = key;
	else
	{
		activity.general = FActivityCatalog.value(key).general;
		activity.specific = FActivityCatalog.value(key).specific;
	}
	activity.text = ui.pteText->toPlainText();
	FUserActivity->setActivity(FStreamJid, activity);
	accept();
}

UserActivityDialog::~UserActivityDialog()
{

}
