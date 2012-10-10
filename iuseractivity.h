#ifndef IUSERACTIVITY_H
#define IUSERACTIVITY_H

#include <QString>
#include <QIcon>

#include "definitions.h"
#include <utils/iconstorage.h>
#include <utils/jid.h>

#define USERACTIVITY_UUID "{f916b5a0-0cc2-11e2-892e-0800200c9a66}"
#define PEP_USERACTIVITY 4020

struct Activity
{
	QString general;
	QString specific;
	QString text;
};

struct ActivityData
{
	QString general;
	QString locname;
	QIcon icon;
};

class ActivityContact
{
public:
	QString keyname;
	QString text;
};

class IUserActivity
{
public:
	virtual QObject *instance() = 0;
	virtual void setActivity(const Jid &AStreamJid, const ActivityContact &contact) = 0;
	virtual QIcon activityIcon(const QString &keyname) const = 0;
	virtual QString activityName(const QString &keyname) const = 0;
	virtual QIcon contactActivityIcon(const Jid &contactJid) const = 0;
	virtual QString contactActivityKey(const Jid &contactJid) const = 0;
	virtual QString contactActivityGeneralKey(const Jid &contactJid) const = 0;
	virtual QString contactActivityName(const Jid &contactJid) const = 0;
	virtual QString contactActivityText(const Jid &contactJid) const = 0;
//signals:
};

Q_DECLARE_INTERFACE(IUserActivity,"Vacuum.ExternalPlugin.IUserActivity/0.1")


#endif // IUSERACTIVITY_H
