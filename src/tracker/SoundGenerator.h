/* 
 * synth (https://github.com/hsaturn/synth)
 * Copyright (C) 2018  hsaturn 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 2021 modified to make things work with MilkyTracker (Leon van Kammen)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <list>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <chrono>
#include <atomic>

using namespace std;

uint32_t ech = 44100;
const int BUF_LENGTH = 1024;

class SoundGenerator
{
	public:
		virtual ~SoundGenerator() {};

		// Next sample to add to left and right 
		// added value should be from -1 to 1
		// speed = samples/sec modifier
		virtual void next(float &left, float &right, float speed=1.0) = 0;

		static SoundGenerator* factory(istream& in, bool needed=false);
		static string getTypes()
		{
			string s;
			for(auto it: generators)
				s+= it.first+' ';
			return s;
		}

		static float rand()
		{
			return (float)::rand() / ((float)RAND_MAX/2) -1.0;
		}

		static void help()
		{
			map<const SoundGenerator*, bool>	done;
			for(auto generator : generators)
			{
				if (done.find(generator.second)==done.end())
				{
					done[generator.second] = true;
					generator.second->help(cout);
				}
			}
		}

		static string randomNote(){
		  char tpl[255];
		  const int Nw = 3;
		  const char* waves[Nw];
		  waves[0] = "sinus";
		  waves[1] = "square";
		  waves[2] = "tri";
		  const int Nt = 3;
		  const char* templates[Nt];
		  templates[0] = "reverb 200:10 adsr 1:100 200:50 400:50 450:0 500:0 once { %s 220 %s 440:50 %s 880:25 }";
		  templates[1] = "gain 70 reverb 200:10 adsr 1:100 200:50 400:50 450:0 500:0 once { %s 440 %s 880:50 %s 1320:25 }";
		  templates[2] = "gain 100 reverb 200:10 adsr 1:100 110:1 1:50 150:1 150:1 once { %s 880 %s 1320:50 %s 1600:25 }";
		  int n = sprintf(
			  tpl, 
			  templates[ std::rand() % Nt ], 
			  waves[ std::rand() % Nw], 
			  waves[ std::rand() % Nw],
			  waves[ std::rand() % Nw]
		  );
		  return string(tpl);
		}
	
		static string last_type;

		static void missingGeneratorExit(string msg="")
		{
			if (last_type.length())
				cerr << "Unknown generator type (" << last_type << ")" << endl;
			else
				cerr << "Missing generator" << endl;
			if (msg.length())
				cerr << msg;
		}

	protected:
		SoundGenerator() {};

		// Auto register for the factory
		SoundGenerator(string name)
		{
			while(name.length())
			{
				if (name.find(' ')==string::npos)
				{
					generators[name] = this;
					name="";
				}
				else
				{
					string n = name.substr(0, name.find(' '));
					if (n.length())
						generators[n] = this;
					name.erase(0, name.find(' ')+1);
				}
			}
		}

		// Generic constructor for freq:vol
		SoundGenerator(istream& in)
		{
			string s;
			in >> s;
			freq = atof(s.c_str());
			if (s.find(':')!=string::npos)
			{
				s.erase(0, s.find(':')+1);
				volume = atof(s.c_str())/100;
			}
			else
				volume = 1;

			if (freq<=0 || freq>30000)
			{
				cerr << "Invalid frequency " << freq << endl;
			}

			if (volume<0 || volume>1)
			{
				cerr << "Invalid volume " << volume*100 << endl;
			}
		};

		virtual SoundGenerator* build(istream& in) const=0;
		virtual void help(ostream& out) const
		{
			out << "freq[:volume]";
		}

		float volume;
		float freq;

	private:
		static map<string, const SoundGenerator*> generators;
		static map<string, string> defines;
		static bool echo;

		// For the factory
};

string SoundGenerator::last_type;
map<string, const SoundGenerator*> SoundGenerator::generators;
map<string, string> SoundGenerator::defines;
bool SoundGenerator::echo=true;

template<class T>
class SoundGeneratorVarHook : public SoundGenerator
{
	public:
		SoundGeneratorVarHook(atomic<T>* v, T min, T max, string name)
		:
		mref(v),
		mmin(min),
		mmax(max),
		SoundGenerator(name){}

		SoundGeneratorVarHook(istream &in, atomic<T>* v, T min, T max)
		:
		mref(v), mmin(min), mmax(max) {}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			float delta = (float)(*mref - mmin)/(float)(mmax-mmin);

			left += delta;
			right += delta;
		}

		virtual void help(ostream &out) const
		{ out << "Help not define (SoundGeneratorVarHook)" << endl; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new SoundGeneratorVarHook<T>(in, mref,mmin, mmax); }

	private:
		atomic<T>* mref;
		T mmin;
		T mmax;
};

class WhiteNoiseGenerator : public SoundGenerator
{
	public:
		WhiteNoiseGenerator() : SoundGenerator("wnoise"){}	// factory

		WhiteNoiseGenerator(istream& in)
		{
		};

		virtual void next(float &left, float &right, float speed=1.0)
		{
			left += SoundGenerator::rand();
			right += SoundGenerator::rand();
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new WhiteNoiseGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "wnoise" << endl;
		}

};

class TriangleGenerator : public SoundGenerator
{
	public:
		TriangleGenerator() : SoundGenerator("tri triangle"){};	// factory

		TriangleGenerator(istream& in)
		:
		SoundGenerator(in)
		{
			a = 0;
			da = 4 / ((float)ech / freq);
			sign = 1;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			a += da * speed * (float)sign;
			float dv=a;
			if (dv>1)

			{
				sign = -sign;
				dv=1;
			}
			else if (dv<-1)
			{
				dv=-1;
				sign = -sign;
			}
			left += (float)dv * volume;
			right += (float)dv * volume;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new TriangleGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "triangle ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
		int sign;
};


class SquareGenerator : public SoundGenerator
{
	public:
		SquareGenerator() : SoundGenerator("sq square") {}

		SquareGenerator(istream& in)
		: SoundGenerator(in)
		{
			a = 0;
			da = 1.0f / (float)ech;
			val=1;
			invert = (float)(ech>>1) / freq;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			a += speed;
			left += (float)val * volume;
			right += (float)val * volume;
			if (a > invert)
			{
				a -= invert;
				val = -val;
			}
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SquareGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "square ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
		float invert;
		int val;
};

class SinusGenerator : public SoundGenerator
{
	public:
		SinusGenerator() : SoundGenerator("sin sinus") {};

		SinusGenerator(istream& in)
		: SoundGenerator(in)
		{
			a = 0;
			da = (2 * PI * freq) / (float)ech;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			a += da * speed;
			float s = sin(a);
			left += volume * s;
			right += volume * s;
			if (a > 2*PI)
			{
				a -= 2*PI;
			}
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SinusGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "sinus ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
};

class DistortionGenerator : public SoundGenerator
{
	public:
		DistortionGenerator() : SoundGenerator("distorsion"){}

		DistortionGenerator(istream& in)
		{
			in >> level;
			if (in.good()) generator = factory(in, true);
			if (level<0 || level>100)
			{
				this->help(cerr);
			}

			level = level/100+1.0f;
		}
		
		virtual void next(float &left, float &right, float speed)
		{
			if (!generator) return;
			float l,r;
			generator->next(l,r, speed);
			l *= level;
			r *= level;

			if (l>1) l=1;
			else if (l<-1) l=-1;
			if (r>1) r=1;
			else if (l<-1) l=-1;

			left += l;
			right += r;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new DistortionGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "distorsion level generator" << endl;
			out << "  level = 0..100, ";
			out << "  example: distorsion 50 sinus 200" << endl;
		}

	private:
		float level;
		SoundGenerator* generator;
};

class LevelSound : public SoundGenerator
{
	public:
		LevelSound() : SoundGenerator("level"){}

		LevelSound(istream &in)
		{
			in >> level;
		}

		virtual void next(float &left, float &right, float speed=0.1)
		{
			left += level;
			right += level;
		}

		virtual void help(ostream &out) const
		{ out << "level value : constant level" << endl; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new LevelSound(in); }

	private:
		float level;
};

class GainSound : public SoundGenerator
{
	public:
		GainSound() : SoundGenerator("gain"){}

		GainSound(istream &in)
		{
			in >> gain;
			generator = factory(in,true);
		}

		virtual void next(float &left, float &right, float speed=0.1)
		{
			if (!generator) return;
			float l=0;
			float r=0;
			generator->next(l, r, speed);
			left = l * ((1.0f / 100.0) * gain);
			right = r * ((1.0f / 100.0) * gain);
		}

		virtual void help(ostream &out) const
		{ out << "gain value : constant gain" << endl; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new GainSound(in); }

	private:
		SoundGenerator* generator;
		float gain;
};

class FmGenerator : public SoundGenerator
{
	public:
		FmGenerator() : SoundGenerator("fm") {}	// for thefactory

		FmGenerator(istream& in)
		{
			a = 0;
			in >> min;
			in >> max;
			mod_gen = true;
			mod_mod = false;

			generator = factory(in);

			if (generator == 0)
			{
				if (last_type=="generator")
				{
					mod_gen = true;
					mod_mod = false;
				}
				else if (last_type=="modulator")
				{
					mod_gen = false;
					mod_mod = true;
				}
				else if (last_type=="both")
				{
					mod_gen = true;
					mod_mod = true;
				}
				else
					missingGeneratorExit("417");
			}

			if (generator == 0) generator = factory(in, true);
			modulator = factory(in, true);

			if (min<0 || min>2000 || max<0 || max>2000 || max<min)
			{
				this->help(cerr);
			}

			max /= 100.0;
			min /= 100.0;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			if (min == max)
			{
				generator->next(left, right, mod_gen ? speed : 1.0);
				return;
			}

			float l=0,r=0;
			if (modulator) {
				modulator->next(l, r, mod_mod ? speed : 1.0);	// nbre entre -1 et 1
			}

			l = (l+r)/2.0;
			l = min + (max-min)*(l+1.0)/2.0;

			if (mod_gen) l *= speed;
			generator->next(left, right, l);
		}
		
	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new FmGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "fm min max [speed_modifier] {sound_generator} {sound_modulator}" << endl;
			out << "  min : minimum frequency shifting level" << endl;
			out << "  max : max frequency shifting (0..2000)" << endl;
			out << "  speed_modifier  : where to apply a speed modification (generator/modulator/both) " << endl;
			out << "  sound_generator : a sound generator (as usual)" << endl;
			out << "  sound_modulator : the frequency modulator (another sound generator)" << endl;
			out << "  ex: fm 80 120 sq 220 sin 10 : 220Hz square modulated with sinus" << endl;
			out << endl;
		}

	private:
		float min;
		float max;
		float a;
		SoundGenerator* generator;
		SoundGenerator* modulator;
		float last_ech_left;
		float last_ech_right;
		bool mod_gen;
		bool mod_mod;
};

class MixerGenerator : public SoundGenerator
{
	public:
		MixerGenerator() : SoundGenerator("{"){};

		MixerGenerator(istream& in)
		{
			while (in.good())
			{
				SoundGenerator* p=factory(in);
				if (p)
					generators.push_front(p);
				else
					break;
			}

			if (SoundGenerator::last_type != "}")
			{
				cerr << "Missing } at end of mixer generator" << endl;
			}
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (generators.size()==0)
				return;
			else if (generators.size()==1)
			{
				generators.front()->next(left, right, speed);
				return;
			}

			float l=0;
			float r=0;

			for(auto generator: generators)
				generator->next(l, r, speed);

			left += l / generators.size();
			right += r / generators.size();
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new MixerGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "{ generator [generator ...] }" << endl;
		}
	
	private:
		list<SoundGenerator*>	generators;

};

class LeftSound : public SoundGenerator
{
	public:
		LeftSound() : SoundGenerator("left") {}

		LeftSound(istream& in)
		{
			generator = factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			float v=0;
			generator->next(left, v);
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new LeftSound(in); }

		virtual void help(ostream& out) const
		{
			out << "left generator : keep only the left channel" << endl;
		}

	private:
		SoundGenerator* generator;
};

class RightSound : public SoundGenerator
{
	public:
		RightSound() : SoundGenerator("right") {}

		RightSound(istream& in)
		{
			generator = factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			float v=0;
			generator->next(v, right);
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new RightSound(in); }

		virtual void help(ostream& out) const
		{
			out << "right generator : keep only the right channel" << endl;
		}

	private:
		SoundGenerator* generator;
};

class EnvelopeSound : public SoundGenerator
{
	public:
		EnvelopeSound() : SoundGenerator("envelope env") {}

		EnvelopeSound(istream &in)
		{
			int ms;
			index = 0;

			in >> ms;

			if (ms<=0)
			{
				cerr << "Null duration" << endl;
			}

			istream* input = 0;
			ifstream file;

			string type;
			in >> type;

			loop=true;
			if (type=="once")
			{
				loop=false;
				in >> type;
			}
			else if (type=="loop")
			{
				in >> type;
			}


			if (type=="data")
				input = &in;
			else if (type=="file")
			{
				string name;
				in >> name;
				file.open(name);
				input = &file;
			}
			else
				cerr << "Unkown type: " << type << endl;
			if (input == 0)
			{
				this->help(cerr);
			}
			float v;
			while(input->good())
			{
				string s;
				(*input) >> s;
				if (s.length()==0)
					break;
				if (s=="end")
					break;
				v = atof(s.c_str())/100.0;

				if (v<-200) v=-200;
				if (v>200) v=200;

				data.push_back(v);
			}
			generator = factory(in, true);

			dindex =  1000.0 * (data.size()-1) / (float)ms / ech;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			if (index>data.size())
				return;

			float l=0;
			float r=0;
			generator->next(l,r,speed);

			index += dindex;

			int idx=(int)index;
			float dec = index-idx;

			if (idx<0) idx=0;
			if (index>=(float)data.size()-1)
			{
				idx = data.size()-1;
				if (loop)
					index -= (float)data.size()-1;
			}
			float cur = data[idx];
			if (idx+1 < (int) data.size())
				idx++;
			float next = data[idx];

			float f = cur + (next-cur)*dec;

			left += l*f;
			right += r*f;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new EnvelopeSound(in); }

		virtual void help(ostream& out) const
		{
			out << "envelope {timems} [once|loop] {data} generator" << endl;
			out << "  data is either : file filename, or data v1 ... v2 end" << endl;
			out << "  values are from -200 to 200 (float, >100 may distort sound)" << endl;
		}

	private:
		bool loop;
		float index;
		float dindex;

		vector<float>	data;
		SoundGenerator* generator;
};

class MonoGenerator : public SoundGenerator
{
	public:
		MonoGenerator() : SoundGenerator("mono") {}

		MonoGenerator(istream &in)
		{
			generator = factory(in, true);
		}

		virtual void next(float &left, float &right, float speed = 1.0)
		{
			if (!generator) return;
			float l=0;
			float v=0;
			generator->next(l,v,speed);
			l = (l+v)/2;
			left +=l;
			right +=l;
		}

		virtual void help(ostream& out) const
		{
			out << "mono generator : convert output of generator to monophonic output" << endl;
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new MonoGenerator(in); }

	private:
		SoundGenerator* generator;
};

class AmGenerator : public SoundGenerator
{
	public:
		AmGenerator() : SoundGenerator("am"){};	// for the factory

		AmGenerator(istream &in)
		{
			in >> min;
			in >> max;

			if (in.good()) generator = factory(in, true);
			if (in.good()) modulator = factory(in, true);

			if (min<0 || min>100 || max<0 || max>100)
			{
				cerr << "Out of range (0..100)" << endl;
			}

			max /= 100.0;
			min /= 100.0;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			float l=0,r=0;
			generator->next(l, r, speed);

			float lv=0,rv=0;
			modulator->next(lv, rv, speed);

			lv = min + (max-min)*(lv+1)/2;
			rv = min + (max-min)*(rv+1)/2;

			left += lv*l;
			right += rv*r;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new AmGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "am min max {sound_generator} {sound_modulator}" << endl;
			out << "  min : minimum sound level" << endl;
			out << "  max : max sound level (0..100)" << endl;
			out << "  sound_generator : a sound generator (as usual)" << endl;
			out << "  sound_modulator : the amplitude modulator (another sound generator)" << endl;
			out << "  ex: am 0 100 sq 220 sin 10 : 220Hz square modulated with sinus" << endl;
			out << endl;
		}

	private:
		float min;
		float max;
		SoundGenerator* generator;
		SoundGenerator* modulator;
};

class ReverbGenerator : public SoundGenerator
{
	public:
		ReverbGenerator() : SoundGenerator("reverb echo") {}

		ReverbGenerator(istream& in)
		{
			echo = (SoundGenerator::last_type=="echo");

			string s;

			in >> s;
			if (s.find(':')==string::npos)
			{
				cerr << "Missing :  in " << SoundGenerator::last_type << " generator." << endl;
			}
			float ms = atof(s.c_str()) / 1000.0;
			s.erase(0, s.find(':')+1);
			vol = atof(s.c_str()) / 100.0;

			if (ms<=0)
			{
				cerr << "Negatif time not allowed." << endl;
			}

			buf_size = ech * ms;

			if (buf_size == 0 || buf_size > 1000000)
			{
				cerr << "Bad Buffer size : " << buf_size << endl;
			}

			buf_left = new float[buf_size];
			buf_right = new float[buf_size];
			for(uint32_t i=0; i<buf_size; i++)
			{
				buf_left[i] = 0;
				buf_right[i] = 0;
			}
			index = 0;
			ech_vol = 1.0 - vol;
			ech_vol = 1.0;

			generator = factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			float l=0;
			float r=0;
			generator->next(l,r,speed);

			if (echo)
			{
				float ll = buf_left[index];
				float rr = buf_right[index];

				buf_left[index] = l;
				buf_right[index] = r;

				l = l*ech_vol + ll*vol;
				r = r*ech_vol + rr*vol;
			}
			else
			{
				l = l*ech_vol + buf_left[index]*vol;
				r = r*ech_vol + buf_right[index]*vol;

				buf_left[index] = l;
				buf_right[index] = r;
			}
			index++;
			if (index==buf_size)
				index= 0;

			left += l;
			right += r;
		}

	protected:
		virtual void help(ostream& out) const
		{
			out << "reverb ms:vol generator" << endl;
			out << "echo ms:vol generator" << endl;
		}

		virtual SoundGenerator* build(istream &in) const
		{ return new ReverbGenerator(in); }

	private:
		bool echo;
		float vol;
		float ech_vol;
		float* buf_left;
		float* buf_right;
		uint32_t	buf_size;
		uint32_t	index;
		SoundGenerator* generator;
};

class AdsrGenerator : public SoundGenerator
{
	struct value
	{
		float s;
		float vol;

		bool operator <= (const value &v)
		{
			return s <= v.s;
		}

		friend ostream& operator<< (ostream &out, const value &v)
		{
			out << '(' << v.s << "s, " << v.vol << ')';
			return out;
		}
	};

	public:
		AdsrGenerator() : SoundGenerator("adsr"){}

		AdsrGenerator(istream& in)
		{
			value v;
			value prev;
			prev.s = 0;
			prev.vol = 0;

			while(read(in, v))
			{
				if (v <= prev)
				{
					cerr << "ADSR times must be ordered." << endl;
					cerr << "previous ms=" << prev.s << " current=" << v.s << endl;
				}
				values.push_back(v);
				prev = v;
			}

			if (values.size()==0)
			{
				cerr << "ADSR must contains at least 1 value at t>0ms." << endl;
			}

			generator = factory(in,true);
			dt = 1.0 / (float)ech;
			restart();
		}

		void restart()
		{
			t=0;
			index = 0;
			target = values[0];
			previous.s = 0;
			previous.vol = 0;
		}

		bool read(istream &in, value &val)
		{
			string s;
			in >> s;
			if (s=="once")
			{
				loop = false;
				return false;
			}
			else if (s=="loop")
			{
				loop = true;
				return false;
			}

			if (s.find(':')==string::npos)
			{
				cerr << "Missing : in value or type (once/loop) for adsr." << endl;
			}
			val.s = atof(s.c_str()) / 1000;
			s.erase(0, s.find(':')+1);
			val.vol = atof(s.c_str()) / 100;		
			return true;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (!generator) return;
			float l=0;
			float r=0;
			generator->next(l, r, speed);
			
			if (index >= values.size())
			{
				left += l*target.vol;
				right += r*target.vol;
				return;
			}

			t += dt;
			while (t >= target.s)
			{
				previous = target;
				index++;
				if (index < values.size())
					target = values[index];
				else
				{
					if (loop) restart();
					break;
				}
			}

			float factor = (t-previous.s)  / (target.s - previous.s);
			float vol = previous.vol  + (target.vol-previous.vol)*factor;

			left += l*vol;
			right += r*vol;
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new AdsrGenerator(in); }

		virtual void help(ostream &out) const
		{
			out << "adsr ms:vol ... ms:vol type generator : adsr type envelope generator" << endl;
			out << "   ms  : time in ms" << endl;
			out << "   vol : volume at this time (0..1)" << endl;
			out << "   type: either once or loop" << endl;
		}

	private:
		float t;
		float dt;

		value previous;
		value target;
		uint32_t index;
		vector<value>	values;
		SoundGenerator* generator;
		bool loop;
};

class ChainSound : public SoundGenerator
{
	class ChainElement
	{
		public:
			ChainElement(uint32_t ms, SoundGenerator* g) : t(ms/1000.0), sound(g){};
			
		float t;
		SoundGenerator* sound;
	};

	public:
		ChainSound() : SoundGenerator("chain") {}

		ChainSound(istream& in)
		{
			uint32_t ms=0;
			while(in.good())
			{
				string sms;
				in >> sms;
				if (sms == "end")
					break;
				ms += atol(sms.c_str());
				SoundGenerator* sound = factory(in);
				if (sound)
					add(ms, sound);
				else
					SoundGenerator::missingGeneratorExit();
			}
			dt = 1.0/(float)ech;
			reset();
		}

		void add(uint32_t ms, SoundGenerator* g)
		{
			if (g)
				sounds.push_back(ChainElement(ms, g));
		}

		void reset()
		{
			t = 0;
			it = sounds.begin();
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (it!=sounds.end())
			{
				t += dt;
				const ChainElement& sound=*it;
				if (t>sound.t)
					it++;
				sound.sound->next(left, right, speed);
			}
		}

		virtual void help(ostream& out) const
		{ out << "chain ms sound_1 ms sound_2 ... end" << endl; }

	private:
		virtual SoundGenerator* build(istream& in) const
		{ return new ChainSound(in); }

		list<ChainElement> sounds;
		list<ChainElement>::const_iterator it;

		float dt;
		float t;
};

SoundGenerator* SoundGenerator::factory(istream& in, bool needed)
{
	SoundGenerator* gen=0;
	last_type="";
	string type;
	while(in.good())
	{
	in >> type;
		if (type.length() && type[0] != '#')
			break;
		else
	{
			string s;
			getline(in, s);
		}
	}
	if (type.find(".synth") != string::npos)	// assume a file
		{
		ifstream file(type);
		if (file.good())
			gen = factory(file, needed);
	}
	else if (type == "print")
	{
		string line;
		getline(in, line);
		if (echo)
			cout << line << endl;
		gen = factory(in, needed);
	}
	else if (type == "-q")
	{
		echo = false;
	}
	else if (type=="define")
	{
		string name;
		in >> name;
		if (name.length())
		{
			uint16_t brackets = 0;
			stringstream define;
			do
			{
				string item;
				in >> item;
				if (item == "{")
					brackets++;
				else if (item == "}")
					brackets--;
				else if (brackets == 0)
				{
					string end_of_line;
					getline(in, end_of_line);
					define << end_of_line << endl;
					break;
				}
				define << item << ' ';
			} while (brackets && in.good());
			defines[name] = define.str();

			gen = factory(in, needed);
		}
		else
		{
			cerr << "missing name" << endl;
		}
	}
	else if (type.length())
	{
		last_type = type;
		auto it = generators.find(last_type);
		if (it != generators.end())
		{
			gen = it->second->build(in);
			if (gen == 0)
			{
				cerr << "Unable to build " << last_type << endl;
	}
		}
		else
		{
			auto it = defines.find(last_type);
			if (it != defines.end())
			{
				stringstream def;
				def << defines[last_type];
				gen = factory(def,false);
				if (gen == 0)
				{
					cerr << "Unable to build " << last_type << ", please fix the corresponding define." << endl;
				}
			}
		}
		if (gen == 0)
			cerr << "Unknown generator " << type << endl;
	}
	if (gen == 0)
	{
		if (needed)
			missingGeneratorExit("");
	}
	return gen;
}

//void help()
//{
//	cout << "Syntax : " << endl;
//	cout << "  synth [duration] [sample_freq] generator_1 [generator_2 [...]]" << endl;
//	cout << endl;
//	cout << "  duration     : sound duration (ms)" << endl;
//	cout << endl;
//	cout << "Available sound generators : " << endl;
//	cout << "  file.synth : read synth file" << endl;
//
//	SoundGenerator::help();
//
//	exit(1);
//}

// Auto register for the factory
static SquareGenerator gen_sq;
static TriangleGenerator gen_tr;
static SinusGenerator gen_si;
static AmGenerator gen_am;
static DistortionGenerator gen_dist;
static FmGenerator gen_fm;
static MixerGenerator gen_mix;
static WhiteNoiseGenerator gen_wn;
static EnvelopeSound gen_env;
static LeftSound gen_left;
static RightSound gen_right;
static AdsrGenerator gen_adrs;
static ReverbGenerator gen_reverb;
static LevelSound gen_level;
static GainSound gen_gain;
static MonoGenerator gen_mono;
static ChainSound gen_chain;