/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "common/system.h"


#include "xeen/game.h"

#include "xeen/graphics/font.h"
#include "xeen/graphics/imagebuffer.h"
#include "xeen/graphics/sprite.h"
#include "xeen/graphics/spritemanager.h"

#include "xeen/ui/window.h"

XEEN::Window::Window(const Common::Rect& area) : _area(area), _pressedButton(0), _pressedTime(0)
{
}

void XEEN::Window::draw(ImageBuffer& out)
{
    const Common::Point location(_area.left, _area.top);

    // Background: TODO: Get correct color; draw border.    
    out.fillRect(_area, 21);
    
    // Draw buttons
    SpriteManager& sprites = XEENgame.getSpriteManager();
    
    for(const Button* button = getButtons(); button && button->sprite; button ++)
    {
        Sprite* const icon = sprites.getSprite(button->sprite);
        
        if(enforce(icon))
        {
            const uint32 frame = (button == _pressedButton) ? button->pressedFrame : button->normalFrame;
            icon->drawCell(out, location + button->area, frame);
        }
    }
    
    // Draw strings
    Font& font = XEENgame.getFont();
    
    for(const String* string = getStrings(); string && (string->text || string->stringID); string ++)
    {
        const char* text = string->text ? string->text : produceString(string->stringID);
        
        if(text)
        {
            font.drawString(out, location + Common::Point(string->x, string->y), text);
        }
    }
}

void XEEN::Window::heartbeat()
{
    if(_pressedButton && (g_system->getMillis() >= _pressedTime))
    {
        handleAction(_pressedButton->actionID);
        _pressedButton = 0;
    }
}

bool XEEN::Window::click(const Common::Point& point)
{
    // Check buttons    
    for(const Button* button = getButtons(); button && button->sprite; button ++)
    {
        const Common::Rect r = button->area;
    
        if(r.contains(point))
        {
            _pressedButton = button;
            _pressedTime = g_system->getMillis() + BUTTON_DELAY;
            return true;
        }
    }
    
    return false;
}

XEEN::CharacterStatusWindow::CharacterStatusWindow() : Window(Common::Rect(0, 0, 320, 146))
{
}

            
const XEEN::Button* XEEN::CharacterStatusWindow::getButtons() const
{
    static const Button buttons[] = 
    {
        {"VIEW.ICN",  0,  1, { 10,  24, 24, 20},  0}, {"VIEW.ICN", 10, 11, { 61,  24, 24, 20},  1},
        {"VIEW.ICN", 20, 21, {112,  24, 24, 20},  2}, {"VIEW.ICN", 30, 31, {177,  24, 24, 20},  3},
        {"VIEW.ICN",  2,  3, { 10,  47, 24, 20},  4}, {"VIEW.ICN", 12, 13, { 61,  47, 24, 20},  5},
        {"VIEW.ICN", 22, 23, {112,  47, 24, 20},  6}, {"VIEW.ICN", 32, 33, {177,  47, 24, 20},  7},
        {"VIEW.ICN",  4,  5, { 10,  70, 24, 20},  8}, {"VIEW.ICN", 14, 15, { 61,  70, 24, 20},  9},
        {"VIEW.ICN", 24, 25, {112,  70, 24, 20}, 10}, {"VIEW.ICN", 34, 35, {177,  70, 24, 20}, 11},
        {"VIEW.ICN",  6,  7, { 10,  93, 24, 20}, 12}, {"VIEW.ICN", 16, 17, { 61,  93, 24, 20}, 13},
        {"VIEW.ICN", 26, 27, {112,  93, 24, 20}, 14}, {"VIEW.ICN", 36, 37, {177,  93, 24, 20}, 15},
        {"VIEW.ICN",  8,  9, { 10, 116, 24, 20}, 16}, {"VIEW.ICN", 18, 19, { 61, 116, 24, 20}, 17},
        {"VIEW.ICN", 28, 29, {112, 116, 24, 20}, 18}, {"VIEW.ICN", 38, 39, {177, 116, 24, 20}, 19},

        {"VIEW.ICN", 40, 41, {285,  11, 24, 20}, 20}, {"VIEW.ICN", 42, 43, {285,  43, 24, 20}, 21},
        {"VIEW.ICN", 44, 45, {285,  75, 24, 20}, 22}, {"VIEW.ICN", 46, 47, {285, 107, 24, 20}, 23}, 

        {(uint16)0, 0, 0, {0, 0, 0, 0}, 0}
    };
    
    return buttons;
}

const XEEN::String* XEEN::CharacterStatusWindow::getStrings() const
{
    static const String strings[] = 
    {
        {"Mgt", 0, 37,  26}, {"Acy", 0, 88,  26}, {"H.P.",   0, 139,  26}, {"Experience", 0, 204,  26},
        {"Int", 0, 37,  49}, {"Lck", 0, 88,  49}, {"S.P.",   0, 139,  49}, {"Party Gold", 0, 204,  49},
        {"Per", 0, 37,  70}, {"Age", 0, 88,  70}, {"Resis",  0, 139,  70}, {"Party Gems", 0, 204,  70},
        {"End", 0, 37,  93}, {"Lvl", 0, 88,  93}, {"Skills", 0, 139,  93}, {"Party Food", 0, 204,  93},
        {"Spd", 0, 37, 116}, {"AC",  0, 88, 116}, {"Awrds",  0, 139, 116}, {"Condition",  0, 204, 116},

        {(uint16)0, 0, 0, 0}
    };
    
    return strings;
}

void XEEN::CharacterStatusWindow::handleAction(unsigned id)
{
    XEEN_VALID();

    if(id == 23 && valid(XEENgame))
    {
        XEENgame.showWindow(0);
    }
}

const char* XEEN::CharacterStatusWindow::produceString(unsigned id)
{
    snprintf(stringBuffer, sizeof(stringBuffer), "%d", id);
    return stringBuffer;
}

