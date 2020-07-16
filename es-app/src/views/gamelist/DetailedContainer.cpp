#include "DetailedContainer.h"

#include "animations/LambdaAnimation.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "SystemData.h"
#include "LocaleES.h"
#include "LangParser.h"

#ifdef _RPI_
#include "Settings.h"
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"

DetailedContainer::DetailedContainer(ISimpleGameListView* parent, GuiComponent* list, Window* window, DetailedContainerType viewType) :
	mParent(parent), mList(list), mWindow(window), mViewType(viewType),
	mDescContainer(window), mDescription(window),
	mImage(nullptr), mVideo(nullptr), mThumbnail(nullptr), mFlag(nullptr),
	mKidGame(nullptr), mFavorite(nullptr), mHidden(nullptr), mManual(nullptr),

	mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window),
	mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window), mLblGameTime(window), mLblFavorite(window),

	mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
	mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window),
	mName(window), mGameTime(window), mTextFavorite(window)	
{
	std::vector<MdImage> mdl = 
	{ 
		{ "md_marquee", { MetaDataId::Marquee, MetaDataId::Wheel } },
		{ "md_fanart", { MetaDataId::FanArt, MetaDataId::TitleShot, MetaDataId::Image } },
		{ "md_titleshot", { MetaDataId::TitleShot, MetaDataId::Image } },
		{ "md_boxart", { MetaDataId::BoxArt, MetaDataId::Thumbnail } },
		{ "md_wheel",{ MetaDataId::Wheel, MetaDataId::Marquee } },
		{ "md_cartridge",{ MetaDataId::Cartridge } },
		{ "md_mix",{ MetaDataId::Mix, MetaDataId::Image, MetaDataId::Thumbnail } },
		{ "md_map", { MetaDataId::Map } }
	};

	for (auto md : mdl)
		mdImages.push_back(md);

	const float padding = 0.01f;
	auto mSize = mParent->getSize();

	if (mViewType == DetailedContainerType::GridView)
	{
		mLblRating.setVisible(false);
		mLblReleaseDate.setVisible(false);
		mLblDeveloper.setVisible(false);
		mLblPublisher.setVisible(false);
		mLblGenre.setVisible(false);
		mLblPlayers.setVisible(false);
		mLblLastPlayed.setVisible(false);
		mLblPlayCount.setVisible(false);
		mName.setVisible(false);
		mPlayCount.setVisible(false);
		mLastPlayed.setVisible(false);
		mPlayers.setVisible(false);
		mGenre.setVisible(false);
		mPublisher.setVisible(false);
		mDeveloper.setVisible(false);
		mReleaseDate.setVisible(false);
		mRating.setVisible(false);
		mDescContainer.setVisible(false);
	}

	// metadata labels + values
	mLblGameTime.setVisible(false);
	mGameTime.setVisible(false);
	mLblFavorite.setVisible(false);
	mTextFavorite.setVisible(false);

	// image
	if (mViewType == DetailedContainerType::DetailedView)
		createImageComponent(&mImage);

	mLblRating.setText(_("Rating") + ": ");
	addChild(&mLblRating);
	addChild(&mRating);
	mLblReleaseDate.setText(_("Released") + ": ");
	addChild(&mLblReleaseDate);
	addChild(&mReleaseDate);
	mLblDeveloper.setText(_("Developer") + ": ");
	addChild(&mLblDeveloper);
	addChild(&mDeveloper);
	mLblPublisher.setText(_("Publisher") + ": ");
	addChild(&mLblPublisher);
	addChild(&mPublisher);
	mLblGenre.setText(_("Genre") + ": ");
	addChild(&mLblGenre);
	addChild(&mGenre);
	mLblPlayers.setText(_("Players") + ": ");
	addChild(&mLblPlayers);
	addChild(&mPlayers);
	mLblLastPlayed.setText(_("Last played") + ": ");
	addChild(&mLblLastPlayed);
	mLastPlayed.setDisplayRelative(true);
	addChild(&mLastPlayed);
	mLblPlayCount.setText(_("Times played") + ": ");
	addChild(&mLblPlayCount);
	addChild(&mPlayCount);
	mLblGameTime.setText(_("Game time") + ": ");
	addChild(&mLblGameTime);
	addChild(&mGameTime);
	mLblFavorite.setText(_("Favorite") + ": ");
	addChild(&mLblFavorite);
	addChild(&mTextFavorite);

	mName.setPosition(mSize.x(), mSize.y());
	mName.setDefaultZIndex(40);
	mName.setColor(0xAAAAAAFF);
	mName.setFont(Font::get(FONT_SIZE_MEDIUM));
	mName.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mName);

	mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
	mDescContainer.setSize(mSize.x() * (0.50f - 2 * padding), mSize.y() - mDescContainer.getPosition().y());
	mDescContainer.setAutoScroll(true);
	mDescContainer.setDefaultZIndex(40);
	addChild(&mDescContainer);

	mDescription.setFont(Font::get(FONT_SIZE_SMALL));
	mDescription.setSize(mDescContainer.getSize().x(), 0);
	mDescContainer.addChild(&mDescription);

	initMDLabels();
	initMDValues();
}

DetailedContainer::~DetailedContainer()
{
	if (mThumbnail != nullptr)
		delete mThumbnail;

	if (mImage != nullptr)
		delete mImage;

	for (auto& md : mdImages)
		if (md.component != nullptr)
			delete md.component;

	mdImages.clear();

	if (mVideo != nullptr)
		delete mVideo;

	if (mFlag != nullptr)
		delete mFlag;

	if (mManual != nullptr)
		delete mManual;

	if (mKidGame != nullptr)
		delete mKidGame;

	if (mFavorite != nullptr)
		delete mFavorite;

	if (mHidden != nullptr)
		delete mHidden;
}



std::vector<MdComponent> DetailedContainer::getMetaComponents()
{
	std::vector<MdComponent> mdl =
	{
		{ "rating",   "md_rating",      &mRating,       "md_lbl_rating",      &mLblRating },
		{ "datetime", "md_releasedate", &mReleaseDate,  "md_lbl_releasedate", &mLblReleaseDate },
		{ "text",     "md_developer",   &mDeveloper,    "md_lbl_developer",   &mLblDeveloper },
		{ "text",     "md_publisher",   &mPublisher,    "md_lbl_publisher",   &mLblPublisher },
		{ "text",     "md_genre",       &mGenre,        "md_lbl_genre",       &mLblGenre },
		{ "text",     "md_players",     &mPlayers,      "md_lbl_players",     &mLblPlayers },
		{ "datetime", "md_lastplayed",  &mLastPlayed,   "md_lbl_lastplayed",  &mLblLastPlayed },
		{ "text",     "md_playcount",   &mPlayCount,    "md_lbl_playcount",   &mLblPlayCount },
		{ "text",     "md_gametime",    &mGameTime,     "md_lbl_gametime",    &mLblGameTime },
		{ "text",     "md_favorite",    &mTextFavorite, "md_lbl_favorite",    &mLblFavorite }
	};
	return mdl;	
}


void DetailedContainer::createImageComponent(ImageComponent** pImage)
{
	if (*pImage != nullptr)
		return;

	const float padding = 0.01f;
	auto mSize = mParent->getSize();

	// Image	
	auto image = new ImageComponent(mWindow);
	image->setAllowFading(false);
	image->setOrigin(0.5f, 0.5f);
	image->setPosition(mSize.x() * 0.25f, mList->getPosition().y() + mSize.y() * 0.2125f);
	image->setMaxSize(mSize.x() * (0.50f - 2 * padding), mSize.y() * 0.4f);
	image->setDefaultZIndex(30);
	mParent->addChild(image);

	*pImage = image;
}

void DetailedContainer::createVideo()
{
	if (mVideo != nullptr)
		return;

	const float padding = 0.01f;
	auto mSize = mParent->getSize();

	// video
	// Create the correct type of video window
#ifdef _RPI_
	if (Settings::getInstance()->getBool("VideoOmxPlayer"))
		mVideo = new VideoPlayerComponent(mWindow, "");
	else
#endif
		mVideo = new VideoVlcComponent(mWindow, "");

	// Default is IMAGE in Recalbox themes -> video view does not exist
	mVideo->setSnapshotSource(IMAGE);

	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setPosition(mSize.x() * 0.25f, mSize.y() * 0.4f);
	mVideo->setSize(mSize.x() * (0.5f - 2 * padding), mSize.y() * 0.4f);
	mVideo->setStartDelay(2000);
	mVideo->setDefaultZIndex(31);
	mParent->addChild(mVideo);
}

void DetailedContainer::initMDLabels()
{
	auto mSize = mParent->getSize();
	std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);

	auto components = getMetaComponents();

	const unsigned int colCount = 2;
	const unsigned int rowCount = (int)(components.size() / 2);

	Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

	const float colSize = (mSize.x() * 0.48f) / colCount;
	const float rowPadding = 0.01f * mSize.y();

	int i = 0;	
	for (unsigned int i = 0; i < components.size(); i++)
	{
		if (components[i].labelid.empty() || components[i].label == nullptr)
			continue;

		const unsigned int row = i % rowCount;
		Vector3f pos(0.0f, 0.0f, 0.0f);
		if (row == 0)
			pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
		else 
		{
			// work from the last component
			GuiComponent* lc = components[i - 1].label;
			pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
		}

		components[i].label->setFont(defaultFont);
		components[i].label->setPosition(pos);
		components[i].label->setDefaultZIndex(40);
	}
}

void DetailedContainer::initMDValues()
{
	auto mSize = mParent->getSize();

	auto components = getMetaComponents();

	std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
	mRating.setSize(defaultFont->getHeight() * 5.0f, (float)defaultFont->getHeight());

	float bottom = 0.0f;

	const float colSize = (mSize.x() * 0.48f) / 2;
	for (unsigned int i = 0; i < components.size(); i++)
	{
		TextComponent* text = dynamic_cast<TextComponent*>(components[i].component);
		if (text != nullptr)
			text->setFont(defaultFont);
		
		if (components[i].labelid.empty() || components[i].label == nullptr)
			continue;

		const float heightDiff = (components[i].label->getSize().y() - components[i].label->getSize().y()) / 2;
		components[i].component->setPosition(components[i].label->getPosition() + Vector3f(components[i].label->getSize().x(), heightDiff, 0));
		components[i].component->setSize(colSize - components[i].label->getSize().x(), components[i].label->getSize().y());
		components[i].component->setDefaultZIndex(40);

		float testBot = components[i].component->getPosition().y() + components[i].component->getSize().y();
		if (testBot > bottom)
			bottom = testBot;
	}

	mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + mSize.y() * 0.01f);
	mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() - mDescContainer.getPosition().y());
}

void DetailedContainer::loadIfThemed(ImageComponent** pImage, const std::shared_ptr<ThemeData>& theme, const std::string& element, bool forceLoad, bool loadPath)
{
	using namespace ThemeFlags;

	if (forceLoad || theme->getElement(getName(), element, "image"))
	{
		createImageComponent(pImage);
		(*pImage)->applyTheme(theme, getName(), element, loadPath ? ALL : ALL ^ (PATH));
	}
	else if ((*pImage) != nullptr)
	{
		removeChild(*pImage);
		delete (*pImage);
		(*pImage) = nullptr;
	}
}

void DetailedContainer::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	using namespace ThemeFlags;

	mName.applyTheme(theme, getName(), "md_name", ALL);	

	if (theme->getElement(getName(), "md_video", "video"))
	{
		createVideo();
		mVideo->applyTheme(theme, getName(), "md_video", ALL ^ (PATH));
	}
	else if (mVideo != nullptr)
	{
		removeChild(mVideo);
		delete mVideo;
		mVideo = nullptr;
	}

	loadIfThemed(&mImage, theme, "md_image", (mVideo == nullptr && mViewType == DetailedContainerType::DetailedView));
	loadIfThemed(&mThumbnail, theme, "md_thumbnail");
	loadIfThemed(&mFlag, theme, "md_flag");

	for (auto& md : mdImages)
		loadIfThemed(&md.component, theme, md.id);

	loadIfThemed(&mKidGame, theme, "md_kidgame", false, true);
	loadIfThemed(&mFavorite, theme, "md_favorite", false, true);
	loadIfThemed(&mHidden, theme, "md_hidden", false, true);
	loadIfThemed(&mManual, theme, "md_manual", false, true);

	initMDLabels();

	for (auto ctrl : getMetaComponents())
		if (ctrl.label != nullptr && theme->getElement(getName(), ctrl.labelid, "text"))
			ctrl.label->applyTheme(theme, getName(), ctrl.labelid, ALL);

	initMDValues();

	for (auto ctrl : getMetaComponents())
		if (ctrl.component != nullptr && theme->getElement(getName(), ctrl.id, ctrl.expectedType))
			ctrl.component->applyTheme(theme, getName(), ctrl.id, ALL);

	if (theme->getElement(getName(), "md_description", "text"))
	{
		mDescContainer.applyTheme(theme, getName(), "md_description", POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
		mDescription.setSize(mDescContainer.getSize().x(), 0);
		mDescription.applyTheme(theme, getName(), "md_description", ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

		if (!isChild(&mDescContainer))
			addChild(&mDescContainer);
	}
	else if (mViewType == DetailedContainerType::GridView)
		removeChild(&mDescContainer);

	mParent->sortChildren();
}

Vector3f DetailedContainer::getLaunchTarget()
{
	Vector3f target(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f, 0);

	if (mVideo != nullptr)
		target = Vector3f(mVideo->getCenter().x(), mVideo->getCenter().y(), 0);
	else if (mImage != nullptr && mImage->hasImage())
		target = Vector3f(mImage->getCenter().x(), mImage->getCenter().y(), 0);
	else if (mThumbnail != nullptr && mThumbnail->hasImage())
		target = Vector3f(mThumbnail->getCenter().x(), mThumbnail->getCenter().y(), 0);

	return target;
}

void DetailedContainer::updateControls(FileData* file, bool isClearing)
{
	bool fadingOut;
	if (file == NULL)
	{
		if (mVideo != nullptr)
		{
			mVideo->setVideo("");
			mVideo->setImage("");
		}

		if (mImage != nullptr && mViewType == DetailedContainerType::GridView)
			mImage->setImage("");

		for (auto& md : mdImages)
			if (md.component != nullptr)
				md.component->setImage("");

		if (mManual != nullptr) mManual->setVisible(false);
		if (mKidGame != nullptr) mKidGame->setVisible(false);
		if (mFavorite != nullptr) mFavorite->setVisible(false);
		if (mHidden != nullptr) mHidden->setVisible(false);

		fadingOut = true;
	}
	else
	{
		std::string imagePath = file->getImagePath().empty() ? file->getThumbnailPath() : file->getImagePath();

		if (mVideo != nullptr)
		{
			if (!mVideo->setVideo(file->getVideoPath()))
				mVideo->setDefaultVideo();

			std::string snapShot = imagePath;

			auto src = mVideo->getSnapshotSource();
			if (src == MARQUEE && !file->getMarqueePath().empty())
				snapShot = file->getMarqueePath();
			if (src == THUMBNAIL && !file->getThumbnailPath().empty())
				snapShot = file->getThumbnailPath();			
			if (src == IMAGE && !file->getImagePath().empty())
				snapShot = file->getImagePath();

			mVideo->setImage(snapShot);
		}

		if (mThumbnail != nullptr)
		{
			if (mViewType == DetailedContainerType::VideoView && mImage != nullptr)
				mImage->setImage(file->getImagePath(), false, mImage->getMaxSizeInfo());

			mThumbnail->setImage(file->getThumbnailPath(), false, mThumbnail->getMaxSizeInfo());
		}
		
		if (mImage != nullptr)
		{
			if (mViewType == DetailedContainerType::VideoView && mThumbnail == nullptr)
				mImage->setImage(file->getThumbnailPath(), false, mImage->getMaxSizeInfo());
			else if (mViewType != DetailedContainerType::VideoView)
				mImage->setImage(imagePath, false, mImage->getMaxSizeInfo());
		}

		for (auto& md : mdImages)
		{
			if (md.component != nullptr)
			{
				std::string image;

				for (auto& id : md.metaDataIds)
				{
					if (id == MetaDataId::Marquee)
					{
						if (Utils::FileSystem::exists(file->getMarqueePath()))
						{
							image = file->getMarqueePath();
							break;
						}

						continue;
					}

					std::string path = file->getMetadata(id);
					if (Utils::FileSystem::exists(path)) 
					{
						image = path;
						break;
					}
				}

				if (!image.empty())
					md.component->setImage(image, false, md.component->getMaxSizeInfo());
				else
					md.component->setImage("");
			}
		}

		if (mFlag != nullptr)
		{
			if (file->getType() == GAME)
			{
				file->detectLanguageAndRegion(false);
				mFlag->setImage(":/flags/" + LangInfo::getFlag(file->getMetadata(MetaDataId::Language), file->getMetadata(MetaDataId::Region)) + ".png"
					, false, mFlag->getMaxSizeInfo());
			}
			else
				mFlag->setImage(":/folder.svg");
		}

		if (mManual != nullptr)
			mManual->setVisible(Utils::FileSystem::exists(file->getMetadata(MetaDataId::Manual)));

		if (mKidGame != nullptr)
			mKidGame->setVisible(file->getKidGame());

		if (mFavorite != nullptr)
			mFavorite->setVisible(file->getFavorite());

		if (mHidden != nullptr)
			mHidden->setVisible(file->getHidden());

		mDescription.setText(file->getMetadata(MetaDataId::Desc));
		mDescContainer.reset();

		auto valueOrUnknown = [](const std::string value) { return value.empty() ? _("Unknown") : value; };

		mRating.setValue(file->getMetadata(MetaDataId::Rating));
		mReleaseDate.setValue(file->getMetadata(MetaDataId::ReleaseDate));
		mDeveloper.setValue(valueOrUnknown(file->getMetadata(MetaDataId::Developer)));
		mPublisher.setValue(valueOrUnknown(file->getMetadata(MetaDataId::Publisher)));
		mGenre.setValue(valueOrUnknown(file->getMetadata(MetaDataId::Genre)));
		mPlayers.setValue(valueOrUnknown(file->getMetadata(MetaDataId::Players)));
		mName.setValue(file->getMetadata(MetaDataId::Name));
		mTextFavorite.setText(file->getFavorite()?_("YES"):_("NO"));

		if (file->getType() == GAME)
		{
			mLastPlayed.setValue(file->getMetadata(MetaDataId::LastPlayed));
			mPlayCount.setValue(file->getMetadata(MetaDataId::PlayCount));
			mGameTime.setValue(Utils::Time::secondsToString(atol(file->getMetadata(MetaDataId::GameTime).c_str())));
		}

		fadingOut = false;
	}

	// We're clearing / populating : don't setup fade animations
	if (file == nullptr && isClearing) //mList.getObjects().size() == 0 && mList.getCursorIndex() == 0 && mList.getScrollingVelocity() == 0)
		return;

	std::vector<GuiComponent*> comps;

	for (auto lbl : getMetaComponents())
		if (lbl.component != nullptr)
			comps.push_back(lbl.component);

	if (mVideo != nullptr) comps.push_back(mVideo);
	if (mImage != nullptr) comps.push_back(mImage);
	if (mThumbnail != nullptr) comps.push_back(mThumbnail);
	if (mFlag != nullptr) comps.push_back(mFlag);
	if (mManual != nullptr) comps.push_back(mManual);
	if (mKidGame != nullptr) comps.push_back(mKidGame);
	if (mFavorite != nullptr) comps.push_back(mFavorite);
	if (mHidden != nullptr) comps.push_back(mHidden);

	for (auto& md : mdImages)
		if (md.component != nullptr)
			comps.push_back(md.component);

	comps.push_back(&mDescription);
	comps.push_back(&mName);

	for (auto lbl : getMetaComponents())
		if (lbl.label != nullptr)
			comps.push_back(lbl.label);

	for (auto it = comps.cbegin(); it != comps.cend(); it++)
	{
		GuiComponent* comp = *it;
		// an animation is playing
		//   then animate if reverse != fadingOut
		// an animation is not playing
		//   then animate if opacity != our target opacity
		if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
			(!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255)))
		{
			auto func = [comp](float t)
			{
				comp->setOpacity((unsigned char)(Math::lerp(0.0f, 1.0f, t) * 255));
			};

			bool isFadeOut = fadingOut;
			comp->setAnimation(new LambdaAnimation(func, 150), 0, [this, isFadeOut, file]
			{
				if (isFadeOut)
				{
					if (mVideo != nullptr) mVideo->setImage("");
					if (mImage != nullptr) mImage->setImage("");
					if (mThumbnail != nullptr) mThumbnail->setImage("");
					if (mFlag != nullptr) mFlag->setImage("");
					if (mManual != nullptr) mManual->setVisible(false);
					if (mKidGame != nullptr) mKidGame->setVisible(false);
					if (mFavorite != nullptr) mFavorite->setVisible(false);
					if (mHidden != nullptr) mHidden->setVisible(false);

					for (auto& md : mdImages)
						if (md.component != nullptr)
							md.component->setImage("");
				}
			}, fadingOut);
		}
	}

	Utils::FileSystem::removeFile(getTitlePath());
}

void DetailedContainer::update(int deltaTime)
{
	if (mVideo != nullptr)
		mVideo->update(deltaTime);
}