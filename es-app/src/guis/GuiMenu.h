#pragma once
#ifndef ES_APP_GUIS_GUI_MENU_H
#define ES_APP_GUIS_GUI_MENU_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include "guis/GuiSettings.h"
#include "components/OptionListComponent.h"
#include <SystemData.h>

class StrInputConfig
{
 public:
  StrInputConfig(std::string ideviceName, std::string ideviceGUIDString) {
    deviceName = ideviceName;
    deviceGUIDString = ideviceGUIDString;
  }

  std::string deviceName;
  std::string deviceGUIDString;
};

struct DecorationSetInfo
{
	DecorationSetInfo() {}
	DecorationSetInfo(const std::string _name, const std::string _path, const std::string _imageUrl)
	{
		name = _name; path = _path; imageUrl = _imageUrl;
	}

	std::string name;
	std::string path;
	std::string imageUrl;
};

class GuiMenu : public GuiComponent
{
public:
	GuiMenu(Window* window, bool animate = true);
	~GuiMenu();

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
	std::vector<HelpPrompt> getHelpPrompts() override;	
	static void openQuitMenu_batocera_static(Window *window, bool forceWin32Menu=false); // batocera

	static void popSystemConfigurationGui(Window* mWindow, SystemData *systemData);
	static void popGameConfigurationGui(Window* mWindow, FileData* fileData);

	static void openThemeConfiguration(Window* mWindow, GuiComponent* s, std::shared_ptr<OptionListComponent<std::string>> theme_set, const std::string systemTheme = "");

	static void updateGameLists(Window* window, bool confirm = true);

private:
	void addEntry(std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName = "");
	void addVersionInfo();
	void openCollectionSystemSettings();
	void openConfigInput();	
	void openScraperSettings();
	void openScreensaverOptions();
	void openSlideshowScreensaverOptions();
	void openVideoScreensaverOptions();
	void openSoundSettings();
	void openUISettings();
	void openUpdatesSettings();

#ifdef _ENABLEEMUELEC
	void openEmuELECSettings(); /* < emuelec */
#endif
	// batocera	
	void openSystemSettings_batocera();
	void openGamesSettings_batocera();
	void openControllersSettings_batocera();		
	void openNetworkSettings_batocera(bool selectWifiEnable = false);
	void openScraperSettings_batocera();
	void openQuitMenu_batocera();
	void openSystemInformations_batocera();
	void openDeveloperSettings();
	void openNetplaySettings(); 
	void openRetroachievementsSettings();
	void openMissingBiosSettings();
	void openFormatDriveSettings();

	static void reloadAllGames(Window* window, bool deleteCurrentGui = false);

	// windows
	void openEmulatorSettings();
	void openSystemEmulatorSettings(SystemData* system);

	static void openWifiSettings(Window* win, std::string title, std::string data, const std::function<void(std::string)>& onsave);

	void createInputTextRow(GuiSettings * gui, std::string title, const char* settingsID, bool password, bool storeInSettings=false, const std::function<void(Window*, std::string/*title*/, std::string /*value*/, const std::function<void(std::string)>& onsave)>& customEditor = nullptr);
	MenuComponent mMenu;
	TextComponent mVersion;

	static std::shared_ptr<OptionListComponent<std::string>> createRatioOptionList(Window *window, std::string configname);
	static std::shared_ptr<OptionListComponent<std::string>> createVideoResolutionModeOptionList(Window *window, std::string configname);
	static void popSpecificConfigurationGui(Window* mWindow, std::string title, std::string configName, SystemData *systemData, FileData* fileData, bool selectCoreLine = false);

	static void openLatencyReductionConfiguration(Window* mWindow, std::string configName);

	std::vector<StrInputConfig*> mLoadedInput; // used to keep information about loaded devices in case there are unpluged between device window load and save
	void clearLoadedInput();
	static void createDecorationItemTemplate(Window* window, std::vector<DecorationSetInfo> sets, std::string data, ComponentListRow& row);

public:
	static std::vector<DecorationSetInfo> getDecorationsSets(SystemData* system = nullptr);
};

#endif // ES_APP_GUIS_GUI_MENU_H
