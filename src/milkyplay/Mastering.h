/*
 * Copyright (c) 2009, The MilkyTracker Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of the <ORGANIZATION> nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Mastering.h
 *  MilkyPlay
 *
 *  Created by Leon van Kammen on 7 mei 2022 16:57:13 CEST
 *
 *  This class is pretty much allowing milkytracker to output 'beefier' tracks.
 *  Enough with the 90s muddy flat mixes, let's step up our game (without introducing VST plugins etc).
 */
#ifndef __MASTERING_H__
#define __MASTERING_H__

#include "Mixable.h"
#include "../tracker/SampleEditorFx.h"

#define ZEROCROSSING(a,b) (a >= 0.0 && b <= 0.0 || a <= 0.0 && b >= 0.0 )
#define NUM_CHUNKS 16
#define BUFFER_TIME 0.0053
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define CO_DB(v) (20.0f * log10f(v))

// ported awesome fastlookahead limiter by steve harris @ https://github.com/swh/ladspa
struct Limiter : public Mixable
{
  float fmax;
  mp_uint32 preset; // samplerate
  mp_uint32 fs; // samplerate
  mp_uint32 buffer_len; 
  mp_uint32 buffer_pos; 
  mp_uint32 delay; 
  mp_uint32 chunk_pos; 
  mp_uint32 chunk_num; 
  mp_uint32 chunk_size; 
  float peak;
  float atten;
  float atten_lp;
  float delta;
  float *buffer;
  float *chunks;
  float limit;
  float release;
  float ingain; // -20 .. 20 
  float attenuation; // output gain 
	
	Limiter() : 
    fs(-1),
    fmax(32678.0f),
    limit(-0.2),
    release(0.01),
    ingain(7.0),
    attenuation(0.0)
	{
	}

  void init(mp_uint32 s_rate){
    cleanup();
    fs = s_rate;
    buffer_len = 128;
    buffer_pos = 0;
    /* Find size for power-of-two interleaved delay buffer */
    while(buffer_len < fs * BUFFER_TIME * 2) {
      buffer_len *= 2;
    }
    buffer = (float *)calloc(buffer_len, sizeof(float));
    delay = (int)(0.005 * fs);
    chunk_pos = 0;
    chunk_num = 0;
    /* find a chunk size (in smaples) thats roughly 0.5ms */
    chunk_size = s_rate / 2000; 
    chunks = (float *)calloc(NUM_CHUNKS, sizeof(float));
    peak = 0.0f;
    atten = 1.0f;
    atten_lp = 1.0f;
    delta = 0.0f;
    memset(buffer, 0, NUM_CHUNKS * sizeof(float));    
  }

  void round_to_zero(volatile float *f){
    *f += 1e-18;
    *f -= 1e-18;
  }

  void cleanup(){
    if( fs == -1 ) return;
    free(buffer);
    free(chunks);
  }

  void buffer_write( mp_sint32 *buffer, mp_uint32 pos, float max,float v){
      round_to_zero(&v);
      if (v < -max) v = -max;
      else if (v > max) v = max;
      buffer[pos] = (mp_sint32)(v*fmax);
  }

  virtual void mix(mp_sint32 *inbuffer, mp_uint32 sample_count)
  {
    ingain = preset == 0 ? 5.0 : 13.0;
    unsigned long pos;
    const float max = DB_CO(limit);
    const float trim = DB_CO(ingain);
    float sig;
    unsigned int i;
    mp_uint32 posL;
    mp_uint32 posR;

    for (pos = 0; pos < sample_count; pos++)
    {
      posL = pos*2;
      posR = posL+1;

      if (chunk_pos++ == chunk_size)
      {
        /* we've got a full chunk */

        delta = (1.0f - atten) / (fs * release);
        round_to_zero(&delta);
        for (i = 0; i < 10; i++)
        {
          const int p = (chunk_num - 9 + i) & (NUM_CHUNKS - 1);
          const float this_delta = (max / chunks[p] - atten) /
                                   ((float)(i + 1) * fs * 0.0005f + 1.0f);
          if (this_delta < delta)
          {
            delta = this_delta;
          }
        }
        chunks[chunk_num++ & (NUM_CHUNKS - 1)] = peak;
        peak = 0.0f;
        chunk_pos = 0;
      }

      float in_1 = (float)inbuffer[posL]*(1.0/fmax);
      float in_2 = (float)inbuffer[posR]*(1.0/fmax);
      
      buffer[(buffer_pos * 2) & (buffer_len - 1)]     = in_1 * trim + 1.0e-30;
      buffer[(buffer_pos * 2 + 1) & (buffer_len - 1)] = in_2 * trim + 1.0e-30;

      sig = fabs(in_1) > fabs(in_2) ? fabs(in_1) : fabs(in_2);
      sig += 1.0e-30;
      if (sig * trim > peak)
      {
        peak = sig * trim;
      }
      // round_to_zero(&peak);
      // round_to_zero(&sig);
      atten += delta;
      atten_lp = atten * 0.1f + atten_lp * 0.9f;
      // round_to_zero(&atten_lp);
      if (delta > 0.0f && atten > 1.0f)
      {
        atten = 1.0f;
        delta = 0.0f;
      }
      
      buffer_write(inbuffer,posL, max,buffer[(buffer_pos * 2 - delay * 2) &
                                      (buffer_len - 1)] *
                                   atten_lp);
      buffer_write(inbuffer,posR, max,buffer[(buffer_pos * 2 - delay * 2 + 1) &
                                      (buffer_len - 1)] *
                                   atten_lp);
      buffer_pos++;
  
    }
    buffer_pos = buffer_pos;
    peak = peak;
    atten = atten;
    atten_lp = atten_lp;
    chunk_pos = chunk_pos;
    chunk_num = chunk_num;
    //*(plugin_data->attenuation) = -CO_DB(atten);
    //*(plugin_data->latency) = delay;
  }
} masteringLimiter;

struct CompandFilter : public Mixable
{
  float max;
  mp_uint32 sampleRate;
  // treble+bass boost
  struct Filter bL;
  struct Filter bR;
  struct Filter tL;
  struct Filter tR;
  // companding
  mp_sint32 preset;
	
	CompandFilter() : 
    max(32678.0f)
	{
    bL.q = bR.q = tL.q = tR.q = 0.3;
    bL.cutoff = bR.cutoff = 60;
    tL.cutoff = tR.cutoff = 98000;
	}

  void init(mp_uint32 s_rate){
    sampleRate = s_rate ;
    filter_init(&bL, sampleRate );
    filter_init(&bR, sampleRate );
    filter_init(&tL, sampleRate );
    filter_init(&tR, sampleRate );
  }
	
	virtual void mix(mp_sint32* buffer, mp_uint32 bufferSize)
	{
    if( preset < 1 ) return;
    mp_uint32 i;
    mp_uint32 j;
    for( i = 0; i < bufferSize*MP_NUMCHANNELS; i+=2 ){
      float srcL = (float)buffer[i  ]*(1.0/max);
      float srcR = (float)buffer[i+1]*(1.0/max);
      // compand using mulaw-ish curve
      if( preset == 2 || preset == 3 ){
        float amp = (float)(preset-1);
        filter(&tL, tanh(srcL*8) ); // excite treble
        filter(&tR, tanh(srcR*8) ); //
        filter(&bL, tanh(srcL*8) ); // excite lows
        filter(&bR, tanh(srcR*8) ); //
        srcL -= tR.hp * 0.15; // mix in treble (stereowiden + compensate loss of high freqs due to summing low/mid freqs))
        srcR -= tL.hp * 0.15; 
        srcL -= bL.lp * 0.25; // mix in bass (compensate loss of low freqs due to companding)
        srcR -= bR.lp * 0.25; 
        for( j = 0; j < 2; j++ ){
          srcL = asinh(srcL*(float)amp)/asinh((float)amp); // compand to compensate accumulated mid freqs 
          srcR = asinh(srcR*(float)amp)/asinh((float)amp); // due to summing
        }
      }
      if( preset == 1 ){
        srcL = (srcL+srcR)/2.0;
        srcR = (srcL+srcR)/2.0;
      }
      buffer[i]   = (mp_sint32)( srcL  * max);
      buffer[i+1] = (mp_sint32)( srcR  * max);
    }
	}
} masteringCompand;


#endif
