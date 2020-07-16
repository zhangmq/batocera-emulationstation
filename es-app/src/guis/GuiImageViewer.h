#pragma once

#include "GuiComponent.h"
#include "Window.h"
#include "components/ImageGridComponent.h"

class ThemeData;

class GuiImageViewer : public GuiComponent
{
public:
	static void showPdf(Window* window, const std::string imagePath);
	static void showImage(Window* window, const std::string imagePath);

	GuiImageViewer(Window* window, bool linearSmooth = false);
	~GuiImageViewer();

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	void add(const std::string imagePath);
	void setCursor(const std::string imagePath);

protected:
	ImageGridComponent<std::string> mGrid;
	std::shared_ptr<ThemeData> mTheme;
};