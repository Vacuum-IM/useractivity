#ifndef USERACTIVITY_H
#define USERACTIVITY_H

#include "iuseractivity.h"
#include "useractivitydialog.h"
#include "ui_useractivitydialog.h"

#include <interfaces/imainwindow.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irostermanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreammanager.h>

#include <definitions/menuicons.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rostertooltiporders.h>

#include <utils/action.h>
#include <utils/advanceditemdelegate.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>

class UserActivity :
	public QObject,
	public IPlugin,
	public IUserActivity,
	public IRosterDataHolder,
	public IRostersLabelHolder,
	public IOptionsDialogHolder,
	public IPEPHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IUserActivity IRosterDataHolder IRostersLabelHolder IOptionsDialogHolder IPEPHandler);

public:
	UserActivity();
	~UserActivity();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return USERACTIVITY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	//IPEPHandler
	virtual bool processPEPEvent(const Jid &streamJid, const Stanza &stanza);
	//IUserActivity
	virtual void setActivity(const Jid &streamJid, const Activity &activity);
	virtual QIcon activityIcon(const QString &keyname) const;
	virtual QString activityName(const QString &keyname) const;
	virtual QIcon contactActivityIcon(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactActivityKey(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactActivityGeneralKey(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactActivitySpecialKey(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactActivityName(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactActivityText(const Jid &streamJid, const Jid &contactJid) const;

signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);

protected slots:
	//INotification
	void onShowNotification(const Jid &streamJid, const Jid &senderJid);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	//IRostersView
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	//IXmppStreams
	void onStreamClosed(IXmppStream *AXmppStream);
	//IPresencePlugin
	void onContactStateChanged(const Jid &streamJid, const Jid &contactJid, bool AStateOnline);
	//IOptionsHolder
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);

	void onSetActivityActionTriggered(bool);
	void onApplicationQuit();

protected:
	void addActivity(const QString &keyname, const QString &general, const QString &locname);
	Action *createSetActivityAction(const Jid &streamJid, const QString &AFeature, QObject *AParent) const;
	void setContactActivity(const Jid &streamJid, const Jid &senderJid, const Activity &activity);

	//IRosterDataHolder
	void updateDataHolder(const Jid &streamJid, const Jid &senderJid);

private:
	IMainWindowPlugin *FMainWindowPlugin;
	IPresenceManager *FPresenceManager;
	IPEPManager *FPEPManager;
	IServiceDiscovery *FDiscovery;
	IXmppStreamManager *FXmppStreamManager;
	IOptionsManager *FOptionsManager;
	IRosterManager *FRosterManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	INotifications *FNotifications;

private:
	int handlerId;
	bool FActivityIconsVisible;
	quint32 FActivityLabelId;

	QMap<int, Jid> FNotifies;
	QList<QString> FActivityList;
	QHash<QString, ActivityData> FActivities;
	QHash<Jid, QHash <QString, Activity> > FContacts;
};

#endif // USERACTIVITY_H
