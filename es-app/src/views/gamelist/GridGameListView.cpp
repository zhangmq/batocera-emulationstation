#include "views/gamelist/GridGameListView.h"

#include "animations/LambdaAnimation.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "LocaleES.h"
#include "Window.h"
#include "SystemConf.h"
#include "guis/GuiGamelistOptions.h"

GridGameListView::GridGameListView(Window* window, FolderData* root, const std::shared_ptr<ThemeData>& theme, std::string themeName, Vector2f gridSize) :
	ISimpleGameListView(window, root),
	mGrid(window),
	mDetails(this, &mGrid, mWindow, DetailedContainer::GridView)
{
	setTag("grid");

	const float padding = 0.01f;

	mGrid.setGridSizeOverride(gridSize);
	mGrid.setPosition(mSize.x() * 0.1f, mSize.y() * 0.1f);
	mGrid.setDefaultZIndex(20);
	mGrid.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
	addChild(&mGrid);
	
	if (!themeName.empty())
		setThemeName(themeName);

	setTheme(theme);

	populateList(mRoot->getChildrenListToDisplay());
	updateInfoPanel();
}

void GridGameListView::onShow()
{
	ISimpleGameListView::onShow();
	updateInfoPanel();
}

void GridGameListView::setThemeName(std::string name)
{
	ISimpleGameListView::setThemeName(name);
	mGrid.setThemeName(getName());
}

FileData* GridGameListView::getCursor()
{
	if (mGrid.size() == 0)
		return nullptr;

	return mGrid.getSelected();
}

void GridGameListView::setCursor(FileData* file)
{
	if (!mGrid.setCursor(file) && file->getParent() != nullptr && !file->isPlaceHolder())
	{
		auto children = mRoot->getChildrenListToDisplay();

		auto gameIter = std::find(children.cbegin(), children.cend(), file);
		if (gameIter == children.cend())
		{
			children = file->getParent()->getChildrenListToDisplay();

			// update our cursor stack in case our cursor just got set to some folder we weren't in before
			if (mCursorStack.empty() || mCursorStack.top() != file->getParent())
			{
				std::stack<FileData*> tmp;
				FileData* ptr = file->getParent();
				while (ptr && ptr != mRoot)
				{
					tmp.push(ptr);
					ptr = ptr->getParent();
				}

				// flip the stack and put it in mCursorStack
				mCursorStack = std::stack<FileData*>();
				while (!tmp.empty())
				{
					mCursorStack.push(tmp.top());
					tmp.pop();
				}
			}
		}

		populateList(children);
		mGrid.setCursor(file);
	}
}

std::string GridGameListView::getQuickSystemSelectRightButton()
{
#ifdef _ENABLEEMUELEC
	return "rightshoulder"; //rightshoulder
#else
	return "r2"; //rightshoulder
#endif
}

std::string GridGameListView::getQuickSystemSelectLeftButton()
{
#ifdef _ENABLEEMUELEC
	return "leftshoulder"; //leftshoulder
#else
	return "l2"; //leftshoulder
#endif
}

bool GridGameListView::input(InputConfig* config, Input input)
{
	if (!UIModeController::getInstance()->isUIModeKid() && config->isMappedTo("select", input) && input.value)
	{
		Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();
		mWindow->pushGui(new GuiGamelistOptions(mWindow, this->mRoot->getSystem(), true));
		return true;

		// Ctrl-R to reload a view when debugging
	}

	if(config->isMappedLike("left", input) || config->isMappedLike("right", input))
		return GuiComponent::input(config, input);

	return ISimpleGameListView::input(config, input);
}

const std::string GridGameListView::getImagePath(FileData* file)
{
	ImageSource src = mGrid.getImageSource();

	if (src == ImageSource::IMAGE)
		return file->getImagePath();
	else if (src == ImageSource::MARQUEE || src == ImageSource::MARQUEEORTEXT)
		return file->getMarqueePath();

	return file->getThumbnailPath();
}

const bool GridGameListView::isVirtualFolder(FileData* file)
{
	return file->getType() == FOLDER && ((FolderData*)file)->isVirtualFolderDisplay();
}

void GridGameListView::populateList(const std::vector<FileData*>& files)
{
	SystemData* system = mCursorStack.size() && mRoot->getSystem()->isGroupSystem() ? mCursorStack.top()->getSystem() : mRoot->getSystem();

	auto groupTheme = system->getTheme();
	if (groupTheme)
	{
		const ThemeData::ThemeElement* logoElem = groupTheme->getElement(getName(), "logo", "image");
		if (logoElem && logoElem->has("path") && Utils::FileSystem::exists(logoElem->get<std::string>("path")))
			mHeaderImage.setImage(logoElem->get<std::string>("path"), false, mHeaderImage.getMaxSizeInfo());
	}

	mHeaderText.setText(system->getFullName());

	mGrid.resetLastCursor();
	mGrid.clear(); 
	mGrid.resetLastCursor();

	if (files.size() > 0)
	{
		bool showParentFolder = Settings::getInstance()->getBool("ShowParentFolder");
		auto spf = Settings::getInstance()->getString(mRoot->getSystem()->getName() + ".ShowParentFolder");
		if (spf == "1") showParentFolder = true;
		else if (spf == "0") showParentFolder = false;

		if (mCursorStack.size())
		{
			auto top = mCursorStack.top();

			std::string imagePath;
			bool displayAsVirtualFolder = true;

			// Find logo image from original system
			if (mCursorStack.size() == 1 && top->getSystem()->isGroupChildSystem())
			{
				std::string startPath = top->getSystem()->getStartPath();
				
				auto parent = top->getSystem()->getParentGroupSystem();
				
				auto theme = parent->getTheme();
				if (theme)
				{
					const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
					if (logoElem && logoElem->has("path"))
						imagePath = logoElem->get<std::string>("path");
				}

				if (imagePath.empty())
				{
					for (auto child : parent->getRootFolder()->getChildren())
					{
						if (child->getPath() == startPath)
						{
							if (child->getType() == FOLDER)
								displayAsVirtualFolder = ((FolderData*)child)->isVirtualFolderDisplayEnabled();

							imagePath = child->getMetadata(MetaDataId::Image);
							break;
						}
					}
				}				
			}

			if (showParentFolder)
			{
				FileData* placeholder = new FileData(PLACEHOLDER, "..", this->mRoot->getSystem());
				mGrid.add(". .", imagePath, "", "", false, true, displayAsVirtualFolder && !imagePath.empty(), placeholder);
			}
		}

		std::string systemName = mRoot->getSystem()->getName();

		bool favoritesFirst = Settings::getInstance()->getBool("FavoritesFirst");

		auto fav = Settings::getInstance()->getString(mRoot->getSystem()->getName() + ".FavoritesFirst");
		if (fav == "1") favoritesFirst = true;
		else if (fav == "0") favoritesFirst = false;

		bool showFavoriteIcon = (systemName != "favorites" && systemName != "recent");
		if (!showFavoriteIcon)
			favoritesFirst = false;

		if (favoritesFirst)
		{
			for (auto file : files)
			{
				if (file->getFavorite() && showFavoriteIcon)
					mGrid.add(_U("\uF006 ") + file->getName(), getImagePath(file), file->getVideoPath(), file->getMarqueePath(), true, file->getType() != GAME, isVirtualFolder(file), file);
			}
		}

		for (auto file : files)
		{
			if (file->getFavorite())
			{
				if (favoritesFirst)
					continue;

				if (showFavoriteIcon)
				{
					mGrid.add(_U("\uF006 ") + file->getName(), getImagePath(file), file->getVideoPath(), file->getMarqueePath(), true, file->getType() != GAME, isVirtualFolder(file), file);
					continue;
				}
			}

			if (file->getType() == FOLDER && Utils::FileSystem::exists(getImagePath(file)))
				mGrid.add(_U("\uF07C ") + file->getName(), getImagePath(file), file->getVideoPath(), file->getMarqueePath(), file->getFavorite(), file->getType() != GAME, isVirtualFolder(file), file);
			else
				mGrid.add(file->getName(), getImagePath(file), file->getVideoPath(), file->getMarqueePath(), file->getFavorite(), file->getType() != GAME, isVirtualFolder(file), file);
		}

		// if we have the ".." PLACEHOLDER, then select the first game instead of the placeholder
		if (showParentFolder && mCursorStack.size() && mGrid.size() > 1 && mGrid.getCursorIndex() == 0)
			mGrid.setCursorIndex(1);
	}
	else
		addPlaceholder();

	updateFolderPath();
}

void GridGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);

	using namespace ThemeFlags;

	mGrid.applyTheme(theme, getName(), "gamegrid", ALL);
	mDetails.onThemeChanged(theme);
	
	sortChildren();
	updateInfoPanel();
}

void GridGameListView::updateInfoPanel()
{
	if (mRoot->getSystem()->isCollection())
		updateHelpPrompts();

	FileData* file = (mGrid.size() == 0 || mGrid.isScrolling()) ? NULL : mGrid.getSelected();
	bool isClearing = mGrid.getObjects().size() == 0 && mGrid.getCursorIndex() == 0 && mGrid.getScrollingVelocity() == 0;
	mDetails.updateControls(file, isClearing);	
}

void GridGameListView::addPlaceholder()
{
	// empty grid - add a placeholder
	FileData* placeholder = new FileData(PLACEHOLDER, "<" + _("No Entries Found") + ">", mRoot->getSystem());
	mGrid.add(placeholder->getName(), "", "", "", false, false, false, placeholder);
}

void GridGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void GridGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem

	FolderData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < (int)siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mGrid.remove(game);
	if(mGrid.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
}

void GridGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if (change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

std::vector<HelpPrompt> GridGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("lr", _("SYSTEM"))); // batocera

	prompts.push_back(HelpPrompt("up/down/left/right", _("CHOOSE"))); // batocera
	prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", _("OPTIONS"))); // batocera

	FileData* cursor = getCursor();
	if (cursor != nullptr && cursor->isNetplaySupported())
		prompts.push_back(HelpPrompt("x", _("NETPLAY"))); // batocera
	else
		prompts.push_back(HelpPrompt("x", _("RANDOM"))); // batocera

	if(mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();

		if (Utils::String::toLower(prompt) == "favorites")
			prompts.push_back(HelpPrompt("y", _("Favorites")));
		else
			prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}

void GridGameListView::setCursorIndex(int cursor)
{
	mGrid.setCursorIndex(cursor);
}

int GridGameListView::getCursorIndex()
{
	return mGrid.getCursorIndex();
}

std::vector<FileData*> GridGameListView::getFileDataEntries()
{
	return mGrid.getObjects();
}
