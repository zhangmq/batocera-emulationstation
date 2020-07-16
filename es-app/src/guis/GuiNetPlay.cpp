#include "GuiNetPlay.h"
#include "Window.h"
#include <string>
#include "Log.h"
#include "Settings.h"
#include "SystemConf.h"
#include "LocaleES.h"
#include "HttpReq.h"
#include "AsyncHandle.h"
#include "guis/GuiMsgBox.h"
#include "SystemData.h"
#include "FileData.h"
#include "ThemeData.h"
#include "components/MenuComponent.h"
#include "components/ButtonComponent.h"
#include "views/ViewController.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/pointer.h>

// http://lobby.libretro.com/list/
// Core list :
// FCEUmm, FB Alpha, MAME 2000, MAME 2003, MAME 2010

GuiNetPlay::GuiNetPlay(Window* window) 
	: GuiComponent(window), 
	mBusyAnim(window),
	mBackground(window, ":/frame.png"),
	mGrid(window, Vector2i(1, 3)),
	mList(nullptr)
{	
	addChild(&mBackground);
	addChild(&mGrid);

	// Form background
	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	// Title

	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));

	mTitle = std::make_shared<TextComponent>(mWindow, _("CONNECT TO NETPLAY"), theme->Title.font, theme->Title.color, ALIGN_CENTER); // batocera
	mSubtitle = std::make_shared<TextComponent>(mWindow, _("Select the game you want to join"), theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);
	// Lobby Entries List
	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true, true);

	// Buttons
	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("REFRESH"), _("REFRESH"), [this] { startRequest(); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CLOSE"), _("CLOSE"), [this] { delete this; }));

	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool 
	{
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			mGrid.moveCursor(Vector2i(0, 1));
			return true;
		}
		return false;
	});

	float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));

	// Position & Size
	if (Renderer::isSmallScreen())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(width, Renderer::getScreenHeight() * 0.85f);

	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
	
	// Loading
    mBusyAnim.setSize(Vector2f(Renderer::getScreenWidth(), Renderer::getScreenHeight()));
	mBusyAnim.setText(_("PLEASE WAIT"));
	startRequest();
}

void GuiNetPlay::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mGrid.setSize(mSize);
	
	const float titleHeight = mTitle->getFont()->getLetterHeight();
	const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
	const float titleSubtitleSpacing = mSize.y() * 0.03f;

	mGrid.setRowHeightPerc(0, (titleHeight + titleSubtitleSpacing + subtitleHeight + TITLE_VERT_PADDING) / mSize.y());
	mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y() / mSize.y());

	mHeaderGrid->setRowHeightPerc(1, titleHeight / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(2, titleSubtitleSpacing / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(3, subtitleHeight / mHeaderGrid->getSize().y());
}

void GuiNetPlay::startRequest()
{
	if (mLobbyRequest != nullptr)
		return;

	mList->clear();

	std::string netPlayLobby = SystemConf::getInstance()->get("global.netplay.lobby");
	if (netPlayLobby.empty())
		netPlayLobby = "http://lobby.libretro.com/list/";

	mLobbyRequest = std::unique_ptr<HttpReq>(new HttpReq(netPlayLobby));
}

void GuiNetPlay::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mLobbyRequest)
	{
		auto status = mLobbyRequest->status();
		if (status != HttpReq::REQ_IN_PROGRESS)
		{			
			if (status == HttpReq::REQ_SUCCESS)
				populateFromJson(mLobbyRequest->getContent());
			else
			  mWindow->pushGui(new GuiMsgBox(mWindow, _("FAILED") + std::string(" : ") + mLobbyRequest->getErrorMsg()));

			mLobbyRequest.reset();
		}

		mBusyAnim.update(deltaTime);
	}
}

bool GuiNetPlay::input(InputConfig* config, Input input)
{
	if (config->isMappedTo(BUTTON_BACK, input) && input.value != 0)
	{
		delete this;
		return true;
	}

	return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiNetPlay::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	return prompts;
}


FileData* GuiNetPlay::getFileData(std::string gameInfo, bool crc)
{
	for (auto sys : SystemData::sSystemVector)
	{
		if (!sys->isNetplaySupported())
			continue;

		for (auto file : sys->getRootFolder()->getFilesRecursive(GAME))
		{
			if (crc)
			{
				if (file->getMetadata(MetaDataId::Crc32) == gameInfo)
					return file;

				continue;
			}
			
			std::string stem = Utils::FileSystem::getStem(file->getPath());
			if (stem == gameInfo)
				return file;			
		}
	}

	return nullptr;
}

class NetPlayLobbyListEntry : public ComponentGrid
{
public:
	NetPlayLobbyListEntry(Window* window, LobbyAppEntry& entry) :
		ComponentGrid(window, Vector2i(4, 3))
	{
		mEntry = entry;

		auto theme = ThemeData::getMenuTheme();

		mImage = std::make_shared<ImageComponent>(mWindow);

		if (entry.fileData == nullptr)
			mImage->setImage(":/cartridge.svg");
		else
			mImage->setImage(entry.fileData->getImagePath());		

		mImage->setRoundCorners(0.25);

		std::string name = entry.fileData == nullptr ? entry.game_name : entry.fileData->getMetadata(MetaDataId::Name) + " [" + entry.fileData->getSystemName() + "]";

		mText = std::make_shared<TextComponent>(mWindow, name.c_str(), theme->Text.font, theme->Text.color);
		mText->setLineSpacing(1.5);
		mText->setVerticalAlignment(ALIGN_TOP);

		std::string userInfo = _U("\uf007  ") + entry.username + _U("  \uf0AC  ") + entry.country + _U("  \uf0E8  ") + entry.ip+ _U("  \uf108  ") + entry.frontend;

		mSubstring = std::make_shared<TextComponent>(mWindow, userInfo.c_str(), theme->TextSmall.font, theme->Text.color);
		mSubstring->setOpacity(192);

		std::string subInfo = _U("\uf11B  ") + entry.core_name + " (" + entry.retroarch_version + ")";

		if (entry.fileData != nullptr && !entry.coreExists)
			subInfo = subInfo + " " + _U("\uf071  ") + _("UNAVAILABLE CORE");
		
		mDetails = std::make_shared<TextComponent>(mWindow, subInfo.c_str(), theme->TextSmall.font, theme->Text.color);
		mDetails->setOpacity(192);
		
		if (entry.has_password || entry.has_spectate_password)
		{					
			if (!entry.has_spectate_password)
				mLockInfo = std::make_shared<TextComponent>(mWindow, std::string(_U("\uf06E")).c_str(), Font::get(FONT_SIZE_MEDIUM, FONT_PATH_REGULAR), theme->Text.color);
			else
				mLockInfo = std::make_shared<TextComponent>(mWindow, std::string(_U("\uf023")).c_str(), Font::get(FONT_SIZE_MEDIUM, FONT_PATH_REGULAR), theme->Text.color);

			mLockInfo->setOpacity(192);
			mLockInfo->setLineSpacing(1);
		}

		setEntry(mImage, Vector2i(0, 0), false, true, Vector2i(1, 3));
		setEntry(mText, Vector2i(2, 0), false, true);
		setEntry(mSubstring, Vector2i(2, 1), false, true);
		setEntry(mDetails, Vector2i(2, 2), false, true);
		
		if (mLockInfo != nullptr)
			setEntry(mLockInfo, Vector2i(3, 0), false, true, Vector2i(1, 3));
		
		float h = mText->getSize().y() * 1.1f + mSubstring->getSize().y() + mDetails->getSize().y();

		float sw = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));

		mImage->setMaxSize(h * 1.15f, h * 0.85f);
		mImage->setPadding(Vector4f(8, 8, 8, 8));

		if (entry.fileData == nullptr)
		{
			mImage->setColorShift(0x80808080);
			mImage->setOpacity(120);
			mText->setOpacity(150);
			mSubstring->setOpacity(120);
			mDetails->setOpacity(120);
		}

		setColWidthPerc(0, h * 1.15f / sw, false);
		setColWidthPerc(1, 0.015f, false);

		if (mLockInfo != nullptr)
			setColWidthPerc(3, 0.055f, false); // cf FONT_SIZE_LARGE
		else 
			setColWidthPerc(3, 0.002f, false);

		setRowHeightPerc(0, mText->getSize().y() / h, false);
		setRowHeightPerc(1, mSubstring->getSize().y() / h, false);
		setRowHeightPerc(2, mDetails->getSize().y() / h, false);

		setSize(Vector2f(0, h));
	}

	LobbyAppEntry& getEntry() {
		return mEntry;
	}

	virtual void setColor(unsigned int color)
	{
		mText->setColor(color);
		mSubstring->setColor(color);
		mDetails->setColor(color);

		if (mLockInfo != nullptr)
			mLockInfo->setColor(color);
	}

private:
	std::shared_ptr<ImageComponent>  mImage;

	std::shared_ptr<TextComponent>  mText;
	std::shared_ptr<TextComponent>  mSubstring;
	std::shared_ptr<TextComponent>  mDetails;
	std::shared_ptr<TextComponent>	mLockInfo;

	LobbyAppEntry mEntry;	
};


bool GuiNetPlay::populateFromJson(const std::string json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (doc.HasParseError())
	{
		std::string err = std::string("GuiNetPlay - Error parsing JSON. \n\t");		
		LOG(LogError) << err;
		return false;
	}

	std::vector<LobbyAppEntry> entries;

	for (auto& item : doc.GetArray())
	{
		if (!item.HasMember("fields"))
			continue;

		const rapidjson::Value& fields = item["fields"];
		if (fields.HasMember("has_password") && fields["has_password"].IsBool() && fields["has_password"].GetBool())
			continue;

		FileData* file = nullptr;

		if (fields.HasMember("game_crc") && fields["game_crc"].IsString() && fields["game_crc"] != "00000000")
			file = getFileData(Utils::String::toUpper(fields["game_crc"].GetString()), true);

		if (file == nullptr && fields.HasMember("game_name") && fields["game_name"].IsString())
			file = getFileData(fields["game_name"].GetString(), false);
		
		LobbyAppEntry game;
		game.fileData = file;
	
		if (fields.HasMember("username") && fields["username"].IsString())
			game.username = fields["username"].GetString();

		if (fields.HasMember("game_crc") && fields["game_crc"].IsString())
			game.game_crc = fields["game_crc"].GetString();

		if (fields.HasMember("mitm_ip") && fields["mitm_ip"].IsString())
			game.mitm_ip = fields["mitm_ip"].GetString();

		if (fields.HasMember("subsystem_name") && fields["subsystem_name"].IsString())
			game.subsystem_name = fields["subsystem_name"].GetString();

		if (fields.HasMember("frontend") && fields["frontend"].IsString())
			game.frontend = fields["frontend"].GetString();

		if (fields.HasMember("created") && fields["created"].IsString())
			game.created = fields["created"].GetString();

		if (fields.HasMember("ip") && fields["ip"].IsString())
			game.ip = fields["ip"].GetString();

		if (fields.HasMember("updated") && fields["updated"].IsString())
			game.updated = fields["updated"].GetString();

		if (fields.HasMember("country") && fields["country"].IsString())
			game.country = fields["country"].GetString();

		if (fields.HasMember("host_method") && fields["host_method"].IsInt())
			game.host_method = fields["host_method"].GetInt();

		if (fields.HasMember("has_password") && fields["has_password"].IsBool())
			game.has_password = fields["has_password"].GetBool();

		if (fields.HasMember("game_name") && fields["game_name"].IsString())
			game.game_name = fields["game_name"].GetString();

		if (fields.HasMember("has_spectate_password") && fields["has_spectate_password"].IsBool())
			game.has_spectate_password = fields["has_spectate_password"].GetBool();		

		if (fields.HasMember("core_name") && fields["core_name"].IsString())
			game.core_name = fields["core_name"].GetString();

		if (fields.HasMember("mitm_port") && fields["mitm_port"].IsInt())
			game.mitm_port = fields["mitm_port"].GetInt();

		if (fields.HasMember("fixed") && fields["fixed"].IsBool())
			game.fixed = fields["fixed"].GetBool();

		if (fields.HasMember("retroarch_version") && fields["retroarch_version"].IsString())
			game.retroarch_version = fields["retroarch_version"].GetString();

		if (fields.HasMember("port") && fields["port"].IsInt())
			game.port = fields["port"].GetInt();

		game.coreExists = coreExists(file, game.core_name);

		entries.push_back(game);
	}	

	auto theme = ThemeData::getMenuTheme();

	bool groupAvailable = false;

	for (auto game : entries)
	{
		if (game.fileData == nullptr)
			continue;

		if (!groupAvailable)
		{			
			mList->addGroup(_("AVAILABLE GAMES"), true);
			groupAvailable = true;
		}

		ComponentListRow row;
		row.addElement(std::make_shared<NetPlayLobbyListEntry>(mWindow, game), true);

		if (game.fileData != nullptr)
			row.makeAcceptInputHandler([this, game] { launchGame(game); });

		mList->addRow(row);
	}

	bool groupUnavailable = false;

	for (auto game : entries)
	{
		if (game.fileData != nullptr)
			continue;

		if (!groupUnavailable)
		{
			mList->addGroup(_("UNAVAILABLE GAMES"), true);
			groupUnavailable = true;
		}

		ComponentListRow row;
		row.addElement(std::make_shared<NetPlayLobbyListEntry>(mWindow, game), true);
		mList->addRow(row);
	}

	if (mList->size() == 0)
	{
		ComponentListRow row;
		auto empty = std::make_shared<TextComponent>(mWindow);
		empty->setText(_("NO GAME FOUND"));
		row.addElement(empty, true);
		mList->addRow(row);

		mGrid.moveCursor(Vector2i(0, 1));
	}
	else
		mList->setCursorIndex(0, true);

	return true;
}

void GuiNetPlay::render(const Transform4x4f &parentTrans) 
{
	GuiComponent::render(parentTrans);

	if (mLobbyRequest)
		mBusyAnim.render(parentTrans);
}

static std::map<std::string, std::string> coreList =
{
#if WIN32 || _ENABLEEMUELEC
	{ "2048", "2048"},
	{ "81", "81"},
	{ "Atari800", "atari800"},
	{ "blueMSX", "bluemsx"},
	{ "Cannonball", "cannonball"},
	{ "Caprice32", "cap32"},
	{ "CrocoDS", "crocods"},
	{ "Daphne", "daphne"},
	{ "Dinothawr", "dinothawr"},
	{ "DOSBox-SVN", "dosbox_svn"},
	{ "EasyRPG Player", "easyrpg"},
	{ "FB Alpha 2012 Neo Geo", "fbalpha2012_neogeo" },
	{ "FB Alpha 2012", "fbalpha2012" },
	{ "FB Alpha", "fbalpha" },
	{ "FinalBurn Neo", "fbneo"},
	{ "FCEUmm", "fceumm"},
	{ "Flycast", "flycast"},
	{ "FreeIntv", "freeintv"},
	{ "Fuse", "fuse"},
	{ "Gambatte", "gambatte"},
	{ "Gearboy", "gearboy"},
	{ "Gearsystem", "gearsystem"},
	{ "Genesis Plus GX", "genesis_plus_gx"},
	{ "Game Music Emu", "gme"},
	{ "gpSP", "gpsp"},
	{ "GW", "gw"},
	{ "Handy", "handy"},
	{ "Hatari", "hatari"},
	{ "MAME 2003-Plus", "mame2003_plus"},
	{ "MAME 2010 (0.139)", "mame2010"},
	{ "MAME 2016 (0.174)", "mame2016"},
	{ "Beetle Lynx", "mednafen_lynx"},
	{ "Beetle NeoPop", "mednafen_ngp"},
	{ "Beetle PCE Fast", "mednafen_pce_fast"},
	{ "Beetle PC-FX", "mednafen_pcfx"},
	{ "Beetle SuperGrafx", "mednafen_supergrafx"},
	{ "Beetle VB", "mednafen_vb"},
	{ "Beetle WonderSwan", "mednafen_wswan"},
	{ "Mesen-S", "mesen-s"},
	{ "mGBA", "mgba"},
	{ "Mr.Boom", "mrboom"},
	{ "Mupen64Plus-Next", "mupen64plus_next"},
	{ "Neko Project II", "nekop2"},
	{ "NeoCD", "neocd"},
	{ "NestopiaCV", "nestopiaCV"},
	{ "Nestopia", "nestopia"},
	{ "Neko Project II", "np2kai"},
	{ "NXEngine", "nxengine"},
	{ "O2EM", "o2em"},
	{ "Opera", "opera"},
	{ "ParaLLEl N64", "parallel_n64"},
	{ "PCSX-ReARMed", "pcsx_rearmed"},
	{ "PicoDrive", "picodrive"},
	{ "PokeMini", "pokemini"},
	{ "PPSSPP", "ppsspp"},
	{ "PrBoom", "prboom"},
	{ "ProSystem", "prosystem"},
	{ "PUAE", "puae"},
	{ "PX68k", "px68k"},
	{ "QUASI88", "quasi88"},
	{ "QuickNES", "quicknes"},
	{ "REminiscence", "reminiscence"},
	{ "SameBoy", "sameboy"},
	{ "ScummVM", "scummvm"},
	{ "Snes9x 2002", "snes9x2002"},
	{ "Snes9x 2005 Plus", "snes9x2005_plus"},
	{ "Snes9x 2010", "snes9x2010"},
	{ "Snes9x", "snes9x"},
	{ "Stella", "stella"},
	{ "TGB Dual", "tgbdual"},
	{ "TyrQuake", "tyrquake"},
	{ "UAE4ARM", "uae4arm"},
	{ "uzem", "uzem"},
	{ "VBA-M", "vbam"},
	{ "VBA Next", "vba_next"},
	{ "vecx", "vecx"},
	{ "VICE", "vice_x128"},
	{ "VICE", "vice_x64"},
	{ "VICE", "vice_xplus4"},
	{ "VICE", "vice_xvic"},
	{ "Virtual Jaguar", "virtualjaguar"},
	{ "x1", "x1"},
	{ "XRick", "xrick"},
	{ "YabaSanshiro", "yabasanshiro"},
	{ "Yabause", "yabause"},
#else
	{ "Beetle NeoPop", "mednafen_ngp" },
	{ "Beetle PCE Fast", "pce" },
	{ "Beetle SuperGrafx", "mednafen_supergrafx" },
	{ "Beetle VB", "vb" },
	{ "Beetle WonderSwan", "mednafen_wswan" },
	{ "DOSBox-SVN", "dosbox" },
	{ "EightyOne", "81" },
	{ "FB Alpha 2012 Neo Geo", "fbalpha2012_neogeo" },
	{ "FB Alpha 2012", "fbalpha2012" },
	{ "FB Alpha", "fbalpha" },
	{ "FinalBurn Neo", "fbneo" },
	{ "Game & Watch", "gw" },
	{ "Genesis Plus GX", "genesisplusgx" },
	{ "MAME 2000", "imame4all" },
	{ "MAME 2003 (0.78)", "mame2003" },
	{ "MAME 2003-Plus", "mame078plus" },
	{ "MAME 2010", "mame0139" },
	{ "MAME 2014", "mame2014" },
	{ "PCSX-ReARMed", "pcsx_rearmed" },
	{ "Snes9x 2002", "pocketsnes" },
	{ "Snes9x 2010", "snes9x_next" },
	{ "TGB Dual", "tgbdual",},
	{ "VICE x64", "vice" }
#endif
};

void GuiNetPlay::launchGame(LobbyAppEntry entry)
{
	LaunchGameOptions options;
	options.netPlayMode = CLIENT;

	if (entry.host_method == 3)
	{
		options.ip = entry.mitm_ip;
		options.port = entry.mitm_port;
	}
	else
	{
		options.ip = entry.ip;
		options.port = entry.port;
	}
	
	auto coreInfo = coreList.find(entry.core_name);
	if (coreInfo != coreList.cend())
		options.core = coreInfo->second;
	else 	
		options.core = Utils::String::toLower(Utils::String::replace(entry.core_name, " ", "_"));
	
	ViewController::get()->launch(entry.fileData, options);
	delete this;
}

bool GuiNetPlay::coreExists(FileData* file, std::string core_name)
{
	if (file == nullptr)
		return false;

	if (core_name.empty())
		return false;

	std::string coreName;

	auto coreInfo = coreList.find(core_name);
	if (coreInfo != coreList.cend())
		coreName = coreInfo->second;
	else
		coreName = Utils::String::toLower(Utils::String::replace(core_name, " ", "_"));

	for (auto& emul : file->getSourceFileData()->getSystem()->getEmulators())
		for (auto& core : emul.cores)
			if (core.name == coreName)
				return true;

	return false;
}
