#pragma once
#ifndef ES_CORE_THEME_DATA_H
#define ES_CORE_THEME_DATA_H

#include "math/Vector2f.h"
#include "math/Vector4f.h"
#include "utils/FileSystemUtil.h"
#include <deque>
#include <map>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <vector>
#include <pugixml/src/pugixml.hpp>

namespace pugi { class xml_node; }

template<typename T>
class TextListComponent;

class GuiComponent;
class ImageComponent;
class NinePatchComponent;
class Sound;
class TextComponent;
class Window;
class Font;

namespace ThemeFlags
{
	enum PropertyFlags : unsigned int
	{
		PATH = 1,
		POSITION = 2,
		SIZE = 4,
		ORIGIN = 8,
		COLOR = 16,
		FONT_PATH = 32,
		FONT_SIZE = 64,
		SOUND = 128,
		ALIGNMENT = 256,
		TEXT = 512,
		FORCE_UPPERCASE = 1024,
		LINE_SPACING = 2048,
		DELAY = 4096,
		Z_INDEX = 8192,
		ROTATION = 16384,
		VISIBLE = 32768,
		ALL = 0xFFFFFFFF
	};
}

class ThemeException : public std::exception
{
public:
	std::string msg;

	virtual const char* what() const throw() { return msg.c_str(); }

	template<typename T>
	friend ThemeException& operator<<(ThemeException& e, T msg);
	
	inline void setFiles(const std::deque<std::string>& deque)
	{
		*this << "from theme \"" << deque.front() << "\"\n";
		for(auto it = deque.cbegin() + 1; it != deque.cend(); it++)
			*this << "  (from included file \"" << (*it) << "\")\n";
		*this << "    ";
	}
};

template<typename T>
ThemeException& operator<<(ThemeException& e, T appendMsg)
{
	std::stringstream ss;
	ss << e.msg << appendMsg;
	e.msg = ss.str();
	return e;
}

struct ThemeSet
{
	std::string path;

	inline std::string getName() const { return Utils::FileSystem::getStem(path); }
	inline std::string getThemePath(const std::string& system) const { return path + "/" + system + "/theme.xml"; }
};


struct Subset
{
	Subset(const std::string set, const std::string nm, const std::string dn, const std::string ssdn)
	{
		subset = set;
		name = nm;
		displayName = dn;
		subSetDisplayName = ssdn;
	}

	std::string subset;
	std::string name;	

	std::string displayName;

	std::vector<std::string> appliesTo;

	std::string subSetDisplayName;
};

struct MenuElement 
{
	unsigned int color;
	unsigned int selectedColor;
	unsigned int selectorColor;
	unsigned int separatorColor;
	unsigned int selectorGradientColor;
	bool selectorGradientType;
	std::string path;
	std::shared_ptr<Font> font;
};

struct MenuBackground
{
	unsigned int color;
	unsigned int centerColor;
	std::string path;
	std::string fadePath;	
	Vector2f cornerSize;	
};

struct MenuGroupElement
{
	unsigned int color;
	unsigned int backgroundColor;
	unsigned int separatorColor;
	float lineSpacing;
	std::string path;
	std::shared_ptr<Font> font;
	int alignment;
	bool visible;
};

struct IconElement 
{
	std::string button;
	std::string button_filled;
	std::string on;
	std::string off;
	std::string option_arrow;
	std::string arrow;
	std::string knob;
	std::string textinput_ninepatch;
	std::string textinput_ninepatch_active;
};

class ThemeData
{
public:
	class ThemeMenu
	{
	public:
		ThemeMenu(ThemeData* theme);

		MenuBackground Background{ 0xFFFFFFFF, 0xFFFFFFFF, ":/frame.png", ":/scroll_gradient.png", Vector2f(16, 16) };
		MenuElement Title{ 0x555555FF, 0x555555FF, 0x555555FF, 0xFFFFFFFF, 0x555555FF, true, "", nullptr };
		MenuElement Text{ 0x777777FF, 0xFFFFFFFF, 0x878787FF, 0xC6C7C6FF, 0x878787FF, true, "", nullptr };
		MenuElement TextSmall{ 0x777777FF, 0xFFFFFFFF, 0x878787FF, 0xC6C7C6FF, 0x878787FF, true, "", nullptr };
		MenuElement Footer{ 0xC6C6C6FF, 0xC6C6C6FF, 0xC6C6C6FF, 0xFFFFFFFF, 0xC6C6C6FF, true, "", nullptr };
		MenuGroupElement Group{ 0x777777FF, 0x00000010, 0xC6C7C6FF, 2.0, "", nullptr, 0 /*ALIGN_LEFT*/, false };
		IconElement Icons{ ":/button.png", ":/button_filled.png", ":/on.svg", ":/off.svg", ":/option_arrow.svg", ":/arrow.svg", ":/slider_knob.svg", ":/textinput_ninepatch.png", ":/textinput_ninepatch_active.png" };

		std::string getMenuIcon(const std::string name)
		{
			auto it = mMenuIcons.find(name);
			if (it != mMenuIcons.cend())
				return it->second;

			return "";
		}

	private:
		std::map<std::string, std::string>		mMenuIcons;
	};

	class ThemeElement
	{
	public:
		ThemeElement() { extra = 0; }

		int extra;

		std::string type;

		struct Property
		{
			void operator= (const Vector2f& value)     { v = value; }
			void operator= (const std::string& value)  { s = value; }
			void operator= (const unsigned int& value) { i = value; }
			void operator= (const float& value)        { f = value; }
			void operator= (const bool& value)         { b = value; }
			void operator= (const Vector4f& value)     { r = value; v = Vector2f(value.x(), value.y()); }

			Vector2f     v;
			std::string  s;
			unsigned int i;
			float        f;
			bool         b;
			Vector4f     r;
		};

		std::map< std::string, Property > properties;

		template<typename T>
		const T get(const std::string& prop) const
		{
			if(     std::is_same<T, Vector2f>::value)     return *(const T*)&properties.at(prop).v;
			else if(std::is_same<T, std::string>::value)  return *(const T*)&properties.at(prop).s;
			else if(std::is_same<T, unsigned int>::value) return *(const T*)&properties.at(prop).i;
			else if(std::is_same<T, float>::value)        return *(const T*)&properties.at(prop).f;
			else if(std::is_same<T, bool>::value)         return *(const T*)&properties.at(prop).b;
			else if (std::is_same<T, Vector4f>::value)         return *(const T*)&properties.at(prop).r;
			return T();
		}

		inline bool has(const std::string& prop) const { return (properties.find(prop) != properties.cend()); }
	};

private:
	class ThemeView
	{
	public:
		ThemeView() { isCustomView = false; }

		std::map<std::string, ThemeElement> elements;
		std::vector<std::string> orderedKeys;
		std::string baseType;

		std::vector<std::string> baseTypes;

		bool isOfType(const std::string type);

		std::string displayName;

		bool isCustomView;
	};

public:

	ThemeData();

	// throws ThemeException
	void loadFile(const std::string system, std::map<std::string, std::string> sysDataMap, const std::string& path, bool fromFile = true);

	enum ElementPropertyType
	{
		NORMALIZED_RECT,
		NORMALIZED_PAIR,
		PATH,
		STRING,
		COLOR,
		FLOAT,
		BOOLEAN
	};

	bool hasView(const std::string& view);

	bool isCustomView(const std::string& view);
	std::string getCustomViewBaseType(const std::string& view);

	// If expectedType is an empty string, will do no type checking.
	const ThemeElement* getElement(const std::string& view, const std::string& element, const std::string& expectedType) const;

	static std::vector<GuiComponent*> makeExtras(const std::shared_ptr<ThemeData>& theme, const std::string& view, Window* window, bool forceLoad = false);

	static const std::shared_ptr<ThemeData>& getDefault();

	static std::map<std::string, ThemeSet> getThemeSets();
	static std::string getThemeFromCurrentSet(const std::string& system);
	
	bool hasSubsets() { return mSubsets.size() > 0; }
	static const std::shared_ptr<ThemeData::ThemeMenu>& getMenuTheme();

	std::vector<Subset>		    getSubSets() { return mSubsets; }
	std::vector<std::string>	getSubSetNames(const std::string ofView = "");

	std::string					getDefaultSubSetValue(const std::string subsetname);

	static std::vector<Subset> getSubSet(const std::vector<Subset>& subsets, const std::string& subset);

	static void setDefaultTheme(ThemeData* theme);
	static ThemeData* getDefaultTheme() { return mDefaultTheme; }
	
	std::string getSystemThemeFolder() { return mSystemThemeFolder; }
	
	std::vector<std::pair<std::string, std::string>> getViewsOfTheme();
	std::string getDefaultView() { return mDefaultView; };

	std::string getVariable(std::string name)
	{
		if (mVariables.find(name) != mVariables.cend())
			return mVariables[name];

		return "";
	}	

	std::string getViewDisplayName(const std::string& view);

private:
	static std::map< std::string, std::map<std::string, ElementPropertyType> > sElementMap;
	static std::vector<std::string> sSupportedFeatures;
	static std::vector<std::string> sSupportedViews;

	std::deque<std::string> mPaths;
	float mVersion;
	std::string mDefaultView;

	void parseTheme(const pugi::xml_node& root);

	void parseFeature(const pugi::xml_node& node);	
	void parseInclude(const pugi::xml_node& node);	
	void parseVariable(const pugi::xml_node& node);
	void parseVariables(const pugi::xml_node& root);
	void parseViews(const pugi::xml_node& themeRoot);
	void parseCustomView(const pugi::xml_node& node, const pugi::xml_node& root);	
	void parseViewElement(const pugi::xml_node& node);
	void parseView(const pugi::xml_node& viewNode, ThemeView& view, bool overwriteElements = true);
	void parseElement(const pugi::xml_node& elementNode, const std::map<std::string, ElementPropertyType>& typeMap, ThemeElement& element, bool overwrite = true);
	bool parseRegion(const pugi::xml_node& node);
	bool parseSubset(const pugi::xml_node& node);
	bool isFirstSubset(const pugi::xml_node& node);
	bool parseLanguage(const pugi::xml_node& node);
	bool parseFilterAttributes(const pugi::xml_node& node);
	void parseSubsetElement(const pugi::xml_node& root);

	void parseCustomViewBaseClass(const pugi::xml_node& root, ThemeView& view, std::string baseClass);

	std::string resolveSystemVariable(const std::string& systemThemeFolder, const std::string& path);
	std::string resolvePlaceholders(const char* in);

	std::string mColorset;
	std::string mIconset;
	std::string mMenu;
	std::string mSystemview;
	std::string mGamelistview;
	std::string mSystemThemeFolder;	
	std::string mLanguage;
	std::string mRegion;

	std::map<std::string, std::string> mVariables;
	
	class UnsortedViewMap : public std::vector<std::pair<std::string, ThemeView>>
	{
	public:		
		std::vector<std::pair<std::string, ThemeView>>::const_iterator find(std::string view) const
		{
			for (std::vector<std::pair<std::string, ThemeView>>::const_iterator it = cbegin(); it != cend(); it++)
				if (it->first == view)
					return it;

			return cend();
		}
	
		std::vector<std::pair<std::string, ThemeView>>::iterator find(std::string view)
		{
			for (std::vector<std::pair<std::string, ThemeView>>::iterator it = begin(); it != end(); it++)
				if (it->first == view)
					return it;

			return end();
		}

		std::pair<std::vector<std::pair<std::string, ThemeView>>::iterator, bool> insert(std::pair<std::string, ThemeView> item)
		{			
			std::pair<std::vector<std::pair<std::string, ThemeView>>::iterator, bool> ret;

			ret.first = find(item.first);
			ret.second = ret.first != cend();

			if (ret.first == cend())
			{
				push_back(item);
				ret.first = find(item.first);
			}

			return ret;			
		}
	};

	UnsortedViewMap mViews;
	//	std::map<std::string, ThemeView> mViews;

	std::vector<Subset> mSubsets;

	static std::shared_ptr<ThemeData::ThemeMenu> mMenuTheme;
	static ThemeData* mDefaultTheme;	
};

#endif // ES_CORE_THEME_DATA_H
