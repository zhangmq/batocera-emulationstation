#include <SystemData.h>
#include "guis/GuiCollectionSystemsOptions.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Window.h"
#include "GuiMenu.h"
#include "ApiSystem.h"
#include "GuiGamelistFilter.h"
#include "views/gamelist/IGameListView.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(Window* window) 
	: GuiSettings(window, _("GAME COLLECTION SETTINGS").c_str())
{
	initializeMenu();
}

void GuiCollectionSystemsOptions::initializeMenu()
{
	addGroup(_("COLLECTIONS TO DISPLAY"));

	// get collections
	addSystemsToMenu();

	auto groupNames = SystemData::getAllGroupNames();
	if (groupNames.size() > 0)
	{
		auto ungroupedSystems = std::make_shared<OptionListComponent<std::string>>(mWindow, _("GROUPED SYSTEMS"), true);
		for (auto groupName : groupNames)
		{
			std::string description;
			for (auto zz : SystemData::getGroupChildSystemNames(groupName))
			{
				if (!description.empty())
					description += ", ";

				description += zz;
			}

			ungroupedSystems->addEx(groupName, description, groupName, !Settings::getInstance()->getBool(groupName + ".ungroup"));
		}

		addWithLabel(_("GROUPED SYSTEMS"), ungroupedSystems);

		addSaveFunc([this, ungroupedSystems, groupNames]
		{
			std::vector<std::string> checkedItems = ungroupedSystems->getSelectedObjects();
			for (auto groupName : groupNames)
			{
				bool isGroupActive = std::find(checkedItems.cbegin(), checkedItems.cend(), groupName) != checkedItems.cend();
				if (Settings::getInstance()->setBool(groupName + ".ungroup", !isGroupActive))
					setVariable("reloadSystems", true);
			}
		});
	}

	addGroup(_("CREATE CUSTOM COLLECTION"));

	// add "Create New Custom Collection from Theme"
	std::vector<std::string> unusedFolders = CollectionSystemManager::get()->getUnusedSystemsFromTheme();
	if (unusedFolders.size() > 0)
	{
		addEntry(_("CREATE NEW CUSTOM COLLECTION FROM THEME").c_str(), true, [this, unusedFolders] 
	    {
			auto s = new GuiSettings(mWindow, _("SELECT THEME FOLDER").c_str());
		    std::shared_ptr< OptionListComponent<std::string> > folderThemes = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT THEME FOLDER"), true);

			// add Custom Systems
			for(auto it = unusedFolders.cbegin() ; it != unusedFolders.cend() ; it++)
			{
				ComponentListRow row;
				std::string name = *it;

				std::function<void()> createCollectionCall = [name, this, s] {
					createCollection(name);
				};
				row.makeAcceptInputHandler(createCollectionCall);

				auto themeFolder = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(name), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
				row.addElement(themeFolder, true);
				s->addRow(row);				
			}
			mWindow->pushGui(s);
		});
	}

	auto createCustomCollection = [this](const std::string& newVal) {
		std::string name = newVal;
		// we need to store the first Gui and remove it, as it'll be deleted by the actual Gui
		Window* window = mWindow;
		GuiComponent* topGui = window->peekGui();
		window->removeGui(topGui);
		createCollection(name);
	};
	addEntry(_("CREATE NEW EDITABLE COLLECTION").c_str(), true, [this, createCustomCollection] 
	{
		if (Settings::getInstance()->getBool("UseOSK"))
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("New Collection Name"), "", createCustomCollection, false));
		else
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("New Collection Name"), "", createCustomCollection, false));		
	});

	auto createDynCollection = [this](const std::string& newVal) { createFilterCollection(newVal); };

	addEntry(_("CREATE NEW DYNAMIC COLLECTION").c_str(), true, [this, createDynCollection]
	{
		if (Settings::getInstance()->getBool("UseOSK"))
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("New Collection Name"), "", createDynCollection, false));
		else
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("New Collection Name"), "", createDynCollection, false));
	});

	addGroup(_("OPTIONS"));

	std::shared_ptr<SwitchComponent> bundleCustomCollections = std::make_shared<SwitchComponent>(mWindow);
	bundleCustomCollections->setState(Settings::getInstance()->getBool("UseCustomCollectionsSystem"));
	addWithLabel(_("GROUP UNTHEMED CUSTOM COLLECTIONS"), bundleCustomCollections);
	addSaveFunc([this, bundleCustomCollections]
	{
		if (Settings::getInstance()->setBool("UseCustomCollectionsSystem", bundleCustomCollections->getState()))
			setVariable("reloadAll", true);
	});
	
	std::string sortMode = Settings::getInstance()->getString("SortSystems");

	auto sortType = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SORT COLLECTIONS AND SYSTEMS"), false);
	sortType->add(_("NO"), "", sortMode.empty());
	sortType->add(_("ALPHABETICALLY"), "alpha", sortMode == "alpha");

	if (SystemData::isManufacturerSupported())
		sortType->add(_("BY MANUFACTURER"), "manufacturer", sortMode == "manufacturer");

	if (!sortType->hasSelection())
		sortType->selectFirstItem();
	
	addWithLabel(_("SORT SYSTEMS"), sortType);
	addSaveFunc([this, sortType]
	{
		if (Settings::getInstance()->setString("SortSystems", sortType->getSelected()))
			setVariable("reloadAll", true);
	});

	/*


	std::shared_ptr<SwitchComponent> sortAllSystemsSwitch = std::make_shared<SwitchComponent>(mWindow);
	sortAllSystemsSwitch->setState(Settings::getInstance()->getBool("SortAllSystems"));
	addWithLabel(_("SORT CUSTOM COLLECTIONS AND SYSTEMS"), sortAllSystemsSwitch);
	addSaveFunc([this, sortAllSystemsSwitch]
	{
		if (Settings::getInstance()->setBool("SortAllSystems", sortAllSystemsSwitch->getState()))
			setVariable("reloadAll", true);
	});
	*/
	std::shared_ptr<SwitchComponent> toggleSystemNameInCollections = std::make_shared<SwitchComponent>(mWindow);
	toggleSystemNameInCollections->setState(Settings::getInstance()->getBool("CollectionShowSystemInfo"));
	addWithLabel(_("SHOW SYSTEM NAME IN COLLECTIONS"), toggleSystemNameInCollections);
	addSaveFunc([this, toggleSystemNameInCollections]
	{
		if (Settings::getInstance()->setBool("CollectionShowSystemInfo", toggleSystemNameInCollections->getState()))
			setVariable("reloadAll", true);
	});

#if defined(WIN32) && !defined(_DEBUG)		
	if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::GAMESETTINGS))
		addEntry(_("UPDATE GAMES LISTS"), false, [this] { GuiMenu::updateGameLists(mWindow); }); // Game List Update
#endif		

	if(CollectionSystemManager::get()->isEditing())
		addEntry((_("FINISH EDITING COLLECTION") + " : " + Utils::String::toUpper(CollectionSystemManager::get()->getEditingCollection())).c_str(), false, std::bind(&GuiCollectionSystemsOptions::exitEditMode, this));
	
	addSaveFunc([this]
	{
		std::string newAutoSettings = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
		std::string newCustomSettings = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());

		bool dirty = Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
		dirty |= Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);

		if (dirty)
			setVariable("reloadAll", true);
	});

	onFinalize([this]
	{
		if (getVariable("reloadSystems"))
		{
			Window* window = mWindow;
			window->renderSplashScreen(_("Loading..."));

			ViewController::get()->goToStart();
			delete ViewController::get();
			ViewController::init(window);
			CollectionSystemManager::deinit();
			CollectionSystemManager::init(window);
			SystemData::loadConfig(window);

			GuiComponent* gui;
			while ((gui = window->peekGui()) != NULL) 
			{
				window->removeGui(gui);
				if (gui != this)
					delete gui;
			}
			ViewController::get()->reloadAll(nullptr, false); // Avoid reloading themes a second time
			window->closeSplashScreen();

			window->pushGui(ViewController::get());
		}
		else if (getVariable("reloadAll"))
		{			
			Settings::getInstance()->saveFile();

			CollectionSystemManager::get()->loadEnabledListFromSettings();
			CollectionSystemManager::get()->updateSystemsList();
			ViewController::get()->goToStart();
			ViewController::get()->reloadAll(mWindow);
			mWindow->closeSplashScreen();
		}
	});
}

void GuiCollectionSystemsOptions::createCollection(std::string inName) 
{
	std::string name = CollectionSystemManager::get()->getValidNewCollectionName(inName);
	SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);
	customOptionList->add(name, name, true);

	std::string outAuto = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string outCustom = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());
	updateSettings(outAuto, outCustom);

	ViewController::get()->goToSystemView(newSys);

	Window* window = mWindow;
	CollectionSystemManager::get()->setEditMode(name);
	while(window->peekGui() && window->peekGui() != ViewController::get())
		delete window->peekGui();
}

void GuiCollectionSystemsOptions::createFilterCollection(std::string inName)
{
	std::string name = CollectionSystemManager::get()->getValidNewCollectionName(inName);
	if (name.empty())
		return;

	CollectionFilter cf;
	cf.create(name);
	
	SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);

	std::string setting = Settings::getInstance()->getString("CollectionSystemsCustom");
	setting = setting.empty() ? name : setting + "," + name;
	if (Settings::getInstance()->setString("CollectionSystemsCustom", setting))
		Settings::getInstance()->saveFile();
		
	CollectionSystemManager::get()->loadEnabledListFromSettings();
	CollectionSystemManager::get()->updateSystemsList();
	ViewController::get()->reloadAll(nullptr, false);
	ViewController::get()->goToStart();

	std::map<std::string, CollectionSystemData> customCollections = CollectionSystemManager::get()->getCustomCollectionSystems();
	auto customCollection = customCollections.find(name);
	if (customCollection == customCollections.cend())
		return;

	Window* window = mWindow;

	GuiComponent* topGui = window->peekGui();
	if (topGui != nullptr)
		window->removeGui(topGui);

	while (window->peekGui() && window->peekGui() != ViewController::get())
		delete window->peekGui();

	GuiGamelistFilter* ggf = new GuiGamelistFilter(window, customCollection->second.filteredIndex);
	
	ggf->onFinalize([name, newSys, window]()
	{
		window->renderSplashScreen();

		CollectionSystemManager::get()->reloadCollection(name, false);						
		ViewController::get()->goToGameList(newSys, true);

		window->closeSplashScreen();
	});
	
	window->pushGui(ggf);
}

void GuiCollectionSystemsOptions::exitEditMode()
{
	CollectionSystemManager::get()->exitEditMode();
	close();
}

GuiCollectionSystemsOptions::~GuiCollectionSystemsOptions()
{

}

void GuiCollectionSystemsOptions::addSystemsToMenu()
{

	std::map<std::string, CollectionSystemData> &autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();

	autoOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT COLLECTIONS"), true);

	bool hasGroup = false;

	auto arcadeGames = CollectionSystemManager::get()->getArcadeCollection()->getRootFolder()->getFilesRecursive(GAME);

	// add Auto Systems && preserve order
	for (auto systemDecl : CollectionSystemManager::getSystemDecls())
	{
		auto it = autoSystems.find(systemDecl.name);
		if (it == autoSystems.cend())
			continue;

        if (it->second.decl.displayIfEmpty)
            autoOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
        else
        {
			bool hasGames = false;

			if (it->second.isPopulated)
				hasGames = it->second.system->getRootFolder()->getChildren().size() > 0;
			else
			{
				for (auto file : arcadeGames)
				{
					if (file->getMetadata(MetaDataId::ArcadeSystemName) == systemDecl.themeFolder)
					{
						hasGames = true;
						break;
					}
				}
			}

			if (!hasGames)
				continue;

			if (!hasGroup)
			{
				autoOptionList->addGroup(_("ARCADE SYSTEMS"));
				hasGroup = true;
			}

            autoOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
        }
	}
	addWithLabel(_("AUTOMATIC GAME COLLECTIONS"), autoOptionList);

	std::map<std::string, CollectionSystemData> customSystems = CollectionSystemManager::get()->getCustomCollectionSystems();

	customOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SELECT COLLECTIONS"), true);

	customOptionList->addGroup(_("EDITABLE COLLECTIONS"));

	for (auto sys : customSystems)
		if (sys.second.filteredIndex == nullptr)
			customOptionList->add(sys.second.decl.longName, sys.second.decl.name, sys.second.isEnabled);

	customOptionList->addGroup(_("DYNAMIC COLLECTIONS"));

	for (auto sys : customSystems)
		if (sys.second.filteredIndex != nullptr)
			customOptionList->add(sys.second.decl.longName, sys.second.decl.name, sys.second.isEnabled);

	addWithLabel(_("CUSTOM GAME COLLECTIONS"), customOptionList);
}

void GuiCollectionSystemsOptions::updateSettings(std::string newAutoSettings, std::string newCustomSettings)
{
	bool dirty = Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
	dirty |= Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);

	if (dirty)
	{
		Settings::getInstance()->saveFile();
		CollectionSystemManager::get()->loadEnabledListFromSettings();
		CollectionSystemManager::get()->updateSystemsList();		
		ViewController::get()->reloadAll();
		ViewController::get()->goToStart();
	}
}