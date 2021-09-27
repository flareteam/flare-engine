/*
Copyright © 2020 Justin Jacobs

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

#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "MenuMovementType.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"

#include <string>

MenuMovementType::MenuMovementType()
	: Menu()
	, button_keyboard(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_mouse(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_joystick(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, icon_keyboard(NULL)
	, icon_mouse(NULL)
	, icon_joystick(NULL)
{
	// Load config settings
	FileParser infile;
	// @CLASS MenuMovementType|Description of menus/movement_type.txt
	if(infile.open("menus/movement_type.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "label_title") {
				// @ATTR label_title|label|Position of the "Select a Movement Type" text.
				label_title.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "label_config_hint") {
				// @ATTR label_config_hint|label|Position of the "Can be changed later in the Configuration menu" text.
				label_config_hint.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "button_keyboard") {
				// @ATTR button_keyboard|point|Position of the "Keyboard" button.
				Point pos = Parse::toPoint(infile.val);
				button_keyboard->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "button_mouse") {
				// @ATTR button_mouse|point|Position of the "Mouse" button.
				Point pos = Parse::toPoint(infile.val);
				button_mouse->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "button_joystick") {
				// @ATTR button_joystick|point|Position of the "Joystick" button.
				Point pos = Parse::toPoint(infile.val);
				button_joystick->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "icon_keyboard") {
				// @ATTR icon_keyboard|point|Position of the keyboard icon.
				icon_keyboard_pos = Parse::toPoint(infile.val);
			}
			else if (infile.key == "icon_mouse") {
				// @ATTR icon_mouse|point|Position of the mouse icon.
				icon_mouse_pos = Parse::toPoint(infile.val);
			}
			else if (infile.key == "icon_joystick") {
				// @ATTR icon_joystick|point|Position of the joystick icon.
				icon_joystick_pos = Parse::toPoint(infile.val);
			}
		}
		infile.close();
	}

	label_title.setText(msg->get("Select a Movement Type"));
	label_title.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
	label_config_hint.setText(msg->get("Can be changed later in the Configuration menu."));
	label_config_hint.setColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));

	button_keyboard->setLabel(msg->get("Keyboard"));
	button_mouse->setLabel(msg->get("Mouse"));
	button_joystick->setLabel(msg->get("Joystick"));

	tablist.ignore_no_mouse = true;
	tablist.add(button_keyboard);
	tablist.add(button_mouse);
	tablist.add(button_joystick);

	setBackground("images/menus/movement_type.png");

	// load icons
	Image *graphics;
	graphics = render_device->loadImage("images/menus/movement_type_keyboard.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		icon_keyboard = graphics->createSprite();
		graphics->unref();
	}
	graphics = render_device->loadImage("images/menus/movement_type_mouse.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		icon_mouse = graphics->createSprite();
		graphics->unref();
	}
	graphics = render_device->loadImage("images/menus/movement_type_joystick.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		icon_joystick = graphics->createSprite();
		graphics->unref();
	}

	// button tooltips
	button_keyboard->tooltip = msg->get("Keyboard") + "\n\n" + \
							   msg->get("Up") + ": " + inpt->getBindingString(Input::UP) + "\n" + \
							   msg->get("Left") + ": " + inpt->getBindingString(Input::LEFT) + "\n" + \
							   msg->get("Down") + ": " + inpt->getBindingString(Input::DOWN) + "\n" + \
							   msg->get("Right") + ": " + inpt->getBindingString(Input::RIGHT);

	button_mouse->tooltip = msg->get("Mouse") + "\n\n" + \
							msg->get("Button") + ": " + inpt->getBindingString(settings->mouse_move_swap ? Input::MAIN2 : Input::MAIN1);

	button_joystick->tooltip = msg->get("Joystick") + "\n\n" + \
							   msg->get("Up") + ": " + inpt->getGamepadBindingString(Input::UP) + "\n" + \
							   msg->get("Left") + ": " + inpt->getGamepadBindingString(Input::LEFT) + "\n" + \
							   msg->get("Down") + ": " + inpt->getGamepadBindingString(Input::DOWN) + "\n" + \
							   msg->get("Right") + ": " + inpt->getGamepadBindingString(Input::RIGHT);

	align();

	visible = false;
}

void MenuMovementType::align() {
	Menu::align();

	label_title.setPos(window_area.x, window_area.y);
	label_config_hint.setPos(window_area.x, window_area.y);

	button_keyboard->setPos(window_area.x, window_area.y);
	button_mouse->setPos(window_area.x, window_area.y);
	button_joystick->setPos(window_area.x, window_area.y);

	if (icon_keyboard)
		icon_keyboard->setDest(window_area.x + icon_keyboard_pos.x, window_area.y + icon_keyboard_pos.y);
	if (icon_mouse)
		icon_mouse->setDest(window_area.x + icon_mouse_pos.x, window_area.y + icon_mouse_pos.y);
	if (icon_joystick)
		icon_joystick->setDest(window_area.x + icon_joystick_pos.x, window_area.y + icon_joystick_pos.y);
}

void MenuMovementType::logic() {
	if (!visible)
		return;

	tablist.logic();

	button_joystick->enabled = (inpt->getNumJoysticks() > 0 && platform.config_input[Platform::Input::JOYSTICK]);

	if (button_keyboard->checkClick()) {
		settings->mouse_move = false;
		settings->enable_joystick = false;
		settings->joystick_device = -1;
		close();
	}
	else if (button_mouse->checkClick()) {
		settings->mouse_move = true;
		settings->no_mouse = false;
		settings->mouse_aim = true;
		settings->enable_joystick = false;
		settings->joystick_device = -1;
		close();
	}
	else if (button_joystick->checkClick()) {
		settings->mouse_move = false;
		settings->enable_joystick = true;
		if (settings->joystick_device == -1)
			settings->joystick_device = 0;
		close();
	}
}

void MenuMovementType::close() {
	visible = false;
	tablist.defocus();

	inpt->initJoystick();
	inpt->joysticks_changed = false;

	settings->move_type_dimissed = true;
	settings->saveSettings();
}

void MenuMovementType::render() {
	if (!visible)
		return;

	// background
	Menu::render();

	label_title.render();
	label_config_hint.render();

	button_keyboard->render();
	button_mouse->render();
	button_joystick->render();

	render_device->render(icon_keyboard);
	render_device->render(icon_mouse);
	render_device->render(icon_joystick);
}

MenuMovementType::~MenuMovementType() {
	delete button_keyboard;
	delete button_mouse;
	delete button_joystick;

	delete icon_keyboard;
	delete icon_mouse;
	delete icon_joystick;
}

