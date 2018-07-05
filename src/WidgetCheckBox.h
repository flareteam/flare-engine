/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2014 Justin Jacobs

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
 * class WidgetCheckBox
 */

#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H

class Widget;

class WidgetCheckBox : public Widget {
public:
	static const std::string DEFAULT_FILE;

	explicit WidgetCheckBox (const std::string &fname);
	~WidgetCheckBox ();

	void activate();
	void setChecked(const bool status);
	bool checkClick();
	bool checkClickAt(int x, int y);
	bool isChecked() const;
	void render();
	bool enabled;
	std::string tooltip;

private:
	void checkTooltip(const Point& mouse);

	Sprite *cb;
	bool checked;
	bool pressed;
	bool activated;
};

#endif

