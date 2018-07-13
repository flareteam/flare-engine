/*
Copyright © 2018 Justin Jacobs

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

#include "InputState.h"
#include "MenuTouchControls.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"

MenuTouchControls::MenuTouchControls()
	: Menu()
	, move_radius(settings->view_h / 4)
	, move_center(0,0)
	, move_center_base(move_radius, -(move_radius / 2))
	, move_align(Utils::ALIGN_BOTTOMLEFT)
	, move_deadzone(settings->view_h / 20)
	, main1_radius(settings->view_h / 6)
	, main1_center(0,0)
	, main1_center_base(-main1_radius - (settings->view_h / 4), -(settings->view_h / 8))
	, main1_align(Utils::ALIGN_BOTTOMRIGHT)
	, main2_radius(settings->view_h / 6)
	, main2_center(0,0)
	, main2_center_base(0, -(settings->view_h / 6))
	, main2_align(Utils::ALIGN_BOTTOMRIGHT)
	, radius_padding(settings->view_h / 20)
{
	visible = true;
	align();
}

void MenuTouchControls::alignInput(Point& center, const Point& center_base, const int radius, const int _align) {
	Rect input_rect;
	input_rect.x = center_base.x - radius;
	input_rect.y = center_base.y - radius;
	input_rect.w = input_rect.h = radius;

	Utils::alignToScreenEdge(_align, &input_rect);
	center.x = input_rect.x + radius;
	center.y = input_rect.y + radius;
}

void MenuTouchControls::align() {
	alignInput(move_center, move_center_base, move_radius, move_align);
	alignInput(main1_center, main1_center_base, main1_radius, main1_align);
	alignInput(main2_center, main2_center_base, main2_radius, main2_align);
}

void MenuTouchControls::logic() {
	if (!visible || !platform.is_mobile_device)
		return;

	inpt->pressing[Input::LEFT] = inpt->pressing[Input::RIGHT] = inpt->pressing[Input::UP] = inpt->pressing[Input::DOWN] = false;
	inpt->pressing[Input::MAIN2] = false;

	FPoint mv_center(static_cast<float>(move_center.x), static_cast<float>(move_center.y));
	FPoint m2_center(static_cast<float>(main2_center.x), static_cast<float>(main2_center.y));

	FPoint mouse(static_cast<float>(inpt->mouse.x), static_cast<float>(inpt->mouse.y));

	if (inpt->pressing[Input::MAIN1] && Utils::isWithinRadius(mv_center, static_cast<float>(move_radius), mouse)) {
		if (inpt->mouse.x < move_center.x - move_deadzone)
			inpt->pressing[Input::LEFT] = true;
		if (inpt->mouse.x > move_center.x + move_deadzone)
			inpt->pressing[Input::RIGHT] = true;
		if (inpt->mouse.y < move_center.y - move_deadzone)
			inpt->pressing[Input::UP] = true;
		if (inpt->mouse.y > move_center.y + move_deadzone)
			inpt->pressing[Input::DOWN] = true;
	}

	// checking for MAIN1 is redundant, as the touch event itself triggers that

	if (inpt->pressing[Input::MAIN1] && Utils::isWithinRadius(m2_center, static_cast<float>(main2_radius), mouse)) {
		inpt->pressing[Input::MAIN2] = true;
	}
}

bool MenuTouchControls::checkAllowMain1() {
	if (!visible || !platform.is_mobile_device)
		return true;

	FPoint m1_center(static_cast<float>(main1_center.x), static_cast<float>(main1_center.y));
	FPoint mouse(static_cast<float>(inpt->mouse.x), static_cast<float>(inpt->mouse.y));

	return Utils::isWithinRadius(m1_center, static_cast<float>(main1_radius), mouse);
}

void MenuTouchControls::renderInput(const Point& center, const int radius, const Color& color) {
	render_device->drawEllipse(center.x - radius, center.y - radius, center.x + radius, center.y + radius, color, 15);
}

void MenuTouchControls::render() {
	if (!visible || !platform.is_mobile_device)
		return;

	Color color_normal(255,255,255,255);
	Color color_deadzone(127,127,127,255);

	renderInput(move_center, move_radius - radius_padding, color_normal);
	if (move_deadzone > 0) {
		renderInput(move_center, move_deadzone, color_deadzone);
	}
	renderInput(main1_center, main1_radius - radius_padding, color_normal);
	renderInput(main2_center, main2_radius - radius_padding, color_normal);
}

MenuTouchControls::~MenuTouchControls() {
}

