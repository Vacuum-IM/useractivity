#include "useractivity.h"

#define ADR_STREAM_JID	Action::DR_StreamJid
#define RDR_ACTIVITY_NAME	453

UserActivity::UserActivity()
{
	FMainWindowPlugin = NULL;
	FPEPManager = NULL;
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
}

void UserActivity::addActivity(const QString &general, const QString &keyname, const QString &locname)
{
	ActivityData data = {general, locname, IconStorage::staticStorage(RSR_STORAGE_ACTIVITYICONS)->getIcon(keyname)};
	FActivityCatalog.insert(keyname, data);
	FActivityList.append(keyname);
}

UserActivity::~UserActivity()
{

}

void UserActivity::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Activity");
	APluginInfo->description = tr("Allows you to send and receive information about user activities");
	APluginInfo->version = "0.1";
	APluginInfo->author = "Alexey Ivanov aka krab";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool UserActivity::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 12;

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

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0, NULL);
	if(plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0, NULL);
	if(plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if(FPresencePlugin)
		{
			//        connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
			//              SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if(plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if(FRostersModel)
		{
			connect(FRostersModel->instance(), SIGNAL(indexInserted(IRosterIndex *)), SLOT(onRosterIndexInserted(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0, NULL);
	if(plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
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
	}

	connect(Options::instance(), SIGNAL(optionsOpened()), SLOT(onOptionsOpened()));
	connect(Options::instance(), SIGNAL(optionsChanged(const OptionsNode &)), SLOT(onOptionsChanged(const OptionsNode &)));

	connect(APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));


	return FMainWindowPlugin != NULL;
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
		FRostersModel->insertDefaultDataHolder(this);
	}

	if(FRostersViewPlugin)
	{
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)),SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexToolTips(IRosterIndex *, int, QMultiMap<int, QString> &)),SLOT(onRosterIndexToolTips(IRosterIndex *, int, QMultiMap<int, QString> &)));
	}

	if(FRostersViewPlugin)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
		findData.insertMulti(RDR_TYPE, type);
		QList<IRosterIndex *> indexes = FRostersModel->rootIndex()->findChilds(findData, true);

		IRostersLabel label;
		label.order = RLO_USERACTIVITY;
		label.value = RDR_ACTIVITY_NAME;
		FUserActivityLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);

		foreach(IRosterIndex * index, indexes)
		FRostersViewPlugin->rostersView()->insertLabel(FUserActivityLabelId, index);
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


int UserActivity::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> UserActivity::rosterDataRoles() const
{
	static const QList<int> indexRoles = QList<int>() << RDR_ACTIVITY_NAME;
	return indexRoles;
}

QList<int> UserActivity::rosterDataTypes() const
{
	static const QList<int> indexTypes = QList<int>() << RIT_STREAM_ROOT << RIT_CONTACT;
	return indexTypes;
}

QVariant UserActivity::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	if(ARole == RDR_ACTIVITY_NAME)
	{
		QIcon pic = contactActivityIcon(AIndex->data(RDR_PREP_BARE_JID).toString());
		return pic;
	}
	return QVariant();
}

bool UserActivity::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

bool UserActivity::processPEPEvent(const Jid &streamJid, const Stanza &stanza)
{
	QDomElement replyElem = stanza.document().firstChildElement("message");
	if(!replyElem.isNull())
	{
		Jid senderJid;
		Activity data;

		senderJid = replyElem.attribute("from");
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
						if(!generalElem.isNull() && FActivityCatalog.contains(generalElem.nodeName()))
						{
							data.general = generalElem.nodeName();
							QDomElement specificElem = generalElem.firstChildElement();
							if (!specificElem.isNull() && FActivityCatalog.contains(specificElem.nodeName()))
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

void UserActivity::setActivity(const Jid &streamJid, const ActivityContact &contact)
{
	QDomDocument doc("");
	QDomElement root = doc.createElement("item");
	doc.appendChild(root);

	QDomElement activity = doc.createElementNS(ACTIVITY_PROTOCOL_URL, "activity");
	root.appendChild(activity);
	if(contact.keyname != ACTIVITY_NULL)
	{
		QDomElement general = doc.createElement(FActivityCatalog.value(contact.keyname).general);
		activity.appendChild(general);
		if((contact.keyname != FActivityCatalog.value(contact.keyname).general) &&
				(((contact.keyname != ACTIVITY_NULL) ||
				  !contact.keyname.isEmpty())))
		{
			QDomElement specific = doc.createElement(contact.keyname);
			general.appendChild(specific);
		}
	}
	else
	{
		QDomElement name = doc.createElement("");
		activity.appendChild(name);
	}
	if((contact.keyname != ACTIVITY_NULL) && (!contact.text.isEmpty()))
	{
		QDomElement text = doc.createElement("text");
		activity.appendChild(text);

		QDomText t1 = doc.createTextNode(contact.text);
		text.appendChild(t1);
	}
	FPEPManager->publishItem(streamJid, ACTIVITY_PROTOCOL_URL, root);
}

void UserActivity::onShowNotification(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FNotifications && FActivityContact.contains(AContactJid.pBare()) /*&& AContactJid.pBare() != AStreamJid.pBare()*/)
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERACTIVITY);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERACTIVITY;
			notify.data.insert(NDR_ICON,contactActivityIcon(AContactJid));
			notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
			notify.data.insert(NDR_CONTACT_JID,AContactJid.full());
			notify.data.insert(NDR_POPUP_CAPTION,tr("User Activity Notification"));
			notify.data.insert(NDR_POPUP_TITLE,QString("%1 %2").arg(FNotifications->contactName(AStreamJid, AContactJid)).arg(tr("changed activity")));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AContactJid));
			notify.data.insert(NDR_POPUP_HTML,QString("[%1] %2").arg(contactActivityName(AContactJid)).arg(contactActivityText(AContactJid)));
			FNotifies.insert(FNotifications->appendNotification(notify),AContactJid);
		}
	}
}

void UserActivity::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifications->removeNotification(ANotifyId);
	}
}

void UserActivity::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}

void UserActivity::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if(ALabelId == RLID_DISPLAY && AIndexes.count() == 1)
	{
		IRosterIndex *index = AIndexes.first();
		if(index->type() == RIT_STREAM_ROOT)
		{
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin != NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
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

Action *UserActivity::createSetActivityAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const
{
	if(AFeature == ACTIVITY_PROTOCOL_URL)
	{
		Action *action = new Action(AParent);
		action->setText(tr("Activity"));
		QIcon menuicon;
		if(!contactActivityIcon(AStreamJid).isNull())
			menuicon = contactActivityIcon(AStreamJid);
		else
			menuicon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERACTIVITY);
		action->setIcon(menuicon);
		action->setData(ADR_STREAM_JID, AStreamJid.full());
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
		dialog = new UserActivityDialog(this, FActivityCatalog, FActivityList, streamJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserActivity::setContactActivity(const Jid &streamJid, const Jid &senderJid, const Activity &data)
{
	if((((contactActivityKey(senderJid) != data.general) &&
		(contactActivityKey(senderJid) != data.specific)) ||
		contactActivityText(senderJid) != data.text))
	{
		if(!data.general.isEmpty())
		{
			ActivityContact contact;
			contact.keyname = data.general;
			if ((!data.specific.isEmpty()) && (data.specific != ACTIVITY_NULL))
			{
				contact.keyname = data.specific;
			}
			contact.text = data.text;
			FActivityContact.insert(senderJid.pBare(), contact);
			onShowNotification(streamJid, senderJid);
		}
		else
			FActivityContact.remove(senderJid.pBare());
	}
	updateDataHolder(senderJid);
}

void UserActivity::updateDataHolder(const Jid &ASenderJid)
{
	if(FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
		findData.insert(RDR_TYPE, type);
		if(!ASenderJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID, ASenderJid.pBare());
		QList<IRosterIndex *> indexes = FRostersModel->rootIndex()->findChilds(findData, true);
		foreach(IRosterIndex *index, indexes)
		{
			emit rosterDataChanged(index, RDR_ACTIVITY_NAME);
		}
	}
}

void UserActivity::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if(FRostersViewPlugin && rosterDataTypes().contains(AIndex->type()))
	{
		FRostersViewPlugin->rostersView()->insertLabel(FUserActivityLabelId, AIndex);
	}
}

void UserActivity::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int, QString> &AToolTips)
{
	if(ALabelId == RLID_DISPLAY || ALabelId == FUserActivityLabelId)
	{
		Jid AContactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!contactActivityKey(AContactJid).isEmpty())
		{
			QString tooltip_full = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>")
					.arg(tr("Activity:")).arg(contactActivityName(AContactJid)).arg(contactActivityText(AContactJid));
			QString tooltip_short = QString("%1 <div style='margin-left:10px;'>%2</div>")
					.arg(tr("Activity:")).arg(contactActivityName(AContactJid));
			AToolTips.insert(RTTO_USERACTIVITY, contactActivityText(AContactJid).isEmpty() ? tooltip_short : tooltip_full);
		}
	}
}

QIcon UserActivity::activityIcon(const QString &keyname) const
{
	return FActivityCatalog.value(keyname).icon;
}
QString UserActivity::activityName(const QString &keyname) const
{
	return FActivityCatalog.value(keyname).locname;
}
QIcon UserActivity::contactActivityIcon(const Jid &contactJid) const
{
	return FActivityCatalog.value(FActivityContact.value(contactJid.pBare()).keyname).icon;
}
QString UserActivity::contactActivityKey(const Jid &contactJid) const
{
	return FActivityContact.value(contactJid.pBare()).keyname;
}
QString UserActivity::contactActivityGeneralKey(const Jid &contactJid) const
{
	return FActivityCatalog.value(FActivityContact.value(contactJid.pBare()).keyname).general;
}
QString UserActivity::contactActivityName(const Jid &contactJid) const
{
	return FActivityCatalog.value(FActivityContact.value(contactJid.pBare()).keyname).locname;
}

QString UserActivity::contactActivityText(const Jid &contactJid) const
{
	QString text = FActivityContact.value(contactJid.pBare()).text;
	return text.replace("\n", "<br>");
}

void UserActivity::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserActivity)
