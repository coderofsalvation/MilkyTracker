/*
 *  tracker/SampleEditorControlLastValues.h
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

#ifndef __SAMPLEEDITORCONTROLASTVALUES_H__
#define __SAMPLEEDITORCONTROLASTVALUES_H__

#include "BasicTypes.h"
#include "Dictionary.h"

// Last values
struct SampleEditorControlLastValues
{
	pp_int32 newSampleSize;
	pp_int32 changeSignIgnoreBits;
	pp_int32 frequencyBassboost;
	pp_int32 frequencyExcite;

	pp_int32 reverbSize;
	float reverbRatio;

	pp_int32 sizeFadeIn;
	pp_int32 sizeFadeOut;

	pp_int32 vocodeBands;
	pp_int32 vocodeEnvelope;

	pp_int32 filterCutoffA;
	pp_int32 filterCutoffB;
	pp_int32 filterResonance;
	pp_int32 filterType;

	pp_int32 mFilterType;
	pp_int32 mFilterRelease;

	int bitshift;
	int bitshiftByte;

	float exciteFreq;
	float exciteAliase;
	
	float boostSampleVolume;
	float fadeSampleVolumeStart;
	float fadeSampleVolumeEnd;
	pp_int32 decimateBits;
	float decimateRate;
	float DCOffset;
	pp_int32 silenceSize;
	float waveFormVolume;
	float waveFormNumPeriods;
	
	bool hasEQ3BandValues;
	float EQ3BandValues[3];
	
	bool hasEQ10BandValues;
	float EQ10BandValues[10];

	bool hasFilterFullValues;
	float FilterFullValues[3];
	
	pp_int32 resampleInterpolationType;
	bool adjustFtAndRelnote;
	
	static float invalidFloatValue() 
	{
		return -12345678.0f;
	}
	
	static int invalidIntValue() 
	{
		return -12345678;
	}
	
	void reset()
	{
		newSampleSize = invalidIntValue();
		decimateBits = invalidIntValue();
		decimateRate = invalidFloatValue();
		frequencyBassboost = invalidIntValue();
		changeSignIgnoreBits = invalidIntValue();
		boostSampleVolume = invalidFloatValue();
		fadeSampleVolumeStart = invalidFloatValue();
		fadeSampleVolumeEnd = invalidFloatValue();
		DCOffset = invalidFloatValue();
		silenceSize = invalidIntValue();
		waveFormVolume = invalidFloatValue();
		waveFormNumPeriods = invalidFloatValue();
		hasEQ3BandValues = hasEQ10BandValues = false;
		resampleInterpolationType = invalidIntValue();
		adjustFtAndRelnote = true;

		reverbSize = invalidIntValue();
		reverbRatio = invalidFloatValue();

		sizeFadeIn = invalidIntValue();
		sizeFadeOut = invalidIntValue();

		filterCutoffA = invalidIntValue();
		filterCutoffB = invalidIntValue();
		filterResonance = invalidIntValue();
		filterType = invalidIntValue();


		mFilterType = invalidIntValue();
		mFilterRelease  = invalidIntValue();

		vocodeBands = invalidIntValue();
		vocodeEnvelope = invalidIntValue();

		exciteFreq = invalidFloatValue();
		exciteAliase = invalidFloatValue();

		bitshift = invalidIntValue();
		bitshiftByte = invalidIntValue();
	}
		
	PPDictionary convertToDictionary()
	{
		PPDictionary result;
		
		result.store("newSampleSize", newSampleSize);

		result.store("decimateBits", decimateBits);

		result.store("decimateRate", decimateRate);

		result.store("reverbSize", reverbSize);
		result.store("reverbRatio", reverbRatio);

		result.store("filterCutoffA", filterCutoffA);
		result.store("filterCutoffB", filterCutoffB);

		result.store("vocodeBands", vocodeBands);
		result.store("vocodeEnvelope", vocodeEnvelope);

		result.store("exciteFreq", exciteFreq);
		result.store("exciteAliase", exciteAliase);

		result.store("sizeFadeIn", sizeFadeIn);
		result.store("sizeFadeOut", sizeFadeOut);

		result.store("mFilterType", mFilterType);
		result.store("mFilterRelease", mFilterRelease);

		result.store("frequencyBassboost", frequencyBassboost);

		result.store("changeSignIgnoreBits", changeSignIgnoreBits);

		result.store("boostSampleVolume", PPDictionary::convertFloatToIntNonLossy(boostSampleVolume));

		result.store("fadeSampleVolumeStart", PPDictionary::convertFloatToIntNonLossy(fadeSampleVolumeStart));
		result.store("fadeSampleVolumeEnd", PPDictionary::convertFloatToIntNonLossy(fadeSampleVolumeEnd));

		result.store("DCOffset", PPDictionary::convertFloatToIntNonLossy(DCOffset));

		result.store("silenceSize", silenceSize);

		result.store("waveFormVolume", PPDictionary::convertFloatToIntNonLossy(waveFormVolume));
		result.store("waveFormNumPeriods", PPDictionary::convertFloatToIntNonLossy(waveFormNumPeriods));

		result.store("resampleInterpolationType", resampleInterpolationType);
		
		result.store("adjustFtAndRelnote", adjustFtAndRelnote);

		result.store("bitshift", bitshift);
		result.store("bitshiftByte", bitshiftByte);
		
		
		return result;
	}
	
	void restoreFromDictionary(PPDictionary& dictionary)
	{
		PPDictionaryKey* key = dictionary.getFirstKey();
		while (key)
		{
			if (key->getKey().compareToNoCase("newSampleSize") == 0)
			{
				newSampleSize = key->getIntValue();
			}
			else if (key->getKey().compareToNoCase("changeSignIgnoreBits") == 0)
			{
				changeSignIgnoreBits = key->getIntValue();
			}
			else if (key->getKey().compareToNoCase("boostSampleVolume") == 0)
			{
				boostSampleVolume = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("fadeSampleVolumeStart") == 0)
			{
				fadeSampleVolumeStart = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("fadeSampleVolumeEnd") == 0)
			{
				fadeSampleVolumeEnd = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("DCOffset") == 0)
			{
				DCOffset = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("silenceSize") == 0)
			{
				silenceSize = key->getIntValue();
			}
			else if (key->getKey().compareToNoCase("waveFormVolume") == 0)
			{
				waveFormVolume = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("waveFormNumPeriods") == 0)
			{
				waveFormNumPeriods = PPDictionary::convertIntToFloatNonLossy(key->getIntValue());
			}
			else if (key->getKey().compareToNoCase("resampleInterpolationType") == 0)
			{
				resampleInterpolationType = key->getIntValue();
			}
			else if (key->getKey().compareToNoCase("adjustFtAndRelnote") == 0)
			{
				adjustFtAndRelnote = key->getBoolValue();
			}
		
			key = dictionary.getNextKey();
		}
	}
};

#endif
