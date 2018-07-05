/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2014-2016 Justin Jacobs

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

#include "FontEngine.h"
#include "SharedResources.h"
#include "TooltipData.h"

TooltipData::TooltipData()
{}

TooltipData::~TooltipData()
{}

void TooltipData::addColoredText(const std::string &text, const Color& color) {
	if (lines.empty() || !lines.back().empty()) {
		lines.push_back("");
		colors.push_back(color);
	}

	size_t cur = 0;
	while (cur < text.length()) {
		if (text[cur] == '\n') {
			if (lines.back().empty())
				lines.back() = " ";

			lines.push_back("");
			colors.push_back(color);
		}
		else {
			lines.back() += text[cur];
		}

		cur++;
	}

	if (lines.back().empty())
		lines.back() = " ";
}

void TooltipData::addText(const std::string &text) {
	addColoredText(text, font->getColor(FontEngine::COLOR_WIDGET_NORMAL));
}

void TooltipData::clear() {
	lines.clear();
	colors.clear();
}

bool TooltipData::isEmpty() const {
	return lines.empty();
}

bool TooltipData::compareFirstLine(const std::string &text) {
	if (lines.empty()) return false;
	if (lines[0] != text) return false;
	return true;
}

bool TooltipData::compare(const TooltipData& tip) {
	if (lines.size() != tip.lines.size())
		return false;

	for (size_t i = 0; i < lines.size(); ++i) {
		if (lines[i] != tip.lines[i] || colors[i] != tip.colors[i])
			return false;
	}

	return true;
}

