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
	QString specific;
	QString locname;
	QIcon icon;
};

class IUserActivity
{
public:
	virtual QObject *instance() = 0;
	virtual void setActivity(const Jid &AStreamJid, const Activity &activity) = 0;
	virtual QIcon activityIcon(const QString &keyname) const = 0;
	virtual QString activityName(const QString &keyname) const = 0;
	virtual QIcon contactActivityIcon(const Jid &streamJid, const Jid &contactJid) const = 0;
	virtual QString contactActivityKey(const Jid &streamJid, const Jid &contactJid) const = 0;
	virtual QString contactActivityGeneralKey(const Jid &streamJid, const Jid &contactJid) const = 0;
	virtual QString contactActivitySpecialKey(const Jid &streamJid, const Jid &contactJid) const = 0;
	virtual QString contactActivityName(const Jid &streamJid, const Jid &contactJid) const = 0;
	virtual QString contactActivityText(const Jid &streamJid, const Jid &contactJid) const = 0;
//signals:
};

Q_DECLARE_INTERFACE(IUserActivity,"Vacuum.ExternalPlugin.IUserActivity/0.2")


#endif // IUSERACTIVITY_H
