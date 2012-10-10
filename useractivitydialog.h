#ifndef USERACTIVITYDIALOG_H
#define USERACTIVITYDIALOG_H

#include <QDialog>

#include <definitions/resources.h>
#include <utils/iconstorage.h>
#include <utils/jid.h>

#include "iuseractivity.h"
#include "ui_useractivitydialog.h"

class UserActivityDialog : public QDialog
{
	Q_OBJECT

public:
	UserActivityDialog(IUserActivity *AUserActivity,
					   const QHash<QString, ActivityData> &AActivityCatalog,
					   const QList<QString> &AActivityList,
					   Jid &AStreamJid, QWidget *AParent = 0);
	~UserActivityDialog();

protected slots:
	void onDialogAccepted();

private:
	Ui::UserActivityDialog ui;
	IUserActivity *FUserActivity;
	Jid FStreamJid;
};

#endif // USERACTIVITYDIALOG_H
