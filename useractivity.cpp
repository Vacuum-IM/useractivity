#include "useractivity.h"

#define ADR_STREAM_JID Action::DR_StreamJid
#define RDR_USERACTIVITY 455

static const QList<int> RosterKinds = QList<int>() << RIK_CONTACT << RIK_CONTACTS_ROOT << RIK_STREAM_ROOT;

UserActivity::UserActivity()
{
	FMainWindowPlugin = NULL;
	FPresenceManager = NULL;
	FPEPManager = NULL;
	FDiscovery = NULL;
	FXmppStreamManager = NULL;
	FOptionsManager = NULL;
	FRosterManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FNotifications = NULL;
	FActivityLabelId = 0;

#ifdef DEBUG_RESOURCES_DIR
	FileStorage::setResourcesDirs(FileStorage::resourcesDirs() << DEBUG_RESOURCES_DIR);
#endif
}

UserActivity::~UserActivity()
{

}

void UserActivity::addActivity(const QString &general, const QString &keyname, const QString &locname)
{
	ActivityData data = {general, keyname, locname, IconStorage::staticStorage(RSR_STORAGE_ACTIVITYICONS)->getIcon(keyname)};
	FActivities.insert(keyname, data);
	FActivityList.append(keyname);
}

void UserActivity::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Activity");
	APluginInfo->description = tr("Allows you to send and receive information about user activities");
	APluginInfo->version = "0.8";
	APluginInfo->author = "Alexey Ivanov aka krab";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool UserActivity::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 40;

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0);
	if(plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPEPManager").value(0);
	if(plugin)
	{
		FPEPManager = qobject_cast<IPEPManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0, NULL);
	if(plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0, NULL);
	if(plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if(FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0, NULL);
	if(plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if(FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
					SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if(plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0, NULL);
	if(plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if(FNotifications)
		{
			connect(FNotifications->instance(), SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(), SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0, NULL);
	if(plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
			connect(Options::instance(),SIGNAL(optionsChanged(OptionsNode)),SLOT(onOptionsChanged(OptionsNode)));
		}
	}

	connect(APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));

	return FMainWindowPlugin != NULL && FRosterManager !=NULL && FPEPManager !=NULL;
}

bool UserActivity::initObjects()
{
	handlerId = FPEPManager->insertNodeHandler(ACTIVITY_PROTOCOL_URL, this);

	IDiscoFeature feature;
	feature.active = true;
	feature.name = tr("User Activity");
	feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERACTIVITY);
	feature.description = tr("Supports the exchange of information about user activities");
	feature.var = ACTIVITY_PROTOCOL_URL;
	FDiscovery->insertDiscoFeature(feature);

	feature.name = tr("User activity notification");
	feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERACTIVITY);
	feature.description = tr("Supports the exchange of information about user activities");
	feature.var = ACTIVITY_NOTIFY_PROTOCOL_URL;
	FDiscovery->insertDiscoFeature(feature);

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_USERACTIVITY_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERACTIVITY);
		notifyType.title = tr("When receiving activity");
		notifyType.kindMask = INotification::PopupWindow;
		FNotifications->registerNotificationType(NNT_USERACTIVITY,notifyType);
	}

	if(FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_USERACTIVITY,this);
	}

	if(FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_USERACTIVITY);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = RDR_USERACTIVITY;
		FActivityLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);

		FRostersViewPlugin->rostersView()->insertLabelHolder(RLHO_USERACTIVITY,this);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}

	addActivity(ACTIVITY_NULL, ACTIVITY_NULL, tr("Without activity"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_DOING_CHORES, tr("Doing chores"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_BUYING_GROCERIES, tr("Buying groceries"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_CLEANING, tr("Cleaning"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_COOKING, tr("Cooking"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_DOING_MAINTENANCE, tr("Doing maintenance"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_DOING_THE_DISHES, tr("Doing the dishes"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_DOING_THE_LAUNDRY, tr("Doing the laundry"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_GARDENING, tr("Gardening"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_RUNNING_AN_ERRAND, tr("Running an errand"));
	addActivity(ACTIVITY_DOING_CHORES, ACTIVITY_WALKING_THE_DOG, tr("Walking the dog"));

	addActivity(ACTIVITY_DRINKING, ACTIVITY_DRINKING, tr("Drinking"));
	addActivity(ACTIVITY_DRINKING, ACTIVITY_HAVING_A_BEER, tr("Having a beer"));
	addActivity(ACTIVITY_DRINKING, ACTIVITY_HAVING_COFFEE, tr("Having coffee"));
	addActivity(ACTIVITY_DRINKING, ACTIVITY_HAVING_TEA, tr("Having tea"));

	addActivity(ACTIVITY_EATING, ACTIVITY_EATING, tr("Eating"));
	addActivity(ACTIVITY_EATING, ACTIVITY_HAVING_A_SNACK, tr("Having a snack"));
	addActivity(ACTIVITY_EATING, ACTIVITY_HAVING_BREAKFAST, tr("Having breakfast"));
	addActivity(ACTIVITY_EATING, ACTIVITY_HAVING_DINNER, tr("Having dinner"));
	addActivity(ACTIVITY_EATING, ACTIVITY_HAVING_LUNCH, tr("Having lunch"));

	addActivity(ACTIVITY_EXERCISING, ACTIVITY_EXERCISING, tr("Exercising"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_CYCLING, tr("Cycling"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_DANCING, tr("Dancing"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_HIKING, tr("Hiking"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_JOGGING, tr("Jogging"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_PLAYING_SPORTS, tr("Playing sports"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_RUNNING, tr("Running"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_SKIING, tr("Skiing"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_SWIMMING, tr("Swimming"));
	addActivity(ACTIVITY_EXERCISING, ACTIVITY_WORKING_OUT, tr("Working out"));

	addActivity(ACTIVITY_GROOMING, ACTIVITY_GROOMING, tr("Grooming"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_AT_THE_SPA, tr("At the spa"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_BRUSHING_TEETH, tr("Brushing teeth"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_GETTING_A_HAIRCUT, tr("Getting a haircut"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_SHAVING, tr("Shaving"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_TAKING_A_BATH, tr("Taking a bath"));
	addActivity(ACTIVITY_GROOMING, ACTIVITY_TAKING_A_SHOWER, tr("Taking a shower"));

	addActivity(ACTIVITY_HAVING_APPOINTMENT, ACTIVITY_HAVING_APPOINTMENT, tr("Having appointment"));

	addActivity(ACTIVITY_INACTIVE, ACTIVITY_INACTIVE, tr("Inactive"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_DAY_OFF, tr("Day off"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_HANGING_OUT, tr("Hanging out"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_HIDING, tr("Hiding"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_ON_VACATION, tr("On vacation"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_PRAYING, tr("Praying"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_SCHEDULED_HOLIDAY, tr("Sheduled holiday"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_SLEEPING, tr("Sleeping"));
	addActivity(ACTIVITY_INACTIVE, ACTIVITY_THINKING, tr("Thinking"));

	addActivity(ACTIVITY_RELAXING, ACTIVITY_RELAXING, tr("Relaxing"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_FISHING, tr("Fishing"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_GAMING, tr("Gaming"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_GOING_OUT, tr("Going out"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_PARTYING, tr("Partying"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_READING, tr("Reading"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_REHEARSING, tr("Rehearsing"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_SHOPPING, tr("Shopping"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_SMOKING, tr("Smoking"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_SOCIALIZING, tr("Socializing"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_SUNBATHING, tr("Sunbathing"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_WATCHING_TV, tr("Watching tv"));
	addActivity(ACTIVITY_RELAXING, ACTIVITY_WATCHING_A_MOVIE, tr("Watching a movie"));

	addActivity(ACTIVITY_TALKING, ACTIVITY_TALKING, tr("Talking"));
	addActivity(ACTIVITY_TALKING, ACTIVITY_IN_REAL_LIFE, tr("In real life"));
	addActivity(ACTIVITY_TALKING, ACTIVITY_ON_THE_PHONE, tr("On the phone"));
	addActivity(ACTIVITY_TALKING, ACTIVITY_ON_VIDEO_PHONE, tr("On video phone"));

	addActivity(ACTIVITY_TRAVELING, ACTIVITY_TRAVELING, tr("Traveling"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_COMMUTING, tr("Commuting"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_CYCLING, tr("Cycling"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_DRIVING, tr("Driving"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_IN_A_CAR, tr("In a car"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_ON_A_BUS, tr("On a bus"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_ON_A_PLANE, tr("On a plane"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_ON_A_TRAIN, tr("On a train"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_ON_A_TRIP, tr("On a trip"));
	addActivity(ACTIVITY_TRAVELING, ACTIVITY_WALKING, tr("Walking"));

	addActivity(ACTIVITY_WORKING, ACTIVITY_WORKING, tr("Working"));
	addActivity(ACTIVITY_WORKING, ACTIVITY_CODING, tr("Coding"));
	addActivity(ACTIVITY_WORKING, ACTIVITY_IN_A_MEETING, tr("In a meeting"));
	addActivity(ACTIVITY_WORKING, ACTIVITY_STUDYING, tr("Studying"));
	addActivity(ACTIVITY_WORKING, ACTIVITY_WRITING, tr("Writing"));

	return true;
}

bool UserActivity::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_USER_ACTIVITY_ICON_SHOW,true);

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> UserActivity::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_ROSTERVIEW)
	{
		widgets.insertMulti(OWO_ROSTER_USERACTIVITY,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_USER_ACTIVITY_ICON_SHOW),tr("Show contact activities icon"),AParent));
	}
	return widgets;
}


QList<int> UserActivity::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_USERACTIVITY)
		return QList<int>() << RDR_USERACTIVITY;
	return QList<int>();
}

QVariant UserActivity::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder == RDHO_USERACTIVITY)
	{
		switch (AIndex->kind())
		{
		case RIK_STREAM_ROOT:
		case RIK_CONTACT:
			{
				if (ARole == RDR_USERACTIVITY)
				{
					Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
					Jid senderJid = AIndex->data(RDR_PREP_BARE_JID).toString();
					return QIcon(contactActivityIcon(streamJid, senderJid));
				}
			}
			break;
		}
	}
	return QVariant();
}

bool UserActivity::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

QList<quint32> UserActivity::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder==RLHO_USERACTIVITY && FActivityIconsVisible && !AIndex->data(RDR_USERACTIVITY).isNull())
		labels.append(FActivityLabelId);
	return labels;
}

AdvancedDelegateItem UserActivity::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AIndex);
	return FRostersViewPlugin->rostersView()->registeredLabel(ALabelId);
}

bool UserActivity::processPEPEvent(const Jid &streamJid, const Stanza &stanza)
{
	QDomElement replyElem = stanza.document().firstChildElement("message");
	if(!replyElem.isNull())
	{
		Activity data;
		Jid senderJid = replyElem.attribute("from");

		QDomElement eventElem = replyElem.firstChildElement("event");
		if(!eventElem.isNull())
		{
			QDomElement itemsElem = eventElem.firstChildElement("items");
			if(!itemsElem.isNull())
			{
				QDomElement itemElem = itemsElem.firstChildElement("item");
				if(!itemElem.isNull())
				{
					QDomElement activityElem = itemElem.firstChildElement("activity");
					if(!activityElem.isNull())
					{
						QDomElement generalElem = activityElem.firstChildElement();
						if(!generalElem.isNull() && FActivities.contains(generalElem.nodeName()))
						{
							data.general = generalElem.nodeName();
							QDomElement specificElem = generalElem.firstChildElement();
							if (!specificElem.isNull() && FActivities.contains(specificElem.nodeName()))
							{
								data.specific = specificElem.nodeName();
							}
						}
						QDomElement textElem = activityElem.firstChildElement("text");
						if(!textElem.isNull())
						{
							data.text = textElem.text();
						}
					}
					else
						return false;
				}
			}
		}
		setContactActivity(streamJid, senderJid, data);
	}
	else
		return false;

	return true;
}

void UserActivity::setActivity(const Jid &streamJid, const Activity &activity)
{
	QDomDocument docElem("");
	QDomElement rootElem = docElem.createElement("item");
	docElem.appendChild(rootElem);

	QDomElement activityElem = docElem.createElementNS(ACTIVITY_PROTOCOL_URL, "activity");
	rootElem.appendChild(activityElem);
	if(activity.general != ACTIVITY_NULL)
	{
		QDomElement generalElem = docElem.createElement(activity.general);
		activityElem.appendChild(generalElem);
		if(!activity.specific.isEmpty())
		{
			QDomElement specificElem = docElem.createElement(activity.specific);
			generalElem.appendChild(specificElem);
		}
		if(!activity.text.isEmpty())
		{
			QDomElement textElem = docElem.createElement("text");
			activityElem.appendChild(textElem);
			QDomText t1 = docElem.createTextNode(activity.text);
			textElem.appendChild(t1);
		}
	}
	else
	{
		QDomElement nameElem = docElem.createElement("");
		activityElem.appendChild(nameElem);
	}

	FPEPManager->publishItem(streamJid, ACTIVITY_PROTOCOL_URL, rootElem);
}

void UserActivity::onShowNotification(const Jid &streamJid, const Jid &senderJid)
{
	if (FNotifications && FContacts[streamJid].contains(senderJid.pBare()) && streamJid.pBare() != senderJid.pBare())
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERACTIVITY);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERACTIVITY;
			notify.data.insert(NDR_ICON,contactActivityIcon(streamJid, senderJid));
			notify.data.insert(NDR_STREAM_JID,streamJid.full());
			notify.data.insert(NDR_CONTACT_JID,senderJid.full());
			notify.data.insert(NDR_TOOLTIP,QString("%1 %2").arg(FNotifications->contactName(streamJid, senderJid)).arg(tr("changed activity")));
			notify.data.insert(NDR_POPUP_CAPTION,tr("Activity changed"));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid, senderJid));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(senderJid));
			if(!contactActivityText(streamJid,senderJid).isEmpty())
				notify.data.insert(NDR_POPUP_TEXT,QString("%1:\n%2").arg(contactActivityName(streamJid, senderJid)).arg(contactActivityText(streamJid, senderJid)));
			else
				notify.data.insert(NDR_POPUP_TEXT,QString("%1").arg(contactActivityName(streamJid, senderJid)));
			FNotifies.insert(FNotifications->appendNotification(notify),senderJid);
		}
	}
}

void UserActivity::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
		FNotifications->removeNotification(ANotifyId);
}

void UserActivity::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
		FNotifies.remove(ANotifyId);
}

void UserActivity::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		IRosterIndex *index = AIndexes.first();
		if(index->kind() == RIK_STREAM_ROOT)
		{
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresenceManager != NULL ? FPresenceManager->findPresence(streamJid) : NULL;
			if(presence && presence->isOpen())
			{
				int show = index->data(RDR_SHOW).toInt();
				if(show != IPresence::Offline && show != IPresence::Error && FPEPManager->isSupported(streamJid))
				{
					Action *action = createSetActivityAction(streamJid, ACTIVITY_PROTOCOL_URL, AMenu);
					AMenu->addAction(action, AG_RVCM_USERACTIVITY, false);
				}
			}
		}
	}
}

void UserActivity::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if ((ALabelId==AdvancedDelegateItem::DisplayId && RosterKinds.contains(AIndex->kind())) || ALabelId == FActivityLabelId)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!contactActivityKey(streamJid, contactJid).isEmpty())
		{
			QString tooltip_full = QString("<b>%1</b> %2<br>%3</div>")
					.arg(tr("Activity:")).arg(contactActivityName(streamJid, contactJid)).arg(contactActivityText(streamJid, contactJid));
			QString tooltip_short = QString("<b>%1</b> %2</div>")
					.arg(tr("Activity:")).arg(contactActivityName(streamJid, contactJid));
			AToolTips.insert(RTTO_USERACTIVITY, contactActivityText(streamJid, contactJid).isEmpty() ? tooltip_short : tooltip_full);
		}
	}
}

Action *UserActivity::createSetActivityAction(const Jid &streamJid, const QString &AFeature, QObject *AParent) const
{
	if(AFeature == ACTIVITY_PROTOCOL_URL)
	{
		Action *action = new Action(AParent);
		action->setText(tr("Activity"));
		QIcon menuicon;
		if(!contactActivityIcon(streamJid, streamJid).isNull())
			menuicon = contactActivityIcon(streamJid, streamJid);
		else
			menuicon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERACTIVITY);
		action->setIcon(menuicon);
		action->setData(ADR_STREAM_JID, streamJid.full());
		connect(action, SIGNAL(triggered(bool)), SLOT(onSetActivityActionTriggered(bool)));
		return action;
	}
	return NULL;
}

void UserActivity::onSetActivityActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if(action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		UserActivityDialog *dialog;
		dialog = new UserActivityDialog(this, FActivities, FActivityList, streamJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserActivity::setContactActivity(const Jid &streamJid, const Jid &senderJid, const Activity &activity)
{
	if((contactActivityGeneralKey(streamJid, senderJid) != activity.general) ||
		(contactActivitySpecialKey(streamJid, senderJid) != activity.specific) ||
		(contactActivityText(streamJid, senderJid) != activity.text))
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
		if((roster!=NULL && !roster->findItem(senderJid).isNull()) || streamJid.pBare() == senderJid.pBare())
		{
			if(!activity.general.isEmpty())
			{
				FContacts[streamJid].insert(senderJid.pBare(), activity);
				onShowNotification(streamJid, senderJid);
			}
			else
				FContacts[streamJid].remove(senderJid.pBare());
		}
	}
	updateDataHolder(streamJid, senderJid);
}

void UserActivity::updateDataHolder(const Jid &streamJid, const Jid &senderJid)
{
	if(FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		if (!streamJid.isEmpty())
			findData.insert(RDR_STREAM_JID,streamJid.pFull());
		if (!senderJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID,senderJid.pBare());
		findData.insert(RDR_KIND,RIK_STREAM_ROOT);
		findData.insert(RDR_KIND,RIK_CONTACT);
		findData.insert(RDR_KIND,RIK_CONTACTS_ROOT);

		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			emit rosterDataChanged(index, RDR_USERACTIVITY);
	}
}

void UserActivity::onStreamClosed(IXmppStream *AXmppStream)
{
	FContacts.remove(AXmppStream->streamJid());
}

void UserActivity::onContactStateChanged(const Jid &streamJid, const Jid &contactJid, bool AStateOnline)
{
	if (!AStateOnline && FContacts[streamJid].contains(contactJid.pBare()))
	{
		FContacts[streamJid].remove(contactJid.pBare());
		updateDataHolder(streamJid, contactJid);
	}
}

void UserActivity::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_USER_ACTIVITY_ICON_SHOW));
}

void UserActivity::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_USER_ACTIVITY_ICON_SHOW)
	{
		FActivityIconsVisible = ANode.value().toBool();
		emit rosterLabelChanged(FActivityLabelId,NULL);
	}
}

QIcon UserActivity::activityIcon(const QString &keyname) const
{
	return FActivities.value(keyname).icon;
}

QString UserActivity::activityName(const QString &keyname) const
{
	return FActivities.value(keyname).locname;
}

QIcon UserActivity::contactActivityIcon(const Jid &streamJid, const Jid &contactJid) const
{
	return !FContacts[streamJid].value(contactJid.pBare()).specific.isNull() ?
				FActivities.value(FContacts[streamJid].value(contactJid.pBare()).specific).icon :
				FActivities.value(FContacts[streamJid].value(contactJid.pBare()).general).icon;
}

QString UserActivity::contactActivityKey(const Jid &streamJid, const Jid &contactJid) const
{
	return !FContacts[streamJid].value(contactJid.pBare()).specific.isNull() ?
				FContacts[streamJid].value(contactJid.pBare()).specific :
				FContacts[streamJid].value(contactJid.pBare()).general;
}

QString UserActivity::contactActivityGeneralKey(const Jid &streamJid, const Jid &contactJid) const
{
	return FContacts[streamJid].value(contactJid.pBare()).general;
}

QString UserActivity::contactActivitySpecialKey(const Jid &streamJid, const Jid &contactJid) const
{
	return FContacts[streamJid].value(contactJid.pBare()).specific;
}

QString UserActivity::contactActivityName(const Jid &streamJid, const Jid &contactJid) const
{
	return !FContacts[streamJid].value(contactJid.pBare()).specific.isNull() ?
				FActivities.value(FContacts[streamJid].value(contactJid.pBare()).specific).locname :
				FActivities.value(FContacts[streamJid].value(contactJid.pBare()).general).locname;
}

QString UserActivity::contactActivityText(const Jid &streamJid, const Jid &contactJid) const
{
	QString text = FContacts[streamJid].value(contactJid.pBare()).text;
	return text.replace("\n","<br>").toHtmlEscaped();
}

void UserActivity::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}
