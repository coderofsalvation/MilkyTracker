#pragma once

bool Tracker::runMacro(MilkyMacro* m, PPString line) {
	std::string Line = string(line.getStrBuffer());
	float v[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

	if (m->MACRO("volume %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((float)v[0]));
		par.setParameter(1, FilterParameters::Parameter((float)v[0]));
		this->getSampleEditor()->tool_scaleSample(&par);
		return true;
	}

	if (m->MACRO("compress %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		this->getSampleEditor()->tool_compressSample(&par);
		return true;
	}

	if (m->MACRO("reverb %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((float)v[1]));
		this->getSampleEditor()->tool_reverberateSample(&par);
		return true;
	}

	if (m->MACRO("fade %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((float)v[0]));
		par.setParameter(1, FilterParameters::Parameter((float)v[1]));
		getSampleEditor()->tool_scaleSample(&par);
		return true;
	}

	if (m->MACRO("new %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		getSampleEditor()->tool_newSample(&par);
		return true;
	}

	if (m->MACRO("noise %f", (float*)&v, Line)) {
		FilterParameters par(1);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		getSampleEditor()->tool_generateNoise(&par);
		return true;
	}

	if (m->MACRO("sawtooth %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((float)v[0]));
		par.setParameter(1, FilterParameters::Parameter((float)v[1]));
		getSampleEditor()->tool_generateSawtooth(&par);
		return true;
	}

	if (m->MACRO("sweep %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((float)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		getSampleEditor()->tool_modulateFilterSample(&par);
		return true;
	}

	if (m->MACRO("lowpass %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		par.setParameter(2, FilterParameters::Parameter(0));
		getSampleEditor()->tool_resonantFilterSample(&par);
		return true;
	}

	if (m->MACRO("highpass %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		par.setParameter(2, FilterParameters::Parameter(1));
		getSampleEditor()->tool_resonantFilterSample(&par);
		return true;
	}

	if ( Line == string("ptboost") ){
		FilterParameters par(0);
		getSampleEditor()->tool_PTboostSample(&par);
		return true;
	}

	if (Line == string("normalize")) {
		FilterParameters par(0);
		getSampleEditor()->tool_normalizeSample(&par);
		return true;
	}

	if (m->MACRO("bandpass %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		par.setParameter(2, FilterParameters::Parameter(2));
		getSampleEditor()->tool_resonantFilterSample(&par);
		return true;
	}

	if (m->MACRO("bitshift %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		int offset = (int)v[0];
		int shift  = (int)v[1];
		offset = offset > 1 ? 1 : offset;
		offset = offset < 0 ? 0 : offset;
		shift = shift > 8 ? 8 : shift;
		shift = shift < 0 ? 0 : shift;
		par.setParameter(0, FilterParameters::Parameter(offset));
		par.setParameter(1, FilterParameters::Parameter(shift));
		char msg[255];
		getSampleEditor()->tool_bitshiftSample(&par);
		return true;
	}

	if (m->MACRO("notch %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(1, FilterParameters::Parameter((int)v[1]));
		par.setParameter(2, FilterParameters::Parameter(3));
		getSampleEditor()->tool_resonantFilterSample(&par);
		return true;
	}

	if (m->MACRO("select %f %f", (float*)&v, Line)) {
		float length = (float)getSampleEditor()->getSampleLen();
		getSampleEditor()->setSelectionStart((int)(v[0] * length));
		getSampleEditor()->setSelectionEnd((int)(v[1] * length));
		return true;
	}

	if (m->MACRO("alert %s", (float*)&v, Line)) {
		showMessageBoxSized(MESSAGEBOX_UNIVERSAL, Line.substr(6, Line.length()).c_str(), Tracker::MessageBox_OK);
		return true;
	}

	if (m->MACRO("instrument %f", (float*)&v, Line)) {
		listBoxInstruments->setSelectedIndex(((int)v[0]) - 1);
		moduleEditor->setCurrentInstrumentIndex(((int)v[0]) - 1);
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (m->MACRO("sample.volume %f", (float*)&v, Line)) {
		int vol = (int)v[0];
		if (vol > 64) vol = 64;
		if (vol < 0) vol = 0;
		getSampleEditor()->setFT2Volume(vol);
		return true;
	}

	if (m->MACRO("sample.finetune %f", (float*)&v, Line)) {
		int ft = (int)v[0];
		if (ft > 128) ft = 128;
		if (ft < -128) ft = -128;
		getSampleEditor()->setFinetune(ft);
		return true;
	}

	if (m->MACRO("sample.panning %f", (float*)&v, Line)) {
		int ft = (int)v[0];
		if (ft > 255) ft = 255;
		if (ft < 0) ft = 0;
		getSampleEditor()->setPanning(ft);
		return true;
	}

	if (m->MACRO("sample %f", (float*)&v, Line)) {
		listBoxSamples->setSelectedIndex((pp_int32)v[0]);
		moduleEditor->setCurrentSampleIndex((pp_int32)v[0]);
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (m->MACRO("synth %s", (float*)&v, Line)) {
		FilterParameters par(1);
		string synthdef = Line.substr(6);
		par.setParameter(0, FilterParameters::Parameter( (char *)synthdef.c_str() ) );
		getSampleEditor()->tool_synth(&par);
		return true;
	}

	if (m->MACRO("looptype %f", (float*)&v, Line)) {
		getSampleEditor()->setLoopType( (int)v[0] );
		return true;
	}

	if( Line == string("sample_next") || Line == string("sample_previous") ){
		mp_sint32 index = moduleEditor->getCurrentSampleIndex();
		if (Line == string("sample_next")) index++;
		else index--;
		listBoxSamples->setSelectedIndex(index);
		moduleEditor->setCurrentSampleIndex(index);
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (Line == string("instrument_next") || Line == string("instrument_previous")) {
		mp_sint32 index = moduleEditor->getCurrentInstrumentIndex();
		if (Line == string("instrument_next")) index++;
		else index--;
		listBoxInstruments->setSelectedIndex(index);
		moduleEditor->setCurrentInstrumentIndex(index);
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (m->MACRO("seamless %f %f", (float*)&v, Line)) {
		FilterParameters par(2);
		par.setParameter(0, FilterParameters::Parameter((int)v[0]));
		par.setParameter(0, FilterParameters::Parameter((int)v[1]));
		getSampleEditor()->tool_seamlessLoopSample(&par);
		return true;
	}

	if (m->MACRO("relnote %f", (float*)&v, Line)) {
		getSampleEditor()->setRelNoteNum( (int)v[0] );
		return true;
	}

	if (m->MACRO("loop %f %f", (float*)&v, Line)) {
		SampleEditor *s = getSampleEditor();
		float length = (float)s->getSampleLen();
		s->setSelectionStart( (int)(v[0] * length) );
		s->setSelectionEnd( (int)(v[1] * length) );
		s->loopRange();
		return true;
	}

	if (Line == "copy") {
		getSampleEditor()->copy();
		return true;
	}

	if (Line == "paste") {
		getSampleEditor()->paste();
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (Line == "crop") {
		getSampleEditor()->cropSample();
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (Line == "paste_mix") {
		getSampleEditor()->mixPasteSample();
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	if (Line == "selectall") {
		getSampleEditor()->selectAll();
		updateSampleEditor(true); // loads new wave in editor
		return true;
	}

	return false;
}
