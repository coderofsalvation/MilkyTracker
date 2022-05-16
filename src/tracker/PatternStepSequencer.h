/*
 *  tracker/PatternStepSequencer.h
 *
 *  Copyright 2009 Peter Barth
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
 *  PatternEditor.h
 *  MilkyTracker
 *
 *  Created by Peter Barth on 16.11.07.
 *
 */

#ifndef __PATTERNSTEP_SEQUENCER_H__
#define __PATTERNSTEP_SEQUENCER_H__

#include "BasicTypes.h"

class PatternEditor;
class PatternEditorControl;
class PPGraphicsAbstract;

class PatternStepSequencer {

  private:
    PatternEditor *patternEditor;

  public:
    PatternStepSequencer( PatternEditor *editor );
    ~PatternStepSequencer();
    pp_int32 noteToStep(pp_int32 note);
    void writeStep(pp_int32 note, 
							  bool withUndo/* = false*/);
    void setStep(pp_uint32 step, pp_uint32 channel );

};

#endif
