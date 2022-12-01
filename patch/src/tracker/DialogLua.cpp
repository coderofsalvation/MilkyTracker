/*
 *  tracker/DialogLua.cpp
 *
 *  Copyright 2022 coderofsalvation/Leon van Kammen 
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  DialogLua.cpp
 *  MilkyTracker
 *
 *  Created by Leon van Kammen 
 *
 */

#include "Lua.h"
#include "DialogLua.h"
#include "Screen.h"
#include "StaticText.h"
#include "MessageBoxContainer.h"
#include "ScrollBar.h"
#include "Slider.h"
#include "Seperator.h"
#include "PPSystem.h"
#include "FilterParameters.h"

DialogLua::DialogLua(PPScreen *parentScreen, DialogResponder *toolHandlerResponder, int ID, lua_State *l, PPString script ){
	sampleEditor = NULL;
	needUpdate   = false;
	preview      = false;
	ready        = false;
	PPString config = System::getConfigFileName();
	config.append(".lua");

	L = l;
	ready = (L != NULL);
	if( ready = (L != NULL) ){
		lua_settop(L,0);
		Lua::initDSP(L,44100,0);
		if( !Lua::initSYN(L,config.getStrBuffer())                 ) ready = false;
		if( ready && luaL_loadstring( L, script.getStrBuffer() ) != 0 ) ready = false;
		if( !ready ){
			printf("[luadialog:error]\n");
			Lua::printError(L);
		}else lua_call(L, 0, 0);
	}
	if( ready ) initControls(parentScreen, toolHandlerResponder, ID );

}

void DialogLua::initControls(PPScreen* screen, 
				   DialogResponder* responder,
				   pp_int32 id )
{
	const pp_uint32 tickSize = 4;
	
	PPString title  = PPString( Lua::getTableString(L,"dialog","title","LUA dialog") );
	float dheight   = (int)Lua::getNumber(L,"height",230);
	title.toUpper();
	initDialog(screen, responder, id, title.getStrBuffer(), 300, dheight, 26, "Ok", "Cancel");
	
	pp_int32 x      = getMessageBoxContainer()->getLocation().x;
	pp_int32 y      = getMessageBoxContainer()->getLocation().y;
	pp_int32 width  = getMessageBoxContainer()->getSize().width;
	pp_int32 height = getMessageBoxContainer()->getSize().height;
	pp_uint32 borderSpace = 12;
	pp_uint32 scalaSpace = 16*5+8;
	pp_int32 y2 = getMessageBoxContainer()->getControlByID(MESSAGEBOX_STATICTEXT_MAIN_CAPTION)->getLocation().y + 20;
	pp_int32 x2 = x + borderSpace;
	pp_int32 size = width-120;
	
	// get sliders
	pp_uint32 i = 0;
	LUA_FOREACH("sliders")
		const char *key   = lua_tostring(L, -1);
		const char *value = lua_tostring(L, -2);
		char *label = Lua::getString(L,"label","");
		float min = Lua::getNumber(L,"min",0.0f);
		float max = Lua::getNumber(L,"max",0.0f);
		float val = Lua::getNumber(L,"val",0.0f);
		PPSlider* slider = new PPSlider(MESSAGEBOX_CONTROL_USER1+i, screen, this, PPPoint(x2+scalaSpace, y2), size, true, false);
		slider->setBarSize(8192);
		slider->setMinValue((int)min);
		slider->setMaxValue((int)max);
		slider->setCurrentValue((int)val);
		getMessageBoxContainer()->addControl(slider);
		PPFont* font = PPFont::getFont(PPFont::FONT_SYSTEM);
		PPStaticText* staticText = new PPStaticText(0, screen, this, PPPoint(x2+(SCROLLBUTTONSIZE/2), y2), label, true);
		staticText->setFont(font);
		getMessageBoxContainer()->addControl(staticText);

		y2 += SCROLLBUTTONSIZE + 6;
		i++;
	LUA_FOREACH_END
	numSliders = i;
}

void DialogLua::update()
{
	parentScreen->paintControl(messageBoxContainerGeneric);
	needUpdate = false;
}

pp_int32 DialogLua::handleEvent(PPObject* sender, PPEvent* event)
{
	char s[255];
	pp_uint32 id = reinterpret_cast<PPControl*>(sender)->getID();
	if( id >= MESSAGEBOX_CONTROL_USER1 && id <= MESSAGEBOX_CONTROL_USER1+numSliders ){
		pp_uint32 slider = id-MESSAGEBOX_CONTROL_USER1;
		float val = getSlider( slider );
		sprintf(s,"sliders[%i].val=%f",slider+1,val);
		if( luaL_loadstring( L, s ) == 0 ) lua_call(L, 0, 0);
		needUpdate = true;
	}

	if( event->getID() == eCommand ){
		if( preview && sampleEditor != NULL ) sampleEditor->undo();
	}
	if( event->getID() == eLMouseUp && needUpdate ){
		if( sampleEditor != NULL ){
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter( (void *)L ) );
			par.setParameter(1, FilterParameters::Parameter( Lua::getTableString(L,"dialog","func","nil") ) );
			if( preview ) sampleEditor->undo();
			sampleEditor->tool_luaSample(&par);
			preview = true;
		}
		update();
	}
	return PPDialogBase::handleEvent(sender, event);
}

void DialogLua::setSlider(pp_uint32 index, float param)
{
	if (index >= numSliders)
		return;
	PPSlider* slider = static_cast<PPSlider*>(getMessageBoxContainer()->getControlByID(MESSAGEBOX_CONTROL_USER1+index));
	pp_int32 value = (pp_int32)param;
	slider->setCurrentValue(value);
}

float DialogLua::getSlider(pp_uint32 index) const
{
	if (index >= numSliders)
		return 0.0f;
	PPSlider* slider = static_cast<PPSlider*>(getMessageBoxContainer()->getControlByID(MESSAGEBOX_CONTROL_USER1+index));
	float v = slider->getCurrentValue();
	return v;
}
