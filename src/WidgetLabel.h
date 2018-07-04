/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * class WidgetLabel
 *
 * A simple text display for menus.
 * This is preferred to directly displaying text because it helps handle positioning
 */

#ifndef WIDGET_LABEL_H
#define WIDGET_LABEL_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "Widget.h"

class LabelInfo {
public:
	enum {
		VALIGN_CENTER = 0,
		VALIGN_TOP = 1,
		VALIGN_BOTTOM = 2
	};

	int x,y;
	int justify,valign;
	bool hidden;
	std::string font_style;

	LabelInfo();
};

class WidgetLabel : public Widget {
private:
	enum {
		UPDATE_NONE = 0,
		UPDATE_POS = 1,
		UPDATE_RECACHE = 2
	};

	void recacheTextSprite();
	void applyOffsets();
	void setUpdateFlag(int _update_flag);
	void update();

	int justify;
	int valign;
	int max_width;
	int update_flag;
	bool hidden;
	bool window_resize_flag;
	Sprite *label;

	std::string text;
	std::string font_style;
	Color color;

	Rect bounds;

public:
	static const std::string DEFAULT_FONT;

	WidgetLabel();
	~WidgetLabel();
	void render();
	void setMaxWidth(int width);
	void setHidden(bool _hidden);
	void setPos(int offset_x, int offset_y);
	void setJustify(int _justify);
	void setVAlign(int _valign);
	void setText(const std::string& _text);
	void setColor(const Color& _color);
	void setFont(const std::string& _font);
	void setFromLabelInfo(const LabelInfo& label_info);

	std::string getText();
	Rect* getBounds();
	bool isHidden();
};

#endif
