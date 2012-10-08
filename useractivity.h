#ifndef USERACTIVITY_H
#define USERACTIVITY_H

#include "definitions.h"
#include "iuseractivity.h"
#include "useractivitydialog.h"
#include "ui_useractivitydialog.h"

#include <interfaces/imainwindow.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>

#include <definitions/menuicons.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>

#include <utils/action.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>

class UserActivity :
	public QObject,
	public IPlugin,
	public IUserActivity,
	public IRosterDataHolder,
	public IPEPHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IUserActivity IRosterDataHolder IPEPHandler);

public:
	UserActivity();
	~UserActivity();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return USERACTIVITY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }

	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);

	//IPEPHandler
	virtual bool processPEPEvent(const Jid &streamJid, const Stanza &stanza);

	//IUserMood
	virtual void setActivity(const Jid &AStreamJid, const ActivityTempData &tempData);
	virtual QIcon activityIcon(const QString &keyname) const;
	virtual QString activityName(const QString &keyname) const;
	virtual QIcon contactActivityIcon(const Jid &contactJid) const;
	virtual QString contactActivityKey(const Jid &contactJid) const;
	virtual QString contactActivityGeneralKey(const Jid &contactJid) const;
	virtual QString contactActivityName(const Jid &contactJid) const;
	virtual QString contactActivityText(const Jid &contactJid) const;

signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);

protected slots:
//    void onOptionsOpened();
//    void onOptionsChanged(const OptionsNode &ANode);
//    void onRosterIndexInserted(const Jid &AContactJid, const QString &AMood);
	void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int, QString> &AToolTips);
	void onShowNotification(const Jid &AStreamJid, const Jid &AContactJid);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onSetActivityActionTriggered(bool);
	void onApplicationQuit();

protected:
	void addActivity(const QString &keyname, const QString &general, const QString &locname);
	Action *createSetActivityAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const;
	void setContactActivity(const Jid &streamJid, const Jid &senderJid, const ActivityTempData &tempData);

	//IRosterDataHolder
	void updateDataHolder(const Jid &ASenderJid = Jid::null);

private:
	IMainWindowPlugin *FMainWindowPlugin;
	IPresencePlugin *FPresencePlugin;
	IPEPManager *FPEPManager;
	IServiceDiscovery *FDiscovery;
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	INotifications *FNotifications;

	int handlerId;
	int FUserActivityLabelId;

	QMap<int, Jid> FNotifies;
	//QMap<QString/*general*/, QMap<QString/*spicific*/, ActivityData> > FActivityCatalog;
	QMap<QString, ActivityData> FActivityCatalog;

	QMap<QString, ActivityContact> FActivityContact;

};

#endif // USERACTIVITY_H


