/*
 *  tracker/DialogLua.h
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
 *  DialogLua.h
 *  MilkyTracker
 *
 *  Created by coderofsalvation/Leon van Kammen 
 *
 */

#ifndef __DIALOGLUA_H__
#define __DIALOGLUA_H__

#include "DialogBase.h"
#include "Lua.h"
#include "SampleEditor.h"

class DialogLua : public PPDialogBase
{
public:

private:
	pp_uint32 numSliders;
	PPScreen* screen;
	DialogResponder *responder;
	pp_int32 id;
	SampleEditor *sampleEditor;
	bool needUpdate;
	bool preview;
	lua_State* L;
	bool ready;

	virtual pp_int32 handleEvent(PPObject* sender, PPEvent* event);	
	
	void resetSliders();
	void update();

public:
	DialogLua( PPScreen *screen, DialogResponder *responder, pp_int32 id, lua_State *L, PPString script );


	void setSlider(pp_uint32 index, float param);
	float getSlider(pp_uint32 index) const;

	lua_State * getLuaState(){ return L; }
	void initControls(PPScreen* screen, 
				   DialogResponder* responder,
				   pp_int32 id );

	void setSampleEditor(SampleEditor *s){ this->sampleEditor = s; }
	SampleEditor * getSampleEditor(){ return this->sampleEditor; }
	bool isReady(){ return ready; }

};

#endif
