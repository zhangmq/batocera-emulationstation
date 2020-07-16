#pragma once
#ifndef ES_CORE_COMPONENTS_COMPONENT_LIST_H
#define ES_CORE_COMPONENTS_COMPONENT_LIST_H

#include "IList.h"
#include "LocaleES.h"

struct ComponentListElement
{
	ComponentListElement(const std::shared_ptr<GuiComponent>& cmp = nullptr, bool resize_w = true, bool inv = true)
		: component(cmp), resize_width(resize_w), invert_when_selected(inv) { };

	std::shared_ptr<GuiComponent> component;
	bool resize_width;
	bool invert_when_selected;
};

struct ComponentListRow
{
	ComponentListRow() 
	{
		selectable = true;
	};

	bool selectable;
	std::vector<ComponentListElement> elements;

	// The input handler is called when the user enters any input while this row is highlighted (including up/down).
	// Return false to let the list try to use it or true if the input has been consumed.
	// If no input handler is supplied (input_handler == nullptr), the default behavior is to forward the input to 
	// the rightmost element in the currently selected row.
	std::function<bool(InputConfig*, Input)> input_handler;
	
	inline void addElement(const std::shared_ptr<GuiComponent>& component, bool resize_width, bool invert_when_selected = true)
	{
		if (EsLocale::isRTL())
			elements.insert(elements.begin(), ComponentListElement(component, resize_width, invert_when_selected));
		else
			elements.push_back(ComponentListElement(component, resize_width, invert_when_selected));
	}

	// Utility method for making an input handler for "when the users presses A on this, do func."
	inline void makeAcceptInputHandler(const std::function<void()>& func, bool onButtonRelease = false)
	{
		if (func == nullptr)
			return;

		input_handler = [func, onButtonRelease](InputConfig* config, Input input) -> bool {
			if(config->isMappedTo(BUTTON_OK, input) && (onButtonRelease ? input.value == 0 : input.value != 0)) // batocera
			{
				func();
				return true;
			}
			return false;
		};
	}
};

class ComponentList : public IList<ComponentListRow, void*>
{
public:
	ComponentList(Window* window);

	void addRow(const ComponentListRow& row, bool setCursorHere = false);
	void addGroup(const std::string& label, bool forceVisible = false);

	void textInput(const char* text) override;
	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	void onSizeChanged() override;
	void onFocusGained() override;
	void onFocusLost() override;

	bool moveCursor(int amt);
	inline int getCursorId() const { return mCursor; }
	
	float getTotalRowHeight() const;
	inline float getRowHeight(int row) const { return getRowHeight(mEntries.at(row).data); }

	inline void setCursorChangedCallback(const std::function<void(CursorState state)>& callback) { mCursorChangedCallback = callback; };
	inline const std::function<void(CursorState state)>& getCursorChangedCallback() const { return mCursorChangedCallback; };

protected:
	void onCursorChanged(const CursorState& state) override;

	virtual int onBeforeScroll(int cursor, int direction) override
	{ 
		int looped = cursor;
		while (cursor >= 0 && cursor < mEntries.size() && !mEntries[cursor].data.selectable)
		{
			cursor += direction;
			if (cursor == looped || cursor < 0 || cursor >= mEntries.size())
				return mCursor;
		}
		
		return cursor; 
	}

private:
	bool mFocused;

	void updateCameraOffset();
	void updateElementPosition(const ComponentListRow& row);
	void updateElementSize(const ComponentListRow& row);
	
	float getRowHeight(const ComponentListRow& row) const;

	float mSelectorBarOffset;
	float mCameraOffset;

	std::function<void(CursorState state)> mCursorChangedCallback;
};

#endif // ES_CORE_COMPONENTS_COMPONENT_LIST_H
