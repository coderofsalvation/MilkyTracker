/*
 *  tracker/SampleEditorControlToolHandler.cpp
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
 *  SampleEditorControlToolHandler.cpp
 *  MilkyTracker
 *
 *  Created by Peter Barth on 20.10.05.
 *
 */

#include "SampleEditorControl.h"
#include "DialogWithValues.h"
#include "DialogResample.h"
#include "DialogGroupSelection.h"
#include "DialogEQ.h"
#include "SimpleVector.h"
#include "FilterParameters.h"

bool SampleEditorControl::invokeToolParameterDialog(SampleEditorControl::ToolHandlerResponder::SampleToolTypes type)
{
	if (dialog)
	{
		delete dialog;
		dialog = NULL;
	}
	
	toolHandlerResponder->setSampleToolType(type);
	
	switch (type)
	{
		case ToolHandlerResponder::SampleToolTypeNew:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Create new sample" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Enter size in samples:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(3, 1024*1024*10-1, 0); 
			if (lastValues.newSampleSize != SampleEditorControlLastValues::invalidIntValue())
				static_cast<DialogWithValues*>(dialog)->setValueOne((float)lastValues.newSampleSize);
			else
				static_cast<DialogWithValues*>(dialog)->setValueOne(100.0f);
			break;
			
		case ToolHandlerResponder::SampleToolTypeVolume:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Boost sample volume" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Enter new volume in percent:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(-10000.0f, 10000.0f, 2); 
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.boostSampleVolume != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.boostSampleVolume : 100.0f);
			break;

		case ToolHandlerResponder::SampleToolTypeCompress:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Compress sample" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("type:  [0=clean 1=dirty]:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, 1, 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("output:  [0=compensate 1=limit]:");
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0, 1, 0);

			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.compress != SampleEditorControlLastValues::invalidIntValue() ? lastValues.compress : 0 );
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.compressCompensate != SampleEditorControlLastValues::invalidIntValue() ? lastValues.compressCompensate : 0);
			break;

		case ToolHandlerResponder::SampleToolTypeFade:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Fade sample" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Enter start volume in percent:");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Enter end volume in percent:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(-10000.0f, 10000.0f, 2); 
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(-10000.0f, 10000.0f, 2); 
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.fadeSampleVolumeStart != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.fadeSampleVolumeStart : 100.0f);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.fadeSampleVolumeEnd != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.fadeSampleVolumeEnd : 100.0f);
			break;
		
		case ToolHandlerResponder::SampleToolTypeDecimate:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Decimate sample" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Bits [1..16]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Decimate [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(1, 16, 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0.0f, 1.0f, 2);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.decimateBits != SampleEditorControlLastValues::invalidIntValue() ? lastValues.decimateBits : 16);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.decimateRate != SampleEditorControlLastValues::invalidIntValue() ? lastValues.decimateRate : 0.5f );
			break;

		case ToolHandlerResponder::SampleToolTypeBitshift:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Bitshift sample" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Byte offset [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, 1, 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Bitshift [0..6]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0, 6, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.bitshiftByte!= SampleEditorControlLastValues::invalidIntValue() ? lastValues.bitshiftByte : 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.bitshift    != SampleEditorControlLastValues::invalidIntValue() ? lastValues.bitshift     : 0);

			break;

		case ToolHandlerResponder::SampleToolTypeReverberate:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Reverb" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Size [1..200]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Dry/wet ratio [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(1, 200, 2);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0.0f, 1.0f, 2);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.reverbSize != SampleEditorControlLastValues::invalidIntValue() ? lastValues.reverbSize : 100);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.reverbRatio != SampleEditorControlLastValues::invalidIntValue() ? lastValues.reverbRatio : 0.5f);
			break;

		case ToolHandlerResponder::SampleToolTypeExcite:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Excite high frequencies" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Freq area [0..2] (>1 modulates)");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Noise [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0.0f, 10.0f, 2);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0.0f, 1.0f, 1);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.exciteFreq != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.exciteFreq : 0.86);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.exciteAliase != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.exciteAliase : 0.0f);
			break;

		case ToolHandlerResponder::SampleToolTypeResonantFilterLP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterHP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterBP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterNOTCH:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Resonant Filter" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Cutoff [0..22500]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Resonance [0-9]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, 22500, 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0, 9, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.filterCutoffA != SampleEditorControlLastValues::invalidIntValue() ? lastValues.filterCutoffA : 250);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.filterResonance != SampleEditorControlLastValues::invalidIntValue() ? lastValues.filterResonance: 3);
			if (type == ToolHandlerResponder::SampleToolTypeResonantFilterLP)    lastValues.filterType = 0;
			if (type == ToolHandlerResponder::SampleToolTypeResonantFilterHP)    lastValues.filterType = 1;
			if (type == ToolHandlerResponder::SampleToolTypeResonantFilterBP)    lastValues.filterType = 2;
			if (type == ToolHandlerResponder::SampleToolTypeResonantFilterNOTCH) lastValues.filterType = 3;
			break;

		case ToolHandlerResponder::SampleToolTypeFilterSweep:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Sweep filter" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Intensity [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Sweeps [1..100]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0.0, 1.0, 1);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(1, 100, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.mFilterRange != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.mFilterRange : 0.2);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.mFilterSweeps != SampleEditorControlLastValues::invalidIntValue() ? lastValues.mFilterSweeps : 1);
			break;

		case ToolHandlerResponder::SampleToolTypeModulateFilter:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Filter Modulate" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Intensity [0..1]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0.0, 1.0, 1);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.mFilterRange != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.mFilterRange : 0.2);
			break;

		case ToolHandlerResponder::SampleToolTypeModulateEnvelope:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Filter Modulate" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Release [0..9]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, 9, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.mFilterSweeps != SampleEditorControlLastValues::invalidIntValue() ? lastValues.mFilterSweeps : 3);
			break;

		case ToolHandlerResponder::SampleToolTypeBassboost:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Bass boost" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("frequency (hz) [30..300]:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(30, 300, 2);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.frequencyBassboost != SampleEditorControlLastValues::invalidIntValue() ? lastValues.frequencyBassboost : 60.0f);
			break;

		case ToolHandlerResponder::SampleToolTypeChangeSign:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Change sign" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Ignore bits from MSB [0..]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, (sampleEditor->is16Bit() ? 16 : 8), 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.changeSignIgnoreBits != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.changeSignIgnoreBits : 1);
			break;

		case ToolHandlerResponder::SampleToolTypeDCOffset:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "DC offset" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Enter offset in percent [-100..100]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(-100, 100.0f, 2); 
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.DCOffset != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.DCOffset : 0.0f);
			break;

		case ToolHandlerResponder::SampleToolTypeResample:
			dialog = new DialogResample(parentScreen, toolHandlerResponder, PP_DEFAULT_ID);
			if (sampleEditor->isValidSample())
			{
				if (lastValues.resampleInterpolationType != SampleEditorControlLastValues::invalidIntValue()) 
					static_cast<DialogResample*>(dialog)->setInterpolationType(lastValues.resampleInterpolationType);

				static_cast<DialogResample*>(dialog)->setAdjustFtAndRelnote(lastValues.adjustFtAndRelnote);

				static_cast<DialogResample*>(dialog)->setRelNote(sampleEditor->getRelNoteNum());
				static_cast<DialogResample*>(dialog)->setFineTune(sampleEditor->getFinetune());
				static_cast<DialogResample*>(dialog)->setSize(sampleEditor->getSampleLen());
			}
			break;

		case ToolHandlerResponder::SampleToolTypeEQ3Band:
			dialog = new DialogEQ(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, DialogEQ::EQ3Bands);
			if (lastValues.hasEQ3BandValues)
			{
				for (pp_int32 i = 0; i < 3; i++)
					static_cast<DialogEQ*>(dialog)->setBandParam(i, lastValues.EQ3BandValues[i]);
			}
			break;

		case ToolHandlerResponder::SampleToolTypeEQ10Band:
		case ToolHandlerResponder::SampleToolTypeSelectiveEQ10Band:
			dialog = new DialogEQ(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, DialogEQ::EQ10Bands);
			if (lastValues.hasEQ10BandValues)
			{
				for (pp_int32 i = 0; i < 10; i++)
					static_cast<DialogEQ*>(dialog)->setBandParam(i, lastValues.EQ10BandValues[i]);
			}
			break;

		case ToolHandlerResponder::SampleToolTypeGenerateSilence:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Insert silence" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Enter size in samples:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(3, 1024*1024*10-1, 0); 
			if (lastValues.silenceSize != SampleEditorControlLastValues::invalidIntValue())
				static_cast<DialogWithValues*>(dialog)->setValueOne((float)lastValues.silenceSize);
			else
				static_cast<DialogWithValues*>(dialog)->setValueOne(100.0f);
			break;

		case ToolHandlerResponder::SampleToolTypeGenerateNoise:
		{
			PPSimpleVector<PPString> noiseFilterTypes;

			noiseFilterTypes.add(new PPString("White"));
			noiseFilterTypes.add(new PPString("Pink"));
			noiseFilterTypes.add(new PPString("Brown"));

			dialog = new DialogGroupSelection(parentScreen, 
											  toolHandlerResponder, 
											  PP_DEFAULT_ID, 
											  "Select noise type" PPSTR_PERIODS,
											  noiseFilterTypes);
			break;
		}
		
		case ToolHandlerResponder::SampleToolTypeGenerateSine:
		case ToolHandlerResponder::SampleToolTypeGenerateSquare:
		case ToolHandlerResponder::SampleToolTypeGenerateTriangle:
		case ToolHandlerResponder::SampleToolTypeGenerateSawtooth:
		{
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Generate waveform" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Volume in percent:");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Number of periods:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(-1000.0f, 1000.0f, 2); 
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0.0f, (float)(1024*1024*5), 2);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.waveFormVolume != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.waveFormVolume : 100.0f);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.waveFormNumPeriods != SampleEditorControlLastValues::invalidFloatValue() ? lastValues.waveFormNumPeriods : 1.0f);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeSeamlessLoop:
		{
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Generate waveform" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("Fade in length [0..50%]:");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("Fade out length [0..50%]:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0,50,0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(0, 50, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.sizeFadeIn != SampleEditorControlLastValues::invalidIntValue() ? lastValues.sizeFadeIn : 50);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.sizeFadeOut != SampleEditorControlLastValues::invalidIntValue() ? lastValues.sizeFadeOut : 50);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeVocode:
		{
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Vocode" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterTwoValues);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("bands [8/16]");
			static_cast<DialogWithValues*>(dialog)->setValueTwoCaption("release [1-6]");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(8, 16, 0);
			static_cast<DialogWithValues*>(dialog)->setValueTwoRange(1, 6, 0);
			static_cast<DialogWithValues*>(dialog)->setValueOne(lastValues.vocodeBands != SampleEditorControlLastValues::invalidIntValue() ? lastValues.vocodeBands : 8);
			static_cast<DialogWithValues*>(dialog)->setValueTwo(lastValues.vocodeEnvelope != SampleEditorControlLastValues::invalidIntValue() ? lastValues.vocodeEnvelope: 1);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeRepeat:
			dialog = new DialogWithValues(parentScreen, toolHandlerResponder, PP_DEFAULT_ID, "Repeat paste" PPSTR_PERIODS, DialogWithValues::ValueStyleEnterOneValue);
			static_cast<DialogWithValues*>(dialog)->setValueOneCaption("how many times:");
			static_cast<DialogWithValues*>(dialog)->setValueOneRange(0, 1024, 0);
			if (lastValues.bitshift != SampleEditorControlLastValues::invalidIntValue())
				static_cast<DialogWithValues*>(dialog)->setValueOne((float)lastValues.bitshift);
			else
				static_cast<DialogWithValues*>(dialog)->setValueOne(2);
			break;

		default:
			break;
	}
	
	dialog->show();
	
	return true;
}

bool SampleEditorControl::invokeTool(ToolHandlerResponder::SampleToolTypes type)
{
	if (!sampleEditor->isValidSample())
		return false;

	switch (type)
	{
		case ToolHandlerResponder::SampleToolTypeNew:
		{
			FilterParameters par(2);
			lastValues.newSampleSize = (pp_int32)static_cast<DialogWithValues*>(dialog)->getValueOne();
			par.setParameter(0, FilterParameters::Parameter(lastValues.newSampleSize));
			par.setParameter(1, FilterParameters::Parameter(sampleEditor->is16Bit() ? 16 : 8));
			sampleEditor->tool_newSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeVolume:
		{
			lastValues.boostSampleVolume = static_cast<DialogWithValues*>(dialog)->getValueOne();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.boostSampleVolume / 100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.boostSampleVolume / 100.0f));
			sampleEditor->tool_scaleSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeCompress:
		{
			lastValues.compress = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.compressCompensate = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.compress) );
			par.setParameter(1, FilterParameters::Parameter(lastValues.compressCompensate));
			sampleEditor->tool_compressSample(&par);
			break;
		}


		case ToolHandlerResponder::SampleToolTypeFade:
		{
			lastValues.fadeSampleVolumeStart = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.fadeSampleVolumeEnd = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.fadeSampleVolumeStart / 100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.fadeSampleVolumeEnd / 100.0f));
			sampleEditor->tool_scaleSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeDecimate:
		{
			lastValues.decimateBits = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.decimateRate = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.decimateBits ));
			par.setParameter(1, FilterParameters::Parameter(lastValues.decimateRate ));
			sampleEditor->tool_decimateSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeBitshift:
		{
			lastValues.bitshiftByte = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.bitshift     = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.bitshiftByte));
			par.setParameter(1, FilterParameters::Parameter(lastValues.bitshift));
			sampleEditor->tool_bitshiftSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeRepeat:
		{
			lastValues.bitshift = static_cast<DialogWithValues*>(dialog)->getValueOne();
			FilterParameters par(1);
			par.setParameter(0, FilterParameters::Parameter(lastValues.bitshift));
			sampleEditor->tool_repeatSample(&par);
			break;
		}

		

		case ToolHandlerResponder::SampleToolTypeReverberate:
		{
			lastValues.reverbSize = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.reverbRatio = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.reverbSize));
			par.setParameter(1, FilterParameters::Parameter(lastValues.reverbRatio));
			sampleEditor->tool_reverberateSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeResonantFilterLP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterHP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterBP:
		case ToolHandlerResponder::SampleToolTypeResonantFilterNOTCH:
		{
			lastValues.filterCutoffA = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.filterResonance = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(5);
			par.setParameter(0, FilterParameters::Parameter(lastValues.filterCutoffA));
			par.setParameter(1, FilterParameters::Parameter(lastValues.filterResonance));
			par.setParameter(2, FilterParameters::Parameter(lastValues.filterType));
			par.setParameter(3, FilterParameters::Parameter(lastValues.filterCutoffB));
			sampleEditor->tool_resonantFilterSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeFilterSweep:
		case ToolHandlerResponder::SampleToolTypeModulateFilter:
		{
			lastValues.mFilterRange = static_cast<DialogWithValues*>(dialog)->getValueOne();
      lastValues.mFilterSweeps = (type == ToolHandlerResponder::SampleToolTypeFilterSweep) ?
                                 static_cast<DialogWithValues*>(dialog)->getValueTwo()     :
                                 0;
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.mFilterRange));
			par.setParameter(1, FilterParameters::Parameter(lastValues.mFilterSweeps));
			sampleEditor->tool_modulateFilterSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeModulateEnvelope:
		{
			lastValues.mFilterSweeps = static_cast<DialogWithValues*>(dialog)->getValueOne();
			FilterParameters par(5);
			par.setParameter(0, FilterParameters::Parameter(lastValues.mFilterSweeps));
			sampleEditor->tool_modulateEnvelopeSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeBassboost:
		{
			lastValues.frequencyBassboost = static_cast<DialogWithValues*>(dialog)->getValueOne();
			FilterParameters par(1);
			par.setParameter(0, FilterParameters::Parameter(lastValues.frequencyBassboost));
			sampleEditor->tool_bassboostSample(&par);
			break;
		}
		
		case ToolHandlerResponder::SampleToolTypeVocode:
		{
			lastValues.vocodeBands = (pp_int32)static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.vocodeEnvelope = (pp_int32)static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.vocodeBands));
			par.setParameter(1, FilterParameters::Parameter(lastValues.vocodeEnvelope));
			sampleEditor->tool_vocodeSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeExcite:
		{
			lastValues.exciteFreq = (float)static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.exciteAliase = (float)static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.exciteFreq));
			par.setParameter(1, FilterParameters::Parameter(lastValues.exciteAliase));
			sampleEditor->tool_exciteSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeChangeSign:
		{
			lastValues.changeSignIgnoreBits = (pp_int32)static_cast<DialogWithValues*>(dialog)->getValueOne();
			FilterParameters par(1);
			par.setParameter(0, FilterParameters::Parameter(lastValues.changeSignIgnoreBits));
			sampleEditor->tool_changeSignSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeDCOffset:
		{
			FilterParameters par(1);
			lastValues.DCOffset = static_cast<DialogWithValues*>(dialog)->getValueOne();
			par.setParameter(0, FilterParameters::Parameter(lastValues.DCOffset / 100.0f));
			sampleEditor->tool_DCOffsetSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeResample:
		{
			lastValues.resampleInterpolationType = static_cast<DialogResample*>(dialog)->getInterpolationType();
			lastValues.adjustFtAndRelnote = static_cast<DialogResample*>(dialog)->getAdjustFtAndRelnote();

			FilterParameters par(3);
			par.setParameter(0, FilterParameters::Parameter(static_cast<DialogResample*>(dialog)->getC4Speed()));
			par.setParameter(1, FilterParameters::Parameter(static_cast<pp_int32>(lastValues.resampleInterpolationType)));
			par.setParameter(2, FilterParameters::Parameter(lastValues.adjustFtAndRelnote ? 1 : 0));
			sampleEditor->tool_resampleSample(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeEQ3Band:
		case ToolHandlerResponder::SampleToolTypeEQ10Band:
		case ToolHandlerResponder::SampleToolTypeSelectiveEQ10Band:
		{
			pp_uint32 numBands = static_cast<DialogEQ*>(dialog)->getNumBandsAsInt();
			
			float* last = NULL;
			
			if (numBands == 3)
			{
				lastValues.hasEQ3BandValues = true;
				last = lastValues.EQ3BandValues;
			}
			else
			{
				lastValues.hasEQ10BandValues = true;
				last = lastValues.EQ10BandValues;
			}
			
			FilterParameters par(numBands);
			for (pp_uint32 i = 0; i < numBands; i++)
			{
				float val = static_cast<DialogEQ*>(dialog)->getBandParam(i);
				if (last)
					last[i] = val;
				par.setParameter(i, FilterParameters::Parameter(val));
			}
			sampleEditor->tool_eqSample(&par,type==ToolHandlerResponder::SampleToolTypeSelectiveEQ10Band);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeGenerateSilence:
		{
			FilterParameters par(1);
			par.setParameter(0, FilterParameters::Parameter((pp_int32)(static_cast<DialogWithValues*>(dialog)->getValueOne())));
			sampleEditor->tool_generateSilence(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeGenerateNoise:
		{
			FilterParameters par(1);
			par.setParameter(0, FilterParameters::Parameter((pp_int32)(static_cast<DialogGroupSelection*>(dialog)->getSelection())));
			sampleEditor->tool_generateNoise(&par);
			break;
		}
		
		case ToolHandlerResponder::SampleToolTypeGenerateSine:
		{
			lastValues.waveFormVolume = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.waveFormNumPeriods = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.waveFormVolume/100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.waveFormNumPeriods));
			sampleEditor->tool_generateSine(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeSeamlessLoop:
		{
			lastValues.sizeFadeIn = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.sizeFadeOut = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.sizeFadeIn));
			par.setParameter(1, FilterParameters::Parameter(lastValues.sizeFadeOut));
			sampleEditor->tool_seamlessLoopSample(&par);
			break;
		}
		
		case ToolHandlerResponder::SampleToolTypeGenerateSquare:
		{
			lastValues.waveFormVolume = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.waveFormNumPeriods = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.waveFormVolume/100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.waveFormNumPeriods));
			sampleEditor->tool_generateSquare(&par);
			break;
		}
		
		case ToolHandlerResponder::SampleToolTypeGenerateTriangle:
		{
			lastValues.waveFormVolume = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.waveFormNumPeriods = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.waveFormVolume/100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.waveFormNumPeriods));
			sampleEditor->tool_generateTriangle(&par);
			break;
		}

		case ToolHandlerResponder::SampleToolTypeGenerateSawtooth:
		{
			lastValues.waveFormVolume = static_cast<DialogWithValues*>(dialog)->getValueOne();
			lastValues.waveFormNumPeriods = static_cast<DialogWithValues*>(dialog)->getValueTwo();
			FilterParameters par(2);
			par.setParameter(0, FilterParameters::Parameter(lastValues.waveFormVolume/100.0f));
			par.setParameter(1, FilterParameters::Parameter(lastValues.waveFormNumPeriods));
			sampleEditor->tool_generateSawtooth(&par);
			break;
		}
		default:
			break;
	}
	
	return true;
}

SampleEditorControl::ToolHandlerResponder::ToolHandlerResponder(SampleEditorControl& theSampleEditorControl) :
	sampleEditorControl(theSampleEditorControl),
	sampleToolType(SampleToolTypeNone)
{
}

pp_int32 SampleEditorControl::ToolHandlerResponder::ActionOkay(PPObject* sender)
{
	sampleEditorControl.invokeTool(sampleToolType);
	return 0;
}

pp_int32 SampleEditorControl::ToolHandlerResponder::ActionCancel(PPObject* sender)
{
	return 0;
}
