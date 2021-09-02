/*
 *  tracker/SampleEditor.cpp
 *
 *  Copyright 2009 Peter Barth
 * 	
 *  tool_vocodeSample(): GPL /Copyright 2008-2011 David Robillard <http://drobilla.net>
 *	tool_vocodeSample(): GPL /Copyright 1999-2000 Paul Kellett (Maxim Digital Audio)
 *	
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
 *  SampleEditor.cpp
 *  MilkyTracker
 *
 *  Created by Peter Barth on 22.11.07.
 *
 */

#include "SampleEditor.h"
#include "SimpleVector.h"
#include "XModule.h"
#include "VRand.h"
#include "Equalizer.h"
#include "EQConstants.h"
#include "FilterParameters.h"
#include "SampleEditorResampler.h"
#include "Tracker.h"
#include "SoundGenerator.h"
#include <time.h>

SampleEditor::ClipBoard::ClipBoard() :
		buffer(NULL)
{
}

SampleEditor::ClipBoard::~ClipBoard()
{
	delete[] buffer;
}
		
void SampleEditor::ClipBoard::makeCopy(TXMSample& sample, XModule& module, pp_int32 selectionStart, pp_int32 selectionEnd, bool cut/* = false*/)
{
	if (selectionEnd < 0)
		return;
	
	if (selectionStart < 0)
		selectionStart = 0;
		
	if (selectionEnd > (signed)sample.samplen)
		selectionEnd = sample.samplen;

	if (selectionEnd < selectionStart)
	{
		pp_int32 s = selectionEnd; selectionEnd = selectionStart; selectionStart = s;
	}
	
	this->selectionStart = selectionStart;
	this->selectionEnd = selectionEnd;
		
	this->selectionWidth = abs(selectionEnd - selectionStart); 
		
	if (selectionWidth == 0)
		return;

	if (buffer)
		delete[] buffer;	
		
	numBits = (sample.type & 16) ? 16 : 8;
	
	// 16 bit sample
	if (numBits == 16)
	{
		buffer = (mp_sbyte*)(new mp_sword[selectionWidth+1]);
		
		mp_sword* dstptr = (mp_sword*)buffer;
		for (pp_int32 i = selectionStart; i <= selectionEnd; i++)
			*dstptr++ = sample.getSampleValue(i);
	}
	// 8 bit sample
	else if (numBits == 8)
	{
		buffer = new mp_sbyte[selectionWidth+1];

		mp_sbyte* dstptr = (mp_sbyte*)buffer;
		for (pp_int32 i = selectionStart; i <= selectionEnd; i++)
			*dstptr++ = sample.getSampleValue(i);
	}
	else ASSERT(false);
}

void SampleEditor::ClipBoard::paste(TXMSample& sample, XModule& module, pp_int32 pos)
{
	if (pos < 0)
		pos = 0;

	if (sample.sample == NULL)
	{
		sample.samplen = 0;
		pos = 0;
	}

	pp_int32 newSampleSize = sample.samplen + selectionWidth;
	pp_int32 i;
	
	// 16 bit sample
	if (sample.type & 16)
	{
		mp_sword* newBuffer = (mp_sword*)module.allocSampleMem(newSampleSize*2);
		
		// copy stuff before insert start point
		for (i = 0;  i < pos; i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i, sample.getSampleValue(i));
		
		// copy selection to start point
		for (i = 0; i < selectionWidth; i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i+pos, getSampleWord(i));
			
		// copy stuff after insert start point
		for (i = 0;  i < ((signed)sample.samplen - pos); i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i+pos+selectionWidth, sample.getSampleValue(i+pos));
	
		if (sample.sample)
			module.freeSampleMem((mp_ubyte*)sample.sample);
		
		sample.sample = (mp_sbyte*)newBuffer;
	}
	else
	{
		mp_sbyte* newBuffer = (mp_sbyte*)module.allocSampleMem(newSampleSize);
		
		// copy stuff before insert start point
		for (i = 0;  i < pos; i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i, sample.getSampleValue(i));
		
		// copy selection to start point
		for (i = 0; i < selectionWidth; i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i+pos, getSampleByte(i));
			
		// copy stuff after insert start point
		for (i = 0;  i < ((signed)sample.samplen - pos); i++)
			sample.setSampleValue((mp_ubyte*)newBuffer, i+pos+selectionWidth, sample.getSampleValue(i+pos));

		if (sample.sample)
			module.freeSampleMem((mp_ubyte*)sample.sample);

		sample.sample = newBuffer;
	}

	pp_int32 loopend = sample.loopstart + sample.looplen;

	if ((signed)sample.loopstart < pos && loopend > pos)
		loopend+=selectionWidth;
	else if ((signed)sample.loopstart > pos && loopend > pos)
	{
		sample.loopstart+=selectionWidth;
		loopend+=selectionWidth;
	}

	sample.samplen = newSampleSize;

	sample.looplen = loopend - sample.loopstart;

}

void SampleEditor::prepareUndo()
{
	delete before; 
	before = NULL; 
		
	if (undoStackEnabled && undoStackActivated && undoStack) 
	{
		undoUserData.clear();
		notifyListener(NotificationFeedUndoData);

		before = new SampleUndoStackEntry(*sample, 
										  getSelectionStart(), 
										  getSelectionEnd(), 
										  &undoUserData);
	}
}

void SampleEditor::finishUndo()
{
	if (undoStackEnabled && undoStackActivated && undoStack) 
	{ 
		// first of all the listener should get the chance to adjust
		// user data according to our new changes BEFORE we actually save
		// the new state in the undo stack for redo
		lastOperationDidChangeSize = (sample->samplen != before->getSampLen());					
		notifyListener(NotificationChangesValidate);
		
		undoUserData.clear();
		// we want some user data now
		notifyListener(NotificationFeedUndoData);

		SampleUndoStackEntry after(SampleUndoStackEntry(*sample, 
										 getSelectionStart(), 
										 getSelectionEnd(), 
										 &undoUserData)); 
		if (*before != after) 
		{ 
			if (undoStack) 
			{ 
				undoStack->Push(*before); 
				undoStack->Push(after); 
				undoStack->Pop(); 
			} 
		} 
	} 
	
	// we're done, client might want to refresh the screen or whatever
	notifyListener(NotificationChanges);			
}
	
bool SampleEditor::revoke(const SampleUndoStackEntry* stackEntry)
{
	if (sample == NULL)
		return false;
	 if (undoStack == NULL || !undoStackEnabled)
		return false;
		
	sample->samplen = stackEntry->getSampLen();
	sample->loopstart = stackEntry->getLoopStart(); 
	sample->looplen = stackEntry->getLoopLen(); 
	sample->relnote = stackEntry->getRelNote(); 
	sample->finetune = stackEntry->getFineTune(); 
	sample->type = (mp_ubyte)stackEntry->getFlags();
	
	setSelectionStart(stackEntry->getSelectionStart());
	setSelectionEnd(stackEntry->getSelectionEnd());
	
	enterCriticalSection();
	
	// free old sample memory
	if (sample->sample)
	{
		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = NULL;
	}
	
	if (stackEntry->getBuffer())
	{			
		if (sample->type & 16)
		{
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen*2);
			TXMSample::copyPaddedMem(sample->sample, stackEntry->getBuffer(), sample->samplen*2);
		}
		else
		{
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen);
			TXMSample::copyPaddedMem(sample->sample, stackEntry->getBuffer(), sample->samplen);
		}
	}
	
	leaveCriticalSection();
	undoUserData = stackEntry->getUserData();
	notifyListener(NotificationFetchUndoData);
	notifyListener(NotificationChanges);
	return true;
}

void SampleEditor::notifyChanges(bool condition, bool lazy/* = true*/)
{
	lastOperation = OperationRegular;	
	lastOperationDidChangeSize = false;
	if (!lazy)
	{
		setLazyUpdateNotifications(false);
		notifyListener(NotificationChangesValidate);
		notifyListener(NotificationChanges);
	}
	else
	{
		setLazyUpdateNotifications(true);
		notifyListener(NotificationChanges);
		setLazyUpdateNotifications(false);
	}
}

SampleEditor::SampleEditor() :
	EditorBase(),
	sample(NULL),
	undoStackEnabled(true), 
	undoStackActivated(true),	
	before(NULL),
	undoStack(NULL),
	lastOperationDidChangeSize(false),
	lastOperation(OperationRegular),
	drawing(false),
	lastSamplePos(-1),
	lastParameters(NULL),
	lastFilterFunc(NULL)
{
	// Undo history
	undoHistory = new UndoHistory<TXMSample, SampleUndoStackEntry>(UNDOHISTORYSIZE_SAMPLEEDITOR);
	
	resetSelection();

	memset(&lastSample, 0, sizeof(lastSample));
}

SampleEditor::~SampleEditor()
{
	delete lastParameters;
	delete undoHistory;
	delete undoStack;
	delete before;
}

void SampleEditor::attachSample(TXMSample* sample, XModule* module) 
{
	// only return if the sample data really equals what we already have
	if (sample->equals(lastSample) && sample == this->sample)
		return;

	lastSample = *sample;

	// --------- update undo history information --------------------	
	if (undoStackEnabled && undoStackActivated)
	{
		if (undoStack)
		{	
			// if the undo stack is empty, we don't need to save current undo stack
			if (!undoStack->IsEmpty() || !undoStack->IsTop())
			{	
				undoStack = undoHistory->getUndoStack(sample, this->sample, undoStack);
			}
			// delete it if it's empty
			else
			{
				delete undoStack;
				undoStack = NULL;
				
				undoStack = undoHistory->getUndoStack(sample, NULL, NULL);
			}
		}
		
		// couldn't get any from history, create new one
		if (!undoStack)
		{
			undoStack = new PPUndoStack<SampleUndoStackEntry>(UNDODEPTH_SAMPLEEDITOR);
		}
	}

	this->sample = sample;
	attachModule(module);

	resetSelection();
	
	notifyListener(NotificationReload);
}

void SampleEditor::reset()
{
	if (undoStackEnabled)
	{
		if (undoHistory)
			delete undoHistory;
		
		if (undoStack)
		{
			delete undoStack;
			undoStack = NULL;
			undoStack = new PPUndoStack<SampleUndoStackEntry>(UNDODEPTH_SAMPLEEDITOR);
		}
		
		undoHistory = new UndoHistory<TXMSample, SampleUndoStackEntry>(UNDOHISTORYSIZE_SAMPLEEDITOR);
	}
	else
	{
		if (undoHistory)
		{
			delete undoHistory;
			undoHistory = NULL;	
		}
		
		if (undoStack)
		{
			delete undoStack;
			undoStack = NULL;
		}
	}
}

bool SampleEditor::isEmptySample() const  
{
	if (!isValidSample())
		return true;
	
	return (sample->sample == NULL);
}

bool SampleEditor::canMinimize() const
{
	if (!isValidSample())
		return false;
	
	return sample->samplen && sample->sample && (sample->type & 3);
}

bool SampleEditor::isEditableSample() const
{
	if (!isValidSample())
		return false;
	
	return (sample->sample != NULL) && (sample->samplen != 0);
}

void SampleEditor::enableUndoStack(bool enable)
{
	undoStackEnabled = enable;
	reset();
}

bool SampleEditor::undo()
{
	if (!undoStackEnabled || undoStack == NULL) return false;
	if (undoStack->IsEmpty()) return false;
	return revoke(undoStack->Pop());
}

bool SampleEditor::redo()
{
	if (!undoStackEnabled || undoStack == NULL) return false;
	if (undoStack->IsTop()) return false;
	return revoke(undoStack->Advance());
}

void SampleEditor::selectAll()
{
	if (isEmptySample())
		return;

	selectionStart = 0;
	selectionEnd = sample->samplen;
}

void SampleEditor::loopRange()
{
	if (!hasValidSelection())
		return;

	// If a loop type is not enabled, set loop to Forward.
	// - Changes loop type to Forward when loop type is set to One shot
	// 	 and the start of the selection is not at the start of the sample, 
	// 	 but so does dragging the start of the loop. 
	if (!getLoopType())
		setLoopType(1);

	// Once loop is enabled, set the loop start/end points to selection start/end points.
	setRepeatStart(getSelectionStart());
	setRepeatEnd(getSelectionEnd());

	// Doesn't currently have undo or have the sample do the new loop 
	// until it retriggers, but neither does dragging the loop points.
}

bool SampleEditor::validate()
{
	if (isEmptySample())
	{
		resetSelection();
		return false;
	}

	pp_int32 sStart = getSelectionStart();
	pp_int32 sEnd = getSelectionEnd();
	if (sEnd < sStart)
	{
		pp_int32 s = sEnd; sEnd = sStart; sStart = s;
	}
	setSelectionStart(sStart);
	setSelectionEnd(sEnd);

	if (getSelectionEnd() != -1 && getSelectionStart () != -1 &&
		getSelectionEnd() < 0)
	{
		resetSelection();
	}

	if (getSelectionEnd() > 0 && getSelectionStart() < 0)
		setSelectionStart(0);

	if (getSelectionStart() > (signed)sample->samplen)
	{
		resetSelection();
	}
	if (getSelectionEnd() > (signed)sample->samplen)
	{
		setSelectionEnd(sample->samplen);
	}
	
	if (sample->loopstart > sample->samplen)
		sample->loopstart = 0;
	if (sample->loopstart + sample->looplen > sample->samplen)
		sample->looplen -= (sample->loopstart + sample->looplen) - sample->samplen;
	
	// one shot sample only allows loopstart == 0
	if ((sample->type & 32) && sample->loopstart)
	{
		sample->type &= ~32;
	}
	return true;
}

bool SampleEditor::canPaste() const
{ 
	if (selectionEnd == selectionStart &&
		selectionStart == -1 &&
		sample->sample != NULL)
		return false;

	return !ClipBoard::getInstance()->isEmpty(); 
}

pp_uint32 SampleEditor::getRepeatStart() const
{
	return sample == NULL ? 0 : sample->loopstart;
}

pp_uint32 SampleEditor::getRepeatEnd() const
{
	return sample == NULL ? 0 : sample->loopstart + sample->looplen;
}

pp_uint32 SampleEditor::getRepeatLength() const
{
	return sample == NULL ? 0 : sample->looplen;
}

void SampleEditor::setRepeatStart(pp_uint32 start)
{
	if (sample == NULL)
		return;

	mp_uint32 before = sample->loopstart;		
		
	sample->loopstart = start;

	validate();

	notifyChanges(before != sample->loopstart, false);
}

void SampleEditor::setRepeatEnd(pp_uint32 end)
{
	if (sample == NULL)
		return;
		
	mp_uint32 before = sample->looplen;	
		
	sample->looplen = (end - sample->loopstart);
	
	validate();

	notifyChanges(before != sample->looplen, false);
}

void SampleEditor::setRepeatLength(pp_uint32 length)
{
	if (sample == NULL)
		return;

	mp_uint32 before = sample->looplen;	
		
	sample->looplen = length;	

	validate();

	notifyChanges(before != sample->looplen, false);
}

bool SampleEditor::increaseRepeatStart()
{
	if (isEmptySample())
		return false;
	
	mp_uint32 before = sample->loopstart;
	
	pp_int32 loopend = sample->loopstart+sample->looplen;
	pp_int32 loopstart = sample->loopstart+1;
	if (loopstart >= 0 && loopstart < loopend && loopend >= 0 && loopend <= (signed)sample->samplen)
	{
		sample->looplen = loopend - loopstart;
		sample->loopstart = loopstart;
	}
	
	validate();

	notifyChanges(before != sample->loopstart, false);

	return true;
}

bool SampleEditor::decreaseRepeatStart()
{
	if (isEmptySample())
		return false;

	mp_uint32 before = sample->loopstart;

	pp_int32 loopend = sample->loopstart+sample->looplen;
	pp_int32 loopstart = sample->loopstart-1;
	if (loopstart >= 0 && loopstart < loopend && loopend >= 0 && loopend <= (signed)sample->samplen)
	{
		sample->looplen = loopend - loopstart;
		sample->loopstart = loopstart;
	}

	validate();

	notifyChanges(before != sample->loopstart, false);

	return true;
}

bool SampleEditor::increaseRepeatLength()
{
	if (isEmptySample())
		return false;

	mp_uint32 before = sample->looplen;
	
	pp_int32 loopend = sample->loopstart+sample->looplen+1;
	pp_int32 loopstart = sample->loopstart;
	if (loopstart >= 0 && loopstart < loopend && loopend >= 0 && loopend <= (signed)sample->samplen)
	{
		sample->looplen = loopend - loopstart;
		sample->loopstart = loopstart;
	}

	validate();

	notifyChanges(before != sample->looplen, false);

	return true;
}

bool SampleEditor::decreaseRepeatLength()
{
	if (isEmptySample())
		return false;

	mp_uint32 before = sample->looplen;

	pp_int32 loopend = sample->loopstart+sample->looplen-1;
	pp_int32 loopstart = sample->loopstart;
	if (loopstart >= 0 && loopstart < loopend && loopend >= 0 && loopend <= (signed)sample->samplen)
	{
		sample->looplen = loopend - loopstart;
		sample->loopstart = loopstart;
	}
	
	validate();
	
	notifyChanges(before != sample->looplen, false);

	return true;
}

bool SampleEditor::setLoopType(pp_uint8 type)
{
	if (sample == NULL)
		return false;

	mp_ubyte before = sample->type;

	if (type <= 2)
	{
		sample->type &= ~(3+32);
		sample->type |= type;
		
		if (type && 
			sample->loopstart == 0 && 
			sample->looplen == 0)
		{
			sample->loopstart = 0;
			sample->looplen = sample->samplen;
		}
	}
	else if (type == 3)
	{
		sample->type &= ~(3+32);
		sample->type |= (1+32);
		mp_sint32 loopend = sample->loopstart + sample->looplen;
		sample->loopstart = 0;
		sample->looplen = loopend;
	}
	else ASSERT(false);
	
	notifyChanges(before != sample->type);

	return true;
}

pp_uint8 SampleEditor::getLoopType() const
{ 
	if (sample) 
	{
		if ((sample->type & 3) == 1 && (sample->type & 32))
			return 3;
		else
			return sample->type & 3;
	}
	else 
		return 0; 
}

bool SampleEditor::is16Bit() const
{ 
	if (sample) 
		return (sample->type & 16) == 16;
	else 
		return false; 
}

pp_int32 SampleEditor::getRelNoteNum() const
{
	return sample ? sample->relnote : 0;
}

void SampleEditor::setRelNoteNum(pp_int32 num)
{
	if (sample == NULL)
		return;
	mp_sbyte before = sample->relnote;

	pp_int32 relnote = sample->relnote;
	relnote = num;
	if (relnote > 71)
		relnote = 71;
	if (relnote < -48)
		relnote = -48;
	sample->relnote = (mp_sbyte)relnote;

	notifyChanges(sample->relnote != before);
}


void SampleEditor::increaseRelNoteNum(pp_int32 offset)
{
	if (sample == NULL)
		return;
		
	mp_sbyte before = sample->relnote;
		
	pp_int32 relnote = sample->relnote;
	relnote+=offset;
	if (relnote > 71)
		relnote = 71;
	if (relnote < -48)
		relnote = -48;
	sample->relnote = (mp_sbyte)relnote;

	notifyChanges(sample->relnote != before);
}

pp_int32 SampleEditor::getFinetune() const
{
	return sample ? sample->finetune : 0;
}

void SampleEditor::setFinetune(pp_int32 finetune)
{
	if (sample == NULL)
		return;

	mp_sbyte before = sample->finetune;

	if (finetune < -128)
		finetune = -128;
	if (finetune > 127)
		finetune = 127; 

	sample->finetune = (mp_sbyte)finetune;

	notifyChanges(sample->finetune != before);
}

void SampleEditor::setFT2Volume(pp_int32 vol)
{
	if (sample == NULL)
		return;

	mp_ubyte before = sample->vol;

	sample->vol = XModule::vol64to255(vol);
	
	notifyChanges(sample->vol != before);
}

pp_int32 SampleEditor::getFT2Volume() const
{
	return sample ? XModule::vol255to64(sample->vol) : 0;
}

void SampleEditor::setPanning(pp_int32 pan)
{
	if (sample == NULL)
		return;

	mp_sbyte before = sample->pan;

	if (pan < 0) pan = 0;
	if (pan > 255) pan = 255;
	sample->pan = (mp_sbyte)pan;
	
	notifyChanges(sample->pan != before);
}

pp_int32 SampleEditor::getPanning() const
{
	return sample ? sample->pan : 0;
}

void SampleEditor::startDrawing()
{
	if (sample)
		sample->restoreOriginalState();

	drawing = true;
	lastSamplePos = -1;
	prepareUndo();
}

void SampleEditor::drawSample(pp_int32 sampleIndex, float s)
{
	s*=2.0f;

	pp_int32 from = lastSamplePos;
	pp_int32 to = sampleIndex;
	if (from == -1)
		from = sampleIndex;

	float froms = 0.0f;
	froms = getFloatSampleFromWaveform(from);

	if (from > to)
	{
		pp_int32 h = from; from = to; to = h;
		float fh = froms; froms = s; s = fh;
	}
	
	float step = 0;
	if (to-from)
		step = (s-froms)/(to-from);
	else
		froms = s;
	
	lastSamplePos = sampleIndex;

	for (pp_int32 si = from; si <= to; si++)
	{
		setFloatSampleInWaveform(si, froms);
		froms+=step;
	}	
}

void SampleEditor::endDrawing()
{
	drawing = false;
	lastSamplePos = -1;
	if (!sample || !sample->sample || !sample->samplen)
		return;
	
	lastOperation = OperationRegular;
	finishUndo();
}

void SampleEditor::minimizeSample()
{
	FilterParameters par(0);
	tool_minimizeSample(&par);
}

void SampleEditor::cropSample()
{
	FilterParameters par(0);
	tool_cropSample(&par);
}

void SampleEditor::clearSample()
{
	FilterParameters par(0);
	tool_clearSample(&par);
}

void SampleEditor::mixSpreadPasteSample()
{
	FilterParameters par(1);
	par.setParameter(0, FilterParameters::Parameter(0) ); // spreads selection across sample (changes pitch)
	tool_mixPasteSample(&par);
}

void SampleEditor::mixPasteSample()
{
	FilterParameters par(1);
	par.setParameter(0, FilterParameters::Parameter(1) ); // paste's selection on top new selection start (preserve pitch)
	tool_mixPasteSample(&par);
}

void SampleEditor::mixOverflowPasteSample()
{
	FilterParameters par(1);
	par.setParameter(0, FilterParameters::Parameter(2)); // paste's selection on top new selection start (preserves pitch + overflow) 
	tool_mixPasteSample(&par);
}

void SampleEditor::substractSample()
{
	FilterParameters par(0);
	tool_substractSample(&par);
}

void SampleEditor::AMPasteSample()
{
	FilterParameters par(0);
	tool_AMPasteSample(&par);
}

void SampleEditor::FMPasteSample()
{
	FilterParameters par(0);
	tool_FMPasteSample(&par);
}

void SampleEditor::PHPasteSample()
{
	FilterParameters par(0);
	tool_PHPasteSample(&par);
}

void SampleEditor::FLPasteSample()
{
	FilterParameters par(0);
	tool_FLPasteSample(&par);
}

void SampleEditor::ReverberateSample()
{
	FilterParameters par(0);
	tool_reverberateSample(&par);
}

void SampleEditor::testSample()
{
	FilterParameters par(0);
	tool_testSample(&par);
}

void SampleEditor::seamlessLoopSample()
{
	FilterParameters par(0);
	tool_seamlessLoopSample(&par);
}

void SampleEditor::convertSampleResolution(bool convert)
{
	FilterParameters par(1);
	par.setParameter(0, FilterParameters::Parameter(convert ? 1 : 0));
	tool_convertSampleResolution(&par);
}

bool SampleEditor::cutSampleInternal()
{
	if (sample == NULL)
		return false;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	if (sStart >= 0 && sEnd >= 0)
	{		
		if (sEnd < sStart)
		{
			pp_int32 s = sEnd; sEnd = sStart; sStart = s;
		}
	}
	else return false;
	
	selectionStart = sStart;
	selectionEnd = sEnd;

	if (sStart == sEnd)
		return false;

	// reset loop area double buffer to original sample state
	// (switch buffering off)
	sample->restoreOriginalState();

	for (pp_uint32 i = selectionEnd; i <= sample->samplen; i++)
		sample->setSampleValue((i-selectionEnd)+selectionStart, sample->getSampleValue(i));

	mp_sint32 sLoopStart = sample->loopstart;
	if (sEnd < (signed)sample->loopstart + (signed)sample->looplen)
		sLoopStart-=(sEnd - sStart);
	if (sLoopStart < 0)
		sLoopStart = 0;
	
	sample->loopstart = sLoopStart;
	sample->samplen -= abs(selectionEnd - selectionStart);
	
	return true;
}

void SampleEditor::cut()
{
	if (sample == NULL)
		return;

	if (!hasValidSelection())
		return;

	// we're going to change the sample buffers, better stop
	enterCriticalSection();
	// undo stuff going on
	prepareUndo();

	// store selection into clipboard
	ClipBoard::getInstance()->makeCopy(*sample, *module, getSelectionStart(), getSelectionEnd());

	// just make clear what kind of an operation this is
	if (cutSampleInternal())
		lastOperation = OperationCut;
	
	// selection no longer intact
	resetSelection();
	
	// validate our internal state
	validate();	
	// redo stuff and client notifications
	finishUndo();
	// keep on playing if you did
	leaveCriticalSection();
}

void SampleEditor::copy()
{
	if (sample == NULL)
		return;

	if (!hasValidSelection())
		return;

	ClipBoard::getInstance()->makeCopy(*sample, *module, getSelectionStart(), getSelectionEnd());

	notifyListener(NotificationUpdateNoChanges);
}

void SampleEditor::paste()
{
	if (sample == NULL)
		return;

	enterCriticalSection();

	prepareUndo();

	if (hasValidSelection())
	{
		mp_uint32 loopstart = sample->loopstart;
		mp_uint32 looplen = sample->looplen;
		if (cutSampleInternal())
			lastOperation = OperationCut;
		sample->loopstart = loopstart;
		sample->looplen = looplen;
	}

	ClipBoard::getInstance()->paste(*sample, *module, getSelectionStart());

	setSelectionEnd(getSelectionStart() + ClipBoard::getInstance()->getWidth());

	validate();	
	finishUndo();

	leaveCriticalSection();
}

SampleEditor::WorkSample* SampleEditor::createWorkSample(pp_uint32 size, pp_uint8 numBits, pp_uint32 sampleRate)
{
	WorkSample* workSample = new WorkSample(*module, size, numBits, sampleRate);
	if (workSample->buffer == NULL)
	{
		delete workSample;
		return NULL;
	}
	
	return workSample;
}

void SampleEditor::pasteOther(WorkSample& src)
{
	enterCriticalSection();

	prepareUndo();

	if (sample->sample)
	{
		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = NULL;
		sample->samplen = 0;
	}
	
	sample->loopstart = 0;
	sample->looplen = 0;
	sample->type = (src.numBits == 16) ? 16 : 0;
	sample->samplen = src.size;

	mp_sbyte rn, ft;
	XModule::convertc4spd((mp_uint32)src.sampleRate, &ft, &rn);
	sample->relnote = rn;
	sample->finetune = ft;
	
	sample->sample = (mp_sbyte*)src.buffer;
	src.buffer = NULL;
	
	finishUndo();
	
	leaveCriticalSection();
}

static float ppfabs(float f)
{
	return f < 0 ? -f : f;
}

float SampleEditor::getFloatSampleFromWaveform(pp_int32 index, void* src/* = NULL*/, pp_int32 size/* = 0*/)
{
	if (isEmptySample())
		return 0.0f;
		
	if (!src)
	{
		if (index > (signed)sample->samplen)
			index = sample->samplen;
		if (index < 0)
			index = 0;
	}
	else if (size != 0)
	{
		if (index >= size)
			index = size-1;
		if (index < 0)
			index = 0;
	}
			
	if (sample->type & 16)
	{
		mp_sword s = src ? *(((mp_sword*)src)+index) : sample->getSampleValue(index);
		return s > 0 ? (float)s*(1.0f/32767.0f) : (float)s*(1.0f/32768.0f);
	}
	else
	{
		mp_sbyte s = src ? *(((mp_sbyte*)src)+index) : sample->getSampleValue(index);
		return s > 0 ? (float)s*(1.0f/127.0f) : (float)s*(1.0f/128.0f);
	}
}

void SampleEditor::setFloatSampleInWaveform(pp_int32 index, float singleSample, void* src/* = NULL*/)
{
	if (isEmptySample() || index > (signed)sample->samplen)
		return;
	
	if (index < 0)
		index = 0;
				
	if (singleSample > 1.0f)
		singleSample = 1.0f;
	if (singleSample < -1.0f)
		singleSample = -1.0f;

	if (sample->type & 16)
	{
		mp_sword s = singleSample > 0 ? (mp_sword)(singleSample*32767.0f+0.5f) : (mp_sword)(singleSample*32768.0f-0.5f);
		if (src)
			*(((mp_sword*)src)+index) = s;
		else
			sample->setSampleValue(index, s);
	}
	else
	{
		mp_sbyte s = singleSample > 0 ? (mp_sbyte)(singleSample*127.0f+0.5f) : (mp_sbyte)(singleSample*128.0f-0.5f);
		if (src)
			*(((mp_sbyte*)src)+index) = s;
		else
			sample->setSampleValue(index, s);
	}
}

void SampleEditor::preFilter(TFilterFunc filterFuncPtr, const FilterParameters* par)
{
	if (filterFuncPtr)
	{
		if (par != NULL)
		{
			FilterParameters newPar(*par);
			if (lastParameters)
			{
				delete lastParameters;
				lastParameters = NULL;
			}
			lastParameters = new FilterParameters(newPar);			
		}
		else
		{
			if (lastParameters)
			{
				delete lastParameters;
				lastParameters = NULL;
			}
		}
		
		lastFilterFunc = filterFuncPtr;
	}

	enterCriticalSection();
	
	lastOperation = OperationRegular;

	notifyListener(NotificationPrepareLengthy);
}

void SampleEditor::postFilter()
{
	notifyListener(NotificationUnprepareLengthy);

	leaveCriticalSection();
}

void SampleEditor::tool_newSample(const FilterParameters* par)
{
	if (!isValidSample())
		return;

	preFilter(NULL, NULL);
	
	prepareUndo();

	pp_int32 numSamples = par->getParameter(0).intPart, numBits = par->getParameter(1).intPart;

	if (sample->sample)
	{
		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = NULL;
	}
	
	sample->samplen = numSamples;
	sample->loopstart = 0;
	sample->looplen = sample->samplen;
	
	switch (numBits)
	{
		case 8:
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen);
			memset(sample->sample, 0, sample->samplen);
			break;
		case 16:
			sample->type |= 16;
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen*2);
			memset(sample->sample, 0, sample->samplen*2);
			break;
		default:
			ASSERT(false);
	}
	
	finishUndo();

	lastOperation = OperationNew;
	postFilter();
}

void SampleEditor::tool_minimizeSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (!(sample->type & 3))
		return;

	preFilter(NULL, NULL);

	prepareUndo();

	pp_int32 loopend = sample->loopstart+sample->looplen;
	
	if (loopend > (signed)sample->samplen)
		loopend = sample->samplen;
	
	sample->samplen = loopend;

	finishUndo();
	
	postFilter();
}

void SampleEditor::tool_cropSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	if (sStart >= 0 && sEnd >= 0)
	{		
		if (sEnd < sStart)
		{
			pp_int32 s = sEnd; sEnd = sStart; sStart = s;
		}
	}
	else return;
	
	selectionStart = sStart;
	selectionEnd = sEnd;

	if (sStart == sEnd)
		return;
		
	preFilter(NULL, NULL);
	
	prepareUndo();
	
	if (sample->type & 16)
	{
		mp_sword* buff = (mp_sword*)sample->sample;
		for (pp_int32 i = selectionStart; i < selectionEnd; i++)
			buff[i-selectionStart] = buff[i];
	}
	else
	{
		mp_sbyte* buff = (mp_sbyte*)sample->sample;
		for (pp_int32 i = selectionStart; i < selectionEnd; i++)
			buff[i-selectionStart] = buff[i];
	}
	
	sample->samplen = abs(selectionEnd - selectionStart);
	
	if (sample->loopstart > sample->samplen)
		sample->loopstart = 0;
	
	pp_int32 loopend = sample->loopstart + sample->looplen;
	
	if (loopend > (signed)sample->samplen)
		loopend = sample->samplen;
	
	sample->looplen = loopend - sample->loopstart;
	
	selectionStart = 0;
	selectionEnd = sample->samplen;
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_clearSample(const FilterParameters* par)
{
	preFilter(NULL, NULL);
	
	prepareUndo();

	module->freeSampleMem((mp_ubyte*)sample->sample);
	sample->sample = NULL;
	sample->samplen = 0;
	sample->loopstart = 0;
	sample->looplen = 0;
	
	finishUndo();

	postFilter();
}

void SampleEditor::tool_convertSampleResolution(const FilterParameters* par)
{
	preFilter(NULL, NULL);

	prepareUndo();

	bool convert = (par->getParameter(0).intPart != 0);

	if (sample->type & 16)
	{
		
		if (!convert)
		{
			sample->type &= ~16;
			sample->samplen<<=1;
			sample->looplen<<=1;
			sample->loopstart<<=1;	

		}
		else
		{
			mp_sbyte* buffer = new mp_sbyte[sample->samplen];
			
			for (mp_sint32 i = 0; i < (signed)sample->samplen; i++)
				buffer[i] = (mp_sbyte)(sample->getSampleValue(i)>>8);
			
			module->freeSampleMem((mp_ubyte*)sample->sample);
			sample->type &= ~16;
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen);
			memcpy(sample->sample, buffer, sample->samplen);
			
			delete[] buffer;
		}
	}
	else
	{
		if (!convert)
		{
			sample->type |= 16;
			sample->samplen>>=1;
			sample->looplen>>=1;
			sample->loopstart>>=1;
		}
		else
		{			
			mp_sword* buff16 = new mp_sword[sample->samplen];
			
			for (mp_sint32 i = 0; i < (signed)sample->samplen; i++)
				buff16[i] = (mp_sword)(sample->getSampleValue(i)<<8);
			
			module->freeSampleMem((mp_ubyte*)sample->sample);
			sample->type |= 16;
			sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen*2);
			memcpy(sample->sample, buff16, sample->samplen*2);
			
			delete[] buff16;
		}
	}

	finishUndo();
	
	postFilter();
}

void SampleEditor::tool_mixPasteSample(const FilterParameters* par)
{
	ClipBoard* clipBoard = ClipBoard::getInstance();

	bool preservePitch  = par->getParameter(0).intPart > 0;
	bool overflow       = par->getParameter(0).intPart > 1;
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = preservePitch ? sStart : 0;
		sEnd = sample->samplen;
	}
	if (preservePitch) sEnd = sStart + clipBoard->getWidth();

	preFilter(NULL, NULL);
	
	prepareUndo();
	
	// preserve pitch (otherwise stretch clipboard to selection)
	float step = preservePitch ? 1 : (float)clipBoard->getWidth() / (float)(sEnd - sStart);
	
	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float frac = j - (float)floor(j);
		pp_int16 s = clipBoard->getSampleWord((pp_int32)j);
		float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
		s = clipBoard->getSampleWord((pp_int32)j+1);
		float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

		float f = (1.0f-frac)*f1 + frac*f2;
		
		setFloatSampleInWaveform(i % sample->samplen, f + getFloatSampleFromWaveform(i % sample->samplen));
		j+=step;
		if (!overflow && i == sample->samplen) break;
	}
				
	finishUndo();	
	
	postFilter();

}

void SampleEditor::tool_substractSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(NULL, NULL);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	float step = (float)clipBoard->getWidth() / (float)(sEnd - sStart);

	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float frac = j - (float)floor(j);

		pp_int16 s = clipBoard->getSampleWord((pp_int32)j);
		float f1 = s < 0 ? (s / 32768.0f) : (s / 32767.0f);
		s = clipBoard->getSampleWord((pp_int32)j + 1);
		float f2 = s < 0 ? (s / 32768.0f) : (s / 32767.0f);

		float f = (1.0f - frac) * f1 + frac * f2;

		setFloatSampleInWaveform(i, getFloatSampleFromWaveform(i) - f);
		j += step;
	}

	finishUndo();

	postFilter();

}

void SampleEditor::tool_repeatSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	preFilter(&SampleEditor::tool_repeatSample, par);

	prepareUndo();

	int repeat = par->getParameter(0).intPart;
	ClipBoard* clipBoard = ClipBoard::getInstance();
	for( int i = 0; i < repeat; i++)
		clipBoard->paste(*sample, *module, getSelectionStart());

	finishUndo();

	postFilter();

}

void SampleEditor::tool_AMPasteSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(NULL, NULL);
	
	prepareUndo();
	
	ClipBoard* clipBoard = ClipBoard::getInstance();
	
	float step = (float)clipBoard->getWidth() / (float)(sEnd-sStart);
	
	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float frac = j - (float)floor(j);
	
		pp_int16 s = clipBoard->getSampleWord((pp_int32)j);
		float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
		s = clipBoard->getSampleWord((pp_int32)j+1);
		float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

		float f = (1.0f-frac)*f1 + frac*f2;
		
		setFloatSampleInWaveform(i, f * getFloatSampleFromWaveform(i));
		j+=step;
	}
				
	finishUndo();	
	
	postFilter();

}

void SampleEditor::tool_modulateFilterSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

  int sweeps = par->getParameter(1).intPart;
	if (ClipBoard::getInstance()->isEmpty() && sweeps == 0){
		return;
  }

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	pp_int32 sLength = sEnd - sStart;

	preFilter(&SampleEditor::tool_modulateFilterSample, par);

	prepareUndo();

	float* sweepbuf;
	ClipBoard* clipBoard = ClipBoard::getInstance();
	
	// inspired by sandy999999's github JUCE example
	// Difference equation for wah wah filter (1-G) x[n]  -(1-G)   x(n-2] + 2Gcos(thetha) y[n-1]  -(2G-1) y[n-2]
	// Represented in variables:               b0 *  x  +  b2     *  x2   +     a1       * y1   +   a2   *  y2

	//Plugin states
	float x = 0;
	float x1 = 0;
	float x2 = 0;

	float y = 0;
	float y1 = 0;
	float y2 = 0;

	float intensity = par->getParameter(0).floatPart; // min: 0.05 max
	float g = 0.5;
	
	if (g > 0.95) g = 0.95;
	
	float b0 = (1 - g);
	float b2 = -(1 - g);
	float a2 = -(2 * g - 1);
	float dry = 0.15;
	
	if( sweeps > 0 ){
		sweepbuf = (float*)malloc(sLength * sizeof(float));
		for (int i = 0; i < sLength; i++){ 
			sweepbuf[i] = fabs( asin(cos(i * (PI/((sLength)/sweeps)))) ) / 1.6; // range 0..1
			sweepbuf[i] *= intensity;
		}
	}

	struct EnvelopeFollow e;
	e.output = 0.0;
	e.samplerate = 44100;
	e.release = 0.01;
	float boost = 0.0;

	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		// update envelope
		float sy = 0.0;
		if( sweeps > 0 ) sy = sweepbuf[i];
		else{
		  pp_int16 s = clipBoard->getSampleWord((pp_int32)i % (clipBoard->getWidth()));
		  sy = s < 0 ? (s / 32768.0f) : (s / 32767.0f);
		  sy = (sy + 1.0) / 2.0; // 0..1
		}
		envelope_follow(sy, &e);
		
		// apply envelope to filter
		float a1 = 2 * g * (sweeps > 0 ? sy : (-e.output+1.0) );
		float x = this->getFloatSampleFromWaveform(sStart + i);
		
		y = b0 * x + b2 * x2 + a1 * y1 + a2 * y2;
		boost = sweeps > 0 ? (-e.output + 1.0) * y : e.output * y;
		this->setFloatSampleInWaveform(sStart + i, (y + boost) + ((x * dry)) );

		x2 = x1;
		x1 = x;

		y2 = y1;
		y1 = y;
	}

	// enable forward loop
	if (!getLoopType()){
    sample->loopstart = sStart;
    sample->looplen = sEnd;
    setLoopType(1);
  }

  if( sweeps > 0 ) free(sweepbuf);

	finishUndo();

	postFilter();
}

void SampleEditor::tool_modulateEnvelopeSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_modulateEnvelopeSample, par);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();
	int type = par->getParameter(0).intPart;

	struct EnvelopeFollow e;
	e.output = 0.0;
	e.samplerate = 44100;
	e.release = 0.0001 + (0.1 / 9) * par->getParameter(0).intPart;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float in = this->getFloatSampleFromWaveform(sStart + i);
		pp_int16 s = clipBoard->getSampleWord((pp_int32)i % clipBoard->getWidth());
		float y = s < 0 ? (s / 32768.0f) : (s / 32767.0f);
		y = (y+1.0)/ 2.0;
		envelope_follow(y, &e);
		setFloatSampleInWaveform(sStart + i, in * e.output);
	}

	finishUndo();

	postFilter();
}


void SampleEditor::tool_FMPasteSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(NULL, NULL);
	
	prepareUndo();
	
	ClipBoard* clipBoard = ClipBoard::getInstance();
	
	float step;
	
	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float frac = j - (float)floor(j);
	
		pp_int16 s = clipBoard->getSampleWord(((pp_int32)j)%clipBoard->getWidth());
		float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
		s = clipBoard->getSampleWord(((pp_int32)j+1)%clipBoard->getWidth());
		float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

		float f = (1.0f-frac)*f1 + frac*f2;
		
		step = powf(16.0f,getFloatSampleFromWaveform(i));
		setFloatSampleInWaveform(i, f);
		j+=step;
		while (j>clipBoard->getWidth()) j-=clipBoard->getWidth();
	}
				
	finishUndo();	
	
	postFilter();

}

void SampleEditor::tool_PHPasteSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(NULL, NULL);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	// this filter changes the ratio between above zero to below
	// zero values by stretching the above half wave and shrinking
	// the below zero half wave (or the other way around)
	// the frequency of the sample stays constant, only
	// if the initial ratio is 1/1
	// If it's not, the frequency shifts.
	// To work with non synthetic or already distorted samples,
	// this ratio needs to be calculated and compensated.
	// The frequency will still shift if the ratio is not constant
	// during a longer sample. Ce la vie.
	pp_int32 ups=0,downs=0,zeros=0;
	for (pp_int32 i = 0; i < clipBoard->getWidth(); i++)
	{
		if (clipBoard->getSampleWord(i)<0)
		{
			downs++;
		}
		else if (clipBoard->getSampleWord(i)>0)
		{
			ups++;
		}
		else
		{
			zeros++;
		}
	}
	if (!downs && !zeros)
	{
		downs++; // div by zero prevention
	}
	float phaseRatio = ((float)ups+(0.5f*(float)zeros))/((float)downs+(0.5f*(float)zeros));
	float step;
	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float f;
		float fi = getFloatSampleFromWaveform(i);
		// we need to oversample at a much shorter step size to
		// track the zero crossing with sufficient accuracy
		for (pp_int32 oversample = 0; oversample<0x80; oversample++)
		{
			float frac = j - (float)floor(j);

			pp_int16 s = clipBoard->getSampleWord(((pp_int32)j)%clipBoard->getWidth());
			float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
			s = clipBoard->getSampleWord(((pp_int32)j+1)%clipBoard->getWidth());
			float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

			f = (1.0f-frac)*f1 + frac*f2;

			step = powf(16.0f,fabsf(fi));
			// the lower half wave is matched
			// to keep the frequency constant
			if (f*fi<0.0f)
			{
				step = 1.0f / (1.0f + (1.0f-(1.0f/step)));
			}
			// which needs to be compensated for a nonzero
			// initial half wave ratio
			if (f<0.0f)
			{
				step = step * (1.0f/phaseRatio);
			}
			// we advance by a fraction due to oversampling
			j+=step*(1.0f/0x80);
		}
		while (j>clipBoard->getWidth()) j-=clipBoard->getWidth();
		setFloatSampleInWaveform(i, f);
	}

	finishUndo();

	postFilter();

}

void SampleEditor::tool_FLPasteSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	if (ClipBoard::getInstance()->isEmpty())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(NULL, NULL);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	float step = (float)clipBoard->getWidth() / (float)(sEnd-sStart);

	// This filter mixes the current sample selection
	// with a copy of itself, which is phase-shifted based on the
	// envelope in the clipboard.
	// For best traditional "flangy" results,
	// the envelope should be a smoothly increasing or decreasing
	// line:
	// 0.25 period of a triangle wave at 25% volume works well,
	// and so do  any other smooth ramps up or down to aprox 25%
	// volume.
	// Interesting effects can be achieved with hand drawn envelopes
	float j = 0.0f;
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float frac = j - (float)floor(j);

		pp_int16 s = clipBoard->getSampleWord((pp_int32)j);
		float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
		s = clipBoard->getSampleWord((pp_int32)j+1);
		float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

		float f = (1.0f-frac)*f1 + frac*f2;

		float g0 = getFloatSampleFromWaveform(i);

		float h = (float)i+f*256.0f;
		frac = h - (float)floor(h);
		pp_int32 hi = (pp_int32)h;
		f1 = getFloatSampleFromWaveform(hi);
		f2 = getFloatSampleFromWaveform(hi+1);
		float g1 = (1.0f-frac)*f1 + frac*f2;
		setFloatSampleInWaveform(i, 0.5f*(g0+g1));
		j+=step;
	}

	finishUndo();

	postFilter();

}

void SampleEditor::tool_reverberateSample(const FilterParameters* par)
{

	if (isEmptySample())
		return;

	pp_int32 sStart = 0;
	pp_int32 sEnd = sample->samplen;

	preFilter(&SampleEditor::tool_reverberateSample, par);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	pp_int32 cLength = clipBoard->getWidth();
	pp_int32 sLength = sEnd - sStart;
	float ratio = par->getParameter(1).floatPart;
	bool usePasteBuffer = false;  // optional: pastebuffer can be used as impulse response (not that useful though)
	bool padSample = true; 	      // add silence to sample

	//if (usePasteBuffer && ClipBoard::getInstance()->isEmpty()) return;

	// create IR float array
	float* impulseResponse;
	pp_int16 reverb_size = par->getParameter(0).intPart; // 1..200
	if (!usePasteBuffer) cLength = 100 * (reverb_size*10); //  100..100000
	impulseResponse = (float*)malloc(cLength * sizeof(float));
	float f;
	VRand rand;
	rand.seed(1);
	for (pp_int32 i = 0; i < cLength; i++) {
		if (usePasteBuffer) {
			pp_int16 s = clipBoard->getSampleWord((pp_int32)i);
			f = s < 0 ? (s / 32768.0f) : (s / 32767.0f);
		}
		else f = rand.white() * (1.0f - ((1.0f / (float)cLength) * (float)i));
		impulseResponse[i] = f;
	}

	if (padSample) {
		pp_int32 newSampleSize = sLength + cLength;
		mp_sword * dst = new mp_sword[newSampleSize];
		pp_int32 j = 0;

		for (j = 0; j < newSampleSize; j++   ) dst[j] = 0;
		for (j = sStart; j < sLength; j++    ) dst[j] = sample->getSampleValue(j);
		
		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = (mp_sbyte*)module->allocSampleMem(newSampleSize * 2);
		memcpy(sample->sample, dst, newSampleSize * 2);
		sample->samplen = newSampleSize;
		sLength = newSampleSize;
		delete[] dst;
	}
	
	// create sample float array
	float* smpin;
	float* smpout;
	smpin = (float*)malloc(sLength * sizeof(float));
	smpout = (float*)malloc(sLength * sizeof(float));
	for (pp_int32 i = 0; i < sLength; i++) {
		smpin[i] = this->getFloatSampleFromWaveform(i);
	}

	convolve(smpin, impulseResponse, sLength, cLength, &smpout);

	for (pp_int32 i = sStart; i < sLength; i++)
	{
		this->setFloatSampleInWaveform(i, ((smpin[i]*(1.0f-ratio)) + (smpout[i]*ratio)) * 1.2 );
	}

	free(impulseResponse);
	free(smpin);
	free(smpout);
	finishUndo();

	postFilter();

}

void SampleEditor::tool_resonantFilterSample(const FilterParameters* par)
{

	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_resonantFilterSample, par);

	prepareUndo();

	pp_int32 sLength = sEnd - sStart;
	struct Filter f;
	float cutoff_a = (float)par->getParameter(0).intPart;
	float cutoff_b = (float)par->getParameter(3).intPart;
	int   sweeps = par->getParameter(4).intPart;
	int   ftype = par->getParameter(2).intPart; // LP:0 HP:1 BP:2 NOTCH:3
	float pos;
	float out;
	float fQ = (float)par->getParameter(1).intPart / 10.0f;
	filter_init(&f);
	f.q = ((float)(1.0f - fQ));
	f.cutoff = cutoff_a;
	if (!sweeps) cutoff_b = cutoff_a;

	float amp = 1.0 + (10.0f * (tan(fQ * 1.5) * 0.1)); // compensate resonance
	float sweepstep = 1.0f / (float)sLength;
	if (cutoff_b > cutoff_a) {
		float tmp = cutoff_a;
		cutoff_a = cutoff_b;
		cutoff_b = tmp;
	}
	float cutoff_size = cutoff_a - cutoff_b;

	for (int i = 0; i < sLength; i++) {

		float in = this->getFloatSampleFromWaveform(sStart + i);
		filter(&f, in);
		switch (ftype) {
			case 0: out = f.lp;    break;
			case 1: out = f.hp;    break;
			case 2: out = f.bp;    break;
			case 3: out = f.notch; break;
		}
		this->setFloatSampleInWaveform(sStart + i, out * amp );
	}
	finishUndo();

	postFilter();

}

void SampleEditor::tool_seamlessLoopSample(const FilterParameters* par)
{

	if (isEmptySample())
		return;

	preFilter(&SampleEditor::tool_seamlessLoopSample, par);

	prepareUndo();

	float* samplebuf;
	float y;
	int fadein = par->getParameter(0).intPart;
	int fadeout = par->getParameter(1).intPart;
	pp_int32 sLength = sample->samplen;
	samplebuf = (float*)malloc(sLength * sizeof(float));
	pp_int32 lenFadeIn = floor(((float)sample->samplen / 100.0f) * fadein);
	pp_int32 lenFadeOut = floor(((float)sample->samplen / 100.0f) * fadeout);
	pp_int32 newSampleSize = sample->samplen - lenFadeOut;

	// copy sample
	for (int i = 0; i < sLength; i++) samplebuf[i] = this->getFloatSampleFromWaveform(i);
	// create new (shorter) sample
	module->freeSampleMem((mp_ubyte*)sample->sample);
	sample->sample = (mp_sbyte*)module->allocSampleMem(newSampleSize * 2);
	sample->samplen = newSampleSize;
	for (int i = 0; i < newSampleSize; i++) {

		float amp_fadein  = lenFadeIn  > 0 ? ((1.0f / lenFadeIn)  * (float)i) : 1.0f;
		float amp_fadeout = lenFadeOut > 0 ? ((1.0f / lenFadeOut) * (float)i) : 0.0f;
		y = 0.0f;
		if (i <= lenFadeIn)   y = samplebuf[i] * amp_fadein;
		else y = samplebuf[i];
		if (i < lenFadeOut)  y += samplebuf[(sLength - lenFadeOut) + i] * (1.0f - amp_fadeout);
		this->setFloatSampleInWaveform(i, y);
	}
	// enable forward loop
	if (!getLoopType()){
    sample->loopstart = 0;
    sample->looplen = sample->samplen;
    setLoopType(1);
  }

	free(samplebuf);

	finishUndo();

	postFilter();
}

void SampleEditor::tool_testSample(const FilterParameters* par)
{

	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_testSample, par);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	pp_int32 cLength = clipBoard->getWidth();
	pp_int32 sLength = sEnd - sStart;
	float ratio = 1.0;// par->getParameter(1).floatPart;
	bool usePasteBuffer = false; // optional: pasteBuffer can be used as impulse response

	if (usePasteBuffer && ClipBoard::getInstance()->isEmpty())
		return;

	float* samplebuf;
	float y;
	int fadein = 25;
	int fadeout = 25;
	samplebuf = (float*)malloc(sample->samplen * sizeof(float));
	pp_int32 lenFadeIn = floor( ((float)sample->samplen / 100.0f) * fadein);
	pp_int32 lenFadeOut = floor(((float)sample->samplen / 100.0f) * fadeout);
	pp_int32 newSampleSize = sample->samplen - lenFadeOut;
	
	// copy sample
	for (int i = 0; i < sample->samplen; i++) samplebuf[i] = this->getFloatSampleFromWaveform(i);
	// create new (shorter) sample
	module->freeSampleMem((mp_ubyte*)sample->sample);
	sample->sample = (mp_sbyte*)module->allocSampleMem(newSampleSize * 2);
	sample->samplen = newSampleSize;
	for (int i = 0; i < newSampleSize; i++) {

		float amp_fadein  = ((1.0f / lenFadeIn) * (float)i);
		float amp_fadeout = ((1.0f / lenFadeOut) * (float)i);
		if (i < lenFadeIn)   y = samplebuf[i] * amp_fadein;
		else y = samplebuf[i];
		if (i < lenFadeOut)  y += samplebuf[(newSampleSize - lenFadeOut) + i] * (1.0f - amp_fadeout);
		this->setFloatSampleInWaveform(i,y);
	}
	
	free(samplebuf);

	finishUndo();

	postFilter();
}

void SampleEditor::tool_vocodeSample(const FilterParameters* par)
{

	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_vocodeSample, par);

	prepareUndo();

	ClipBoard* clipBoard = ClipBoard::getInstance();

	pp_int32 cLength = clipBoard->getWidth();
	pp_int32 sLength = sEnd - sStart;
	
	if (ClipBoard::getInstance()->isEmpty())
		return;

	///global internal variables
	pp_int32 i;
	const pp_int32 bands = 16;
	pp_int32  swap;       //input channel swap
	float gain;       //output level
	float thru, high; //hf thru              
	float kout; //downsampled output
	pp_int32  kval; //downsample counter
	pp_int32  nbnd; //number of bands

	float param[7];
	// SANE DEFAULTS
	param[0] = 0.0f;   //input select
	param[2] = 0.40f;  //hi thru
	param[3] = 0.40f;  //hi band
	param[4] = (1.0f / 9.0)* par->getParameter(1).intPart; // envelope
	param[5] = 0.5f;   // filter q
	param[6] = 1.0f;
	

	//filter coeffs and buffers - seems it's faster to leave this global than make local copy 
	float f[bands][13]; //[0-8][0 1 2 | 0 1 2 3 | 0 1 2 3 | val rate]
                        //  #   reson | carrier |modulator| envelope
	// init 
	double tpofs = 6.2831853 / 44100; /* FIXME somehow guess samplerate */
	double rr, th; //, re;
	float sh;

	if( par->getParameter(0).intPart == 8){
		nbnd = 8;
		//re=0.003f;
		f[1][2] = 3000.0f;
		f[2][2] = 2200.0f;
		f[3][2] = 1500.0f;
		f[4][2] = 1080.0f;
		f[5][2] = 700.0f;
		f[6][2] = 390.0f;
		f[7][2] = 190.0f;
		param[1] = 0.40f + (param[4] * 0.6);  //output dB
	}
	else
	{
		nbnd = 16;
		//re=0.0015f;
		f[1][2] = 5000.0f; //+1000
		f[2][2] = 4000.0f; //+750
		f[3][2] = 3250.0f; //+500
		f[4][2] = 2750.0f; //+450
		f[5][2] = 2300.0f; //+300
		f[6][2] = 2000.0f; //+250
		f[7][2] = 1750.0f; //+250
		f[8][2] = 1500.0f; //+250
		f[9][2] = 1250.0f; //+250
		f[10][2] = 1000.0f; //+250
		f[11][2] = 750.0f; //+210
		f[12][2] = 540.0f; //+190
		f[13][2] = 350.0f; //+155
		f[14][2] = 195.0f; //+100
		f[15][2] = 95.0f;
		param[1] = 0.40f;  //output dB
	}

	for (i = 0; i < nbnd; i++) for (int j = 3; j < 12; j++) f[i][j] = 0.0f; //zero band filters and envelopes
	kout = 0.0f;
	kval = 0;
	swap = 1; if (param[0] > 0.5f) swap = 0;
	gain = (float)pow(10.0f, 2.0f * param[1] - 3.0f * param[5] - 2.0f);

	thru = (float)pow(10.0f, 0.5f + 2.0f * param[1]);
	high = param[3] * param[3] * param[3] * thru;
	thru *= param[2] * param[2] * param[2];

	float a, b, c, d, o = 0.0f, aa, bb, oo = kout, g = gain, ht = thru, hh = high, tmp;
	pp_int32 k = kval, sw = swap, nb = nbnd;

	if (param[4] < 0.05f) //freeze
	{
		for (i = 0; i < nbnd; i++) f[i][12] = 0.0f;
	}
	else
	{
		f[0][12] = (float)pow(10.0, -1.7 - 2.7f * param[4]); //envelope speed

		rr = 0.022f / (float)nbnd; //minimum proportional to frequency to stop distortion
		for (i = 1; i < nbnd; i++)
		{
			f[i][12] = (float)(0.025 - rr * (double)i);
			if (f[0][12] < f[i][12]) f[i][12] = f[0][12];
		}
		f[0][12] = 0.5f * f[0][12]; //only top band is at full rate
	}

	rr = 1.0 - pow(10.0f, -1.0f - 1.2f * param[5]);
	sh = (float)pow(2.0f, 3.0f * param[6] - 1.0f); //filter bank range shift

	for (i = 1; i < nbnd; i++)
	{
		f[i][2] *= sh;
		th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
		f[i][0] = (float)(2.0 * rr * cos(th)); //a0
		f[i][1] = (float)(-rr * rr);           //a1
					//was .98
		f[i][2] *= 0.96f; //shift 2nd stage slightly to stop high resonance peaks
		th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
		f[i][2] = (float)(2.0 * rr * cos(th));
	}

	/* process */
	for (pp_int32 si = 0; si < sLength; si++) {
		pp_int32 j  = si % clipBoard->getWidth();               // repeat carrier
		pp_int16 s  = clipBoard->getSampleWord((pp_int32)j);   // get clipboard sample word
		float fclip = s < 0 ? (s / 32768.0f) : (s / 32767.0f); // convert to float

		a = this->getFloatSampleFromWaveform(si); // speech
		b = fclip;                               // synth 
		
		if (sw == 0) { tmp = a; a = b; b = tmp; } //swap channels?

		tmp = a - f[0][7]; //integrate modulator for HF band and filter bank pre-emphasis
		f[0][7] = a;
		a = tmp;

		if (tmp < 0.0f) tmp = -tmp;
		f[0][11] -= f[0][12] * (f[0][11] - tmp);      //high band envelope
		o = f[0][11] * (ht * a + hh * (b - f[0][3])); //high band + high thru

		f[0][3] = b; //integrate carrier for HF band

		if (++k & 0x1) //this block runs at half sample rate
		{
			oo = 0.0f;
			aa = a + f[0][9] - f[0][8] - f[0][8];  //apply zeros here instead of in each reson
			f[0][9] = f[0][8];  f[0][8] = a;
			bb = b + f[0][5] - f[0][4] - f[0][4];
			f[0][5] = f[0][4];  f[0][4] = b;

			for (i = 1; i < nb; i++) //filter bank: 4th-order band pass
			{
				tmp = f[i][0] * f[i][3] + f[i][1] * f[i][4] + bb;
				f[i][4] = f[i][3];
				f[i][3] = tmp;
				tmp += f[i][2] * f[i][5] + f[i][1] * f[i][6];
				f[i][6] = f[i][5];
				f[i][5] = tmp;

				tmp = f[i][0] * f[i][7] + f[i][1] * f[i][8] + aa;
				f[i][8] = f[i][7];
				f[i][7] = tmp;
				tmp += f[i][2] * f[i][9] + f[i][1] * f[i][10];
				f[i][10] = f[i][9];
				f[i][9] = tmp;

				if (tmp < 0.0f) tmp = -tmp;
				f[i][11] -= f[i][12] * (f[i][11] - tmp);
				oo += f[i][5] * f[i][11];
			}
		}
		o += oo * g; //effect of interpolating back up to Fs would be minimal (aliasing >16kHz)

		setFloatSampleInWaveform(si, o );
	}

	finishUndo();

	postFilter();
}

void SampleEditor::tool_scaleSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_scaleSample, par);
	
	prepareUndo();
	
	float startScale = par->getParameter(0).floatPart;
	float endScale = par->getParameter(1).floatPart;
	
	float step = (endScale - startScale) / (float)(sEnd - sStart);
	
	for (pp_int32 i = sStart; i < sEnd; i++)
	{
		float f = getFloatSampleFromWaveform(i);
		setFloatSampleInWaveform(i, f*startScale);
		startScale+=step;
	}
				
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_normalizeSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_normalizeSample, par);
	
	prepareUndo();
	
	float maxLevel = ((par == NULL)? 1.0f : par->getParameter(0).floatPart);
	float peak = 0.0f;

	pp_int32 i;

	// find peak value
	for (i = sStart; i < sEnd; i++)
	{
		float f = getFloatSampleFromWaveform(i);
		if (ppfabs(f) > peak) peak = ppfabs(f);
	}
	
	float scale = maxLevel / peak;
	
	for (i = sStart; i < sEnd; i++)
	{
		float f = getFloatSampleFromWaveform(i);
		setFloatSampleInWaveform(i, f*scale);
	}
				
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_compressSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_compressSample, par);

	prepareUndo();

	pp_int32 i;
	float peak = 0.0f;

	// find peak value (pre)
	for (i = sStart; i < sEnd; i++)
	{
		float f = getFloatSampleFromWaveform(i);
		if (ppfabs(f) > peak) peak = ppfabs(f);
	}

	float max = 0.0f;
	float compress = peak * 0.66;
	bool limit = par->getParameter(0).intPart == 0;
	bool compensate = par->getParameter(1).intPart == 0;
	float last  = 0.0;
	float wpeak = 0.0;
	int zerocross[2];
	zerocross[0] = -1;
	zerocross[1] = -1;
	float treshold = 0.8;
	float peakTreshold = peak * treshold;

	if (limit) {
		// scaling limiter inspired by awesome 'TAP scaling limiter'
		for (i = sStart; i < sEnd; i++) {
			float f = getFloatSampleFromWaveform(i);
			if (ZEROCROSS(f, last)) {
				zerocross[0] = zerocross[1];
				zerocross[1] = i;
				if (zerocross[0] >= 0 && zerocross[1] > 0) {                   // detected waveset 
					wpeak = 0;
					for (int j = zerocross[0]; j < zerocross[1]; j++) {        // get peak from waveset
						float w = getFloatSampleFromWaveform(j);
						if (ppfabs(w) > wpeak) wpeak = ppfabs(w);
					}
					if (wpeak > peakTreshold) {                                    // scale down waveset if wpeak exceeds treshold
						for (int j = zerocross[0]; j < zerocross[1]; j++) {
							float b = getFloatSampleFromWaveform(j) * (peakTreshold / wpeak);
							this->setFloatSampleInWaveform(j,b );
						}
					}
				}
			}
			last = f;
		}
	}
	else {
		// saturated compress
		for (i = sStart; i < sEnd; i++)
		{
			float f = getFloatSampleFromWaveform(i);
			float b = compress * tanh(f / compress);
			if (ppfabs(f - b) > max) max = ppfabs(f - b);
			setFloatSampleInWaveform(i, b);
		}

	}
	if (compensate) {
		float scale = limit ? (peak/peakTreshold) : (peak / (peak-max) ); // compensate levels
		for (i = sStart; i < sEnd; i++)
		{
			float f = getFloatSampleFromWaveform(i);
			setFloatSampleInWaveform(i, f * scale);
		}
	}

	finishUndo();

	postFilter();
}

void SampleEditor::tool_decimateSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_decimateSample, par);

	prepareUndo();

	float maxLevel = ((par == NULL) ? 1.0f : par->getParameter(0).floatPart);
	float max = 0.0f;

	pp_int32 i;

	int bits = par->getParameter(0).intPart;
	float rate = par->getParameter(1).floatPart;

	long int m = 1 << (bits - 1);
	float y = 0, cnt = 0;

	for (i = sStart; i < sEnd; i++)
	{
		float f = getFloatSampleFromWaveform(i);
		cnt += rate;
		if (cnt >= 1)
		{
			cnt -= 1;
			y = (long int)(f * m) / (float)m;
		}
		setFloatSampleInWaveform(i, y);
	}

	finishUndo();

	postFilter();
}

void SampleEditor::tool_bitshiftSample(const FilterParameters* par)
{
	if (isEmptySample() || sample->type & 8 ) return;

	preFilter(&SampleEditor::tool_bitshiftSample, par);

	prepareUndo();

	mp_sword *samplebuf;
	int byteN    = par->getParameter(0).intPart;
	int bitshift = par->getParameter(1).intPart;
	
	// backup 16bit sample
	samplebuf = (mp_sword*)malloc(sample->samplen * sizeof(mp_sword));
	for (int i = 0; i < sample->samplen; i++) samplebuf[i] = sample->getSampleValue(i);
	
	// convert current sample to 8bit
	mp_sbyte* buffer = new mp_sbyte[sample->samplen];
	for (mp_sint32 i = 0; i < (signed)sample->samplen; i++)
		buffer[i] = (mp_sbyte)(sample->getSampleValue(i) >> 8);
	module->freeSampleMem((mp_ubyte*)sample->sample);
	sample->type &= ~16;
	sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen);
	memcpy(sample->sample, buffer, sample->samplen);
	delete[] buffer;

	for (int i = 0; i < sample->samplen; i++)
	{
		mp_sword f = samplebuf[i];
		mp_sbyte one = f & 0xFF;      // get first byte
		mp_sbyte two = f >> 8;        // get second byte
		mp_sbyte in = byteN == 0 ? one : two;
		switch (bitshift) {  // just to be safe (no idea whether '>> bitshift' works on all compilers)
			case 0: break;
			case 1: in = in >> 1; break;
			case 2: in = in >> 2; break;
			case 3: in = in >> 3; break;
			case 4: in = in >> 4; break;
			case 5: in = in >> 5; break;
			case 6: in = in >> 6; break;
			case 7: in = in >> 7; break;
		}
		sample->setSampleValue(i, in);
	}

	free(samplebuf);

	finishUndo();

	postFilter();
}

void SampleEditor::tool_exciteSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_exciteSample, par);
	prepareUndo();

	mp_sint32 i = 0;

	float x = 0.0f;
	bool subtle = true;
	float aliase = par->getParameter(1).floatPart ;
	float cut1 = 0.86;
	float cut2 = 0.725;
	float cut = par->getParameter(0).floatPart;
	float boost = 15.0;
	float comp = 0.05;
	float lpdata = 0.0;
	float hpdata = 0.0;
	int n = 0;

	for (i = sStart; i < sEnd; i++)
	{
		x = getFloatSampleFromWaveform(i);

		lpdata = lpdata - (cut * (lpdata - x));
		hpdata = lpdata - x;
		// introduce partial aliasing
		if (aliase > 0.0f) {
			n += 1;
			if ((n % 2) == 0) hpdata = hpdata * (aliase+1.0f);
		}
		// maximize
		hpdata = (comp * tanh(hpdata / comp)) * boost;

		setFloatSampleInWaveform(i, (x + hpdata) * 0.9);
	}

	finishUndo();

	postFilter();
}

void SampleEditor::tool_bassboostSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;

	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	preFilter(&SampleEditor::tool_bassboostSample, par);
	prepareUndo();

	mp_sint32 i = 0;

	float x = 0.0f;
	
	float gain1;
	float cap = 0.0f;
	float ratio = 0.4f;
	float selectivity = 60.0f;
	float gain2 = 0.8f;
	

	for (i = sStart; i < sEnd; i++)
	{
		x = getFloatSampleFromWaveform(i);

		gain1 = 1.0 / (selectivity + 1.0);
		cap = (x + cap * selectivity) * gain1;
		x = MIN( MAX(-1.0, (x + cap * ratio) * gain2), 1.0);
		setFloatSampleInWaveform(i,x);
	}

	finishUndo();

	postFilter();
}


void SampleEditor::tool_reverseSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_reverseSample, par);
	
	prepareUndo();
	
	pp_int32 i;
	for (i = 0; i < (sEnd-sStart)>>1; i++)
	{
		float f1 = getFloatSampleFromWaveform(sStart + i);
		float f2 = getFloatSampleFromWaveform(sEnd - 1 - i);
		float h = f2;
		f2 = f1; f1 = h;
		setFloatSampleInWaveform(sStart + i, f1);
		setFloatSampleInWaveform(sEnd - 1 - i, f2);
	}
				
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_PTboostSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_PTboostSample, par);
	
	prepareUndo();
	
	pp_int32 i;
	
	float d0 = 0.0f, d1, d2;
	for (i = sStart; i < sEnd; i++)
	{
		d1 = d2 = getFloatSampleFromWaveform(i);
		d1 -= d0;
		d0 = d2;
		
		if (d1 < 0.0f)
		{
			d1 = -d1;
			d1*= 0.25f;
			d2 -= d1;
		}
		else
		{
			d1*= 0.25f;
			d2 += d1;
		}
		
		if (d2 > 1.0f)
			d2 = 1.0f;
		
		if (d2 < -1.0f)
			d2 = -1.0f;
		
		setFloatSampleInWaveform(i, d2);
	}
	
	finishUndo();	
	
	postFilter();
}

bool SampleEditor::isValidxFadeSelection()
{
	if (isEmptySample() || !hasValidSelection() || !(sample->type & 3))
		return false;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (sStart >= 0 && sEnd >= 0)
	{		
		if (sEnd < sStart)
		{
			pp_int32 s = sEnd; sEnd = sStart; sStart = s;
		}
	}
	
	pp_uint32 loopend = sample->loopstart + sample->looplen;
		
	if (((unsigned)sStart <= sample->loopstart && (unsigned)sEnd >= loopend) ||
		((unsigned)sStart > sample->loopstart && (unsigned)sEnd < loopend) ||
		((unsigned)sStart < sample->loopstart && (unsigned)sEnd < sample->loopstart) || 
		((unsigned)sStart > loopend && (unsigned)sEnd > loopend))
		return false;
		
	return true;
}

void SampleEditor::tool_xFadeSample(const FilterParameters* par)
{
	if (!isValidxFadeSelection())
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (sStart >= 0 && sEnd >= 0)
	{		
		if (sEnd < sStart)
		{
			pp_int32 s = sEnd; sEnd = sStart; sStart = s;
		}
	}
	
	if (!(sample->type & 3) || sEnd < (signed)sample->loopstart || sStart > (signed)(sample->loopstart + sample->looplen))
		return;

	pp_int32 loopend = sample->loopstart + sample->looplen;
		
	preFilter(&SampleEditor::tool_xFadeSample, par);
	
	if (sStart <= (signed)sample->loopstart && sEnd >= loopend)
		return;
		
	if (sStart >= (signed)sample->loopstart && sEnd >= loopend)
	{
		sStart-=loopend;
		sStart+=sample->loopstart;
		sEnd-=loopend;
		sEnd+=sample->loopstart;
	}
	
	mp_ubyte* buffer = new mp_ubyte[(sample->type & 16) ? sample->samplen*2 : sample->samplen];
	if (!buffer)
		return;

	memcpy(buffer, sample->sample, (sample->type & 16) ? sample->samplen*2 : sample->samplen);

	prepareUndo();	
	
	pp_int32 i = 0;
	
	// loop start
	if ((sample->type & 3) == 1)
	{	
		for (i = sStart; i < (signed)sample->loopstart; i++)
		{
			float t = (((float)i - sStart) / (float)(sample->loopstart - sStart))*0.5f;
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(loopend - (sample->loopstart - sStart) + (i - sStart), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}
		
		for (i = sample->loopstart; i < sEnd; i++)
		{
			float t = 0.5f - ((((float)i - sample->loopstart) / (float)(sEnd-sample->loopstart))*0.5f);
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(loopend + (i - sample->loopstart), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}
		
		// loop end
		sStart-=sample->loopstart;
		sStart+=loopend;
		sEnd-=sample->loopstart;
		sEnd+=loopend;	
		
		for (i = sStart; i < loopend; i++)
		{
			float t = (((float)i - sStart) / (float)(loopend - sStart))*0.5f;
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(sample->loopstart - (loopend - sStart) + (i - sStart), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}	
		
		for (i = loopend; i < sEnd; i++)
		{
			float t = 0.5f - ((((float)i - loopend) / (float)(sEnd-loopend))*0.5f);
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(sample->loopstart + (i - loopend), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}
		
	}
	else if ((sample->type & 3) == 2)
	{
		for (i = sStart; i < (signed)sample->loopstart; i++)
		{
			float t = (((float)i - sStart) / (float)(sample->loopstart - sStart))*0.5f;
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(sample->loopstart + (i - sStart), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}
		
		for (i = sample->loopstart; i < sEnd; i++)
		{
			float t = 0.5f - ((((float)i - sample->loopstart) / (float)(sEnd-sample->loopstart))*0.5f);
			
			float f1 = getFloatSampleFromWaveform(i, buffer, sample->samplen);
			float f2 = getFloatSampleFromWaveform(sample->loopstart - (i - sample->loopstart), buffer, sample->samplen);		
			
			float f = f1*(1.0f-t) + f2*t;
			setFloatSampleInWaveform(i, f);
		}
	}
	
	delete[] buffer;
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_changeSignSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;

	pp_int32 ignorebits = par->getParameter(0).intPart;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_changeSignSample, par);
	
	prepareUndo();
	
	pp_int32 i;
	pp_uint32 mask;
	if (sample->type & 16)
	{
		mask = 0xffff >> ignorebits;
	}
	else
	{
		mask = 0xff >> ignorebits;
	}
	// lazyness follows
	for (i = sStart; i < sEnd; i++)
	{
		if (sample->type & 16)
		{
			mp_uword* smp = (mp_uword*)sample->sample;
			smp[i] ^= mask;
		}
		else
		{
			mp_ubyte* smp = (mp_ubyte*)sample->sample;
			smp[i] ^= mask;
		}
	}
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_swapByteOrderSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	if (!(sample->type & 16))
		return;

	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_swapByteOrderSample, par);
	
	prepareUndo();
	
	pp_int32 i;

	mp_uword* smp = (mp_uword*)sample->sample;		
	for (i = sStart; i < sEnd; i++)
	{
		mp_uword s = (smp[i] >> 8) | ((smp[i] & 0xFF) << 8);
		smp[i] = s;
	}
	
	finishUndo();	
	
	postFilter();
}

float getc4spd(mp_sint32 relnote,mp_sint32 finetune);

void SampleEditor::tool_resampleSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	preFilter(&SampleEditor::tool_resampleSample, par);
	
	prepareUndo();

	float c4spd = getc4spd(sample->relnote, sample->finetune);

	pp_uint32 resamplerType = par->getParameter(1).intPart;

	SampleEditorResampler resampler(*module, *sample, resamplerType);
	
	bool res = resampler.resample(c4spd, par->getParameter(0).floatPart);
	
	float step = c4spd / par->getParameter(0).floatPart;

	if (res)
	{
		sample->loopstart = (mp_sint32)(sample->loopstart/step);
		sample->looplen = (mp_sint32)(sample->looplen/step);
	
		if (par->getParameter(2).intPart)
		{
			pp_uint32 c4spdi = (mp_uint32)par->getParameter(0).floatPart;
			mp_sbyte rn, ft;
			XModule::convertc4spd((mp_uint32)c4spdi, &ft, &rn);
			sample->relnote = rn;
			sample->finetune = ft;
		}
	}
	
	lastOperation = OperationCut;
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_DCNormalizeSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_DCNormalizeSample, par);
	
	prepareUndo();
	
	pp_int32 i;

	float DC = 0.0f;
	for (i = sStart; i < sEnd; i++)
	{
		DC += getFloatSampleFromWaveform(i);		
	}
	DC = DC / (float)(sEnd-sStart);
	for (i = sStart; i < sEnd; i++)
	{
		setFloatSampleInWaveform(i, getFloatSampleFromWaveform(i) - DC);
	}
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_DCOffsetSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_DCOffsetSample, par);
	
	prepareUndo();
	
	pp_int32 i;

	float DC = par->getParameter(0).floatPart;
	for (i = sStart; i < sEnd; i++)
	{
		setFloatSampleInWaveform(i, getFloatSampleFromWaveform(i) + DC);
	}
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_rectangularSmoothSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_rectangularSmoothSample, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	mp_ubyte* buffer = new mp_ubyte[(sample->type & 16) ? sLen*2 : sLen];
	if (!buffer)
		return;

	memcpy(buffer, sample->sample + sStart, (sample->type & 16) ? sLen*2 : sLen);

	prepareUndo();	
	
	pp_int32 i;

	for (i = sStart; i < sEnd; i++)
	{
		float f = (getFloatSampleFromWaveform(i - sStart - 1, buffer, sLen) +
				  getFloatSampleFromWaveform(i - sStart, buffer, sLen) +
				  getFloatSampleFromWaveform(i - sStart + 1, buffer, sLen)) * (1.0f/3.0f);
				  
		setFloatSampleInWaveform(i, f);		
	}
	
	delete[] buffer;
	
	finishUndo();	
	
	postFilter();
}

void SampleEditor::tool_triangularSmoothSample(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_triangularSmoothSample, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	mp_ubyte* buffer = new mp_ubyte[(sample->type & 16) ? sLen*2 : sLen];
	if (!buffer)
		return;

	memcpy(buffer, sample->sample + sStart, (sample->type & 16) ? sLen*2 : sLen);

	prepareUndo();	
	
	pp_int32 i;

	for (i = sStart; i < sEnd; i++)
	{
		float f = (getFloatSampleFromWaveform(i - sStart - 2, buffer, sLen) +
				   getFloatSampleFromWaveform(i - sStart - 1, buffer, sLen)*2.0f +
				   getFloatSampleFromWaveform(i - sStart, buffer, sLen)*3.0f +
				   getFloatSampleFromWaveform(i - sStart + 1, buffer, sLen)*2.0f +
				   getFloatSampleFromWaveform(i - sStart + 2, buffer, sLen)) * (1.0f/9.0f);
				  
		setFloatSampleInWaveform(i, f);		
	}
	
	delete[] buffer;
	
	finishUndo();	

	postFilter();
}

void SampleEditor::tool_eqSample(const FilterParameters* par)
{
	tool_eqSample(par,false);
}

void SampleEditor::tool_eqSample(const FilterParameters* par, bool selective)
{
	if (isEmptySample())
		return;

	if (selective && ClipBoard::getInstance()->isEmpty())
		return;

		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}

	if (selective) {
		preFilter(NULL,NULL);
	} else {	
		preFilter(&SampleEditor::tool_eqSample, par);
	}
	
	prepareUndo();	
	
	ClipBoard* clipBoard;
	float step;
	float j2 = 0.0f;
	if (selective) {
		clipBoard = ClipBoard::getInstance();
		step = (float)clipBoard->getWidth() / (float)(sEnd-sStart);
	}
	
	float c4spd = 8363; // there really should be a global constant for this
	
	Equalizer** eqs = new Equalizer*[par->getNumParameters()];
	
	// three band EQ
	if (par->getNumParameters() == 3)
	{
		for (pp_int32 i = 0; i < par->getNumParameters(); i++)
		{
			eqs[i] = new Equalizer();
			eqs[i]->CalcCoeffs(EQConstants::EQ3bands[i], EQConstants::EQ3bandwidths[i], c4spd, Equalizer::CalcGain(par->getParameter(i).floatPart));
		}
	}
	// ten band EQ
	else if (par->getNumParameters() == 10)
	{
		for (pp_int32 i = 0; i < par->getNumParameters(); i++)
		{
			eqs[i] = new Equalizer();
			eqs[i]->CalcCoeffs(EQConstants::EQ10bands[i], EQConstants::EQ10bandwidths[i], c4spd, Equalizer::CalcGain(par->getParameter(i).floatPart));
		}
	}
	else
	{
		delete[] eqs;
		finishUndo();
		return;
	}
	
	// apply EQ here
	pp_int32 i;

	for (i = sStart; i < sEnd; i++)
	{
		// Fetch a stereo signal
		double xL = getFloatSampleFromWaveform(i);
		double xR = xL;
		float x = (float)xL;
			
		for (pp_int32 j = 0; j < par->getNumParameters(); j++)
		{
			double yL, yR;
			// Pass the stereo input
			eqs[j]->Filter(xL, xR, yL, yR);
			
			xL = yL;
			xR = yR;
		}
		if (selective)
		{
			float frac = j2 - (float)floor(j2);
		
			pp_int16 s = clipBoard->getSampleWord((pp_int32)j2);
			float f1 = s < 0 ? (s/32768.0f) : (s/32767.0f);
			s = clipBoard->getSampleWord((pp_int32)j2+1);
			float f2 = s < 0 ? (s/32768.0f) : (s/32767.0f);

			float f = (1.0f-frac)*f1 + frac*f2;

			if (f>=0) {
				x = f * ((float)xL) + (1.0f-f) * x;
			} else {
				x = -f * (x-(float)xL) + (1.0+f) * x; 
			}
			j2+=step;
		} else {
			x = (float)xL;
		}
		setFloatSampleInWaveform(i, x);
	}
	
	for (i = 0; i < par->getNumParameters(); i++)
		delete eqs[i];
	
	delete[] eqs;
	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateSilence(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (sStart >= 0 && sEnd >= 0)
	{
		if (sEnd < sStart)
		{
			pp_int32 s = sEnd; sEnd = sStart; sStart = s;
		}
	}
	else
	{
		sStart = 0;
		sEnd = 0;
	}
	
	preFilter(&SampleEditor::tool_generateSilence, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	prepareUndo();	
	
	pp_int32 i, j;

	pp_int32 size = par->getParameter(0).intPart;

	pp_int32 newSampleSize = (sample->samplen - sLen) + size;
	
	if (sample->type & 16)
	{		
		mp_sword* dst = new mp_sword[newSampleSize];

		j = 0;
		for (i = 0; i < sStart; i++, j++)
			dst[j] = sample->getSampleValue(i);

		for (i = 0; i < size; i++, j++)
			dst[j] = 0;

		for (i = sEnd; i < (signed)sample->samplen; i++, j++)
			dst[j] = sample->getSampleValue(i);

		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = (mp_sbyte*)module->allocSampleMem(newSampleSize*2);
		memcpy(sample->sample, dst, newSampleSize*2);

		sample->samplen = newSampleSize;

		delete[] dst;
	}
	else
	{
		mp_sbyte* dst = new mp_sbyte[newSampleSize];

		j = 0;
		for (i = 0; i < sStart; i++, j++)
			dst[j] = sample->getSampleValue(i);

		for (i = 0; i < size; i++, j++)
			dst[j] = 0;

		for (i = sEnd; i < (signed)sample->samplen; i++, j++)
			dst[j] = sample->getSampleValue(i);

		module->freeSampleMem((mp_ubyte*)sample->sample);
		sample->sample = (mp_sbyte*)module->allocSampleMem(newSampleSize);
		memcpy(sample->sample, dst, newSampleSize);

		sample->samplen = newSampleSize;

		delete[] dst;
	}

	// show everything
	lastOperation = OperationCut;
	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateNoise(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_generateNoise, par);
	
	prepareUndo();	
	
	pp_int32 i;

	pp_int32 type = par->getParameter(0).intPart;

	VRand rand;
	rand.seed();

	switch (type)
	{
		case 0:
			for (i = sStart; i < sEnd; i++)
				setFloatSampleInWaveform(i, rand.white()*2.0f);		
			break;
		case 1:
			for (i = sStart; i < sEnd; i++)
				setFloatSampleInWaveform(i, rand.pink()*2.0f);		
			break;
		case 2:
			for (i = sStart; i < sEnd; i++)
				setFloatSampleInWaveform(i, rand.brown()*2.0f);		
			break;
	}
	
	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateSine(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_generateSine, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	prepareUndo();	
	
	pp_int32 i;

	const float numPeriods = (float)(6.283185307179586476925286766559 * par->getParameter(1).floatPart);
	const float amplify = par->getParameter(0).floatPart;

	// generate sine wave here
	for (i = sStart; i < sEnd; i++)
	{
		float per = (i-sStart)/(float)sLen * numPeriods;
		setFloatSampleInWaveform(i, (float)sin(per)*amplify);	
	}

	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateSquare(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_generateSquare, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	prepareUndo();	
	
	pp_int32 i;

	const float numPeriods = par->getParameter(1).floatPart;
	const float amplify = par->getParameter(0).floatPart;

	// generate square wave here
	for (i = sStart; i < sEnd; i++)
	{
		float per = (i-sStart)/(float)sLen * numPeriods;
		float frac = per-(float)floor(per);
		setFloatSampleInWaveform(i, frac < 0.5f ? amplify : -amplify);	
	}

	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateTriangle(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_generateTriangle, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	prepareUndo();	
	
	pp_int32 i;

	const float numPeriods = par->getParameter(1).floatPart;
	const float amplify = par->getParameter(1).floatPart;

	// generate triangle wave here
	for (i = sStart; i < sEnd; i++)
	{
		float per = (i-sStart)/(float)sLen * numPeriods;
		float frac = per-(float)floor(per);
		if (frac < 0.25f)
			setFloatSampleInWaveform(i, (frac*4.0f)*amplify);	
		else if (frac < 0.75f)
			setFloatSampleInWaveform(i, (1.0f-(frac-0.25f)*4.0f)*amplify);	
		else	
			setFloatSampleInWaveform(i, (-1.0f+(frac-0.75f)*4.0f)*amplify);	
	}

	finishUndo();	

	postFilter();
}

void SampleEditor::tool_generateSawtooth(const FilterParameters* par)
{
	if (isEmptySample())
		return;
		
	pp_int32 sStart = selectionStart;
	pp_int32 sEnd = selectionEnd;
	
	if (hasValidSelection())
	{
		if (sStart >= 0 && sEnd >= 0)
		{		
			if (sEnd < sStart)
			{
				pp_int32 s = sEnd; sEnd = sStart; sStart = s;
			}
		}
	}
	else
	{
		sStart = 0;
		sEnd = sample->samplen;
	}
	
	preFilter(&SampleEditor::tool_generateSawtooth, par);
	
	mp_sint32 sLen = sEnd - sStart;
	
	prepareUndo();	
	
	pp_int32 i;

	const float numPeriods = par->getParameter(1).floatPart;
	const float amplify = par->getParameter(0).floatPart;

	// generate saw-tooth wave here
	for (i = sStart; i < sEnd; i++)
	{
		float per = (i-sStart)/(float)sLen * numPeriods;
		float frac = per-(float)floor(per);
		setFloatSampleInWaveform(i, frac < 0.5f ? (frac*2.0f)*amplify : (-1.0f+((frac-0.5f)*2.0f))*amplify);	
	}

	finishUndo();	

	postFilter();
}

// we're running scripts here so scripts can benefit from the 'redo filter' & undo/redo features
void SampleEditor::tool_runScript(const FilterParameters* par)
{
	pp_int32 i;
	PPString file = PPString(par->getParameter(0).stringPart);
	PPString fin = PPString(par->getParameter(1).stringPart);
	PPString fout = PPString(par->getParameter(2).stringPart);
	
	// push undo/redo here (otherwise 'redo filter' can't reload the script since its buried by all the earlier actions pushed to the undotrack)
	preFilter(&SampleEditor::tool_runScript, par);
	prepareUndo();

	int ok	= this->script.init(fin, fout);
	ok = this->script.exec(file, fin, fout);								// MILKYMACRO-FILE OR EXTERNAL SCRIPTING LANGUAGE
	if ( this->script.outputFile ) this->script.update(fin, fout);
	
	finishUndo();
	postFilter();
}

void SampleEditor::tool_synthRandomNote(const FilterParameters* par) {
	FilterParameters _par(1);
	_par.setParameter(0, FilterParameters::Parameter( "random" ) );
	tool_synth(&_par);
}

void SampleEditor::tool_synth(const FilterParameters* par)
{
	SoundGenerator g;
	bool seamless = false;
	pp_int32 sLength = 19000;
	pp_int32 sStart = 0;
	pp_int32 sEnd = sLength;
	float* buf;
	buf = (float*)malloc(sLength * sizeof(float));

	if (isEmptySample() || sample->samplen != sLength) {
		sample->samplen = sLength;
		sample->loopstart = 0;
		sample->looplen = sample->samplen;
		sample->type |= 16;
		sample->sample = (mp_sbyte*)module->allocSampleMem(sample->samplen * 2);
		memset(sample->sample, 0, sample->samplen * 2);
	}
	setRelNoteNum(24);

	if ( string(par->getParameter(0).stringPart) == string("random")) {
		srand(time(NULL)); // randomize seed
		int r1 = rand() % 5;
		int r2 = rand() % 3;
		bool reverb = (rand() % 3) == 0;
		bool flange = (rand() % 3) == 0;
		seamless = (r2 == 0);
		int ok = g.generateRandom( &buf[0], sLength, seamless );
		for (int i = 0; i < sLength; i++) {
			setFloatSampleInWaveform(sStart + i, buf[i]);
		}
		// apply random fx
		switch (r1) {
			case 0: {
				FilterParameters _par(2);
				_par.setParameter(0, FilterParameters::Parameter(1.0f) );
				_par.setParameter(1, FilterParameters::Parameter(1) );
				tool_modulateFilterSample(&_par);
				break;
			}
			case 1: {
				FilterParameters _par(2);
				for (int j = 0; j < 2; j++){
					_par.setParameter(0, FilterParameters::Parameter(1));
					_par.setParameter(1, FilterParameters::Parameter(0));
					tool_compressSample(&_par);
				}
				break;
			}
		}

		for (int i = 0; i < sample->samplen; i++) setFloatSampleInWaveform(i, getFloatSampleFromWaveform(i) * 0.66); // lower volume

		if (reverb) {
			float wet = (rand() % 2) == 0 ? 0.5 : 1.0;
			int size = (rand() % 2) == 0 ? 70 : 30;
			FilterParameters _par(2);
			_par.setParameter(0, FilterParameters::Parameter(size));
			_par.setParameter(1, FilterParameters::Parameter(wet));
			tool_reverberateSample(&_par);
			if (wet == 1.0) {
				setSelectionStart(sample->samplen / 10);
				cropSample();
			}
			if( wet == 1.0 && (rand() % 2) == 0 ){
				setSelectionStart(0);
				setSelectionEnd(sample->samplen / 2);
				cropSample();
				seamless = true;
			}
		}
		// limit
		FilterParameters _par(2);
		_par.setParameter(0, FilterParameters::Parameter(0));
		_par.setParameter(1, FilterParameters::Parameter(1));
		tool_compressSample(&_par);
		tool_compressSample(&_par);
		if (seamless) {
			FilterParameters _par(2);
			_par.setParameter(0, FilterParameters::Parameter(50));
			_par.setParameter(1, FilterParameters::Parameter(50));
			tool_seamlessLoopSample(&_par);
		}
	}else { // run predefined synth
		g.init( string(par->getParameter(0).stringPart) );
		int ok = g.generate(&buf[0], sLength);
		for (int i = 0; i < sLength; i++) {
			setFloatSampleInWaveform(sStart + i, buf[i]);
		}
	}
	free(buf);

	setSelectionStart(0);
	setSelectionEnd(sample->samplen);
	loopRange();
	setLoopType(seamless ? 1 : 2);
	lastOperation = OperationNew; // force updating the sampleview

	// push undo/redo here (otherwise 'redo filter' can't reload the script since its buried by all the earlier actions pushed to the undotrack)
	preFilter(&SampleEditor::tool_synth, par);
	prepareUndo();
	finishUndo();
	postFilter();

}


bool SampleEditor::tool_canApplyLastFilter() const
{
	return lastFilterFunc != NULL && isValidSample(); 
}

void SampleEditor::tool_applyLastFilter()
{
	if (lastFilterFunc)
	{
		if (lastParameters)
		{
			FilterParameters newPar(*lastParameters);
			(this->*lastFilterFunc)(&newPar);
		}
		else
		{
			(this->*lastFilterFunc)(NULL);
		}
	}
}

pp_uint32 SampleEditor::convertSmpPosToMillis(pp_uint32 pos, pp_int32 relativeNote/* = 0*/)
{
	if (!isValidSample())
		return 0;
			
	relativeNote+=sample->relnote;
	
	double c4spd = XModule::getc4spd(relativeNote, sample->finetune);
	
	return (pp_uint32)(((double)pos / c4spd) * 1000.0);
}

