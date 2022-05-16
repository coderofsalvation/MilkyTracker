#include "PatternStepSequencer.h"
#include "PatternEditor.h"
#include "PatternTools.h"

PatternStepSequencer::PatternStepSequencer(PatternEditor *editor ){
  patternEditor = editor;
}

PatternStepSequencer::~PatternStepSequencer( ){
}

pp_int32 PatternStepSequencer::noteToStep(pp_int32 note){
  pp_int32 step = -1;
  switch( note ){

    case 61: step = 0; break;
    case 63: step = 1; break;
    case 65: step = 2; break;
    case 66: step = 3; break;
    case 68: step = 4; break;
    case 70: step = 5; break;
    case 72: step = 6; break;
    case 73: step = 7; break;
    case 75: step = 8; break;
    case 77: step = 9; break;
    case 78: step = 10; break;
    case 80: step = 11; break;

    default: break;
  }
  return step;
}

void PatternStepSequencer::setStep(pp_uint32 step, pp_uint32 channel ){
  PatternTools patternTools;
  PatternEditorTools::Position cursor = patternEditor->getCursor();
  cursor.channel = channel;
  if( step != -1 ) cursor.row = step;
  cursor.inner = 0;
  patternEditor->setCursor(cursor);
  patternTools.setPosition( patternEditor->getPattern(), channel,step);
}
    
void PatternStepSequencer::writeStep(pp_int32 note, 
							  bool withUndo/* = false*/){
  PatternTools patternTools;
  pp_uint32 channel = patternEditor->getCurrentActiveInstrument()-1;
  mp_sint32 step = noteToStep(note);
  if( step >= 0 ){
    if (withUndo) patternEditor->prepareUndo();
    setStep( step, channel );
    mp_ubyte val = (mp_ubyte)(patternEditor->getPattern()->patternData[0]);
    printf("step = %i note %i val %i channel %i \n",step,note,val,channel);
    return;
    if( val == 0 ){ // no note present
      //patternTools.setNote(61);
      patternTools.setInstrument( channel+1 );
      patternEditor->writeNote(note,true);
    }else{
      //patternTools.setNote(0);
      patternTools.setInstrument( 0 );
      patternEditor->writeNote(0,true);
    }
		if (withUndo) patternEditor->finishUndo(patternEditor->LastChangeSlotChange);
  }else printf("nee\n");
}
