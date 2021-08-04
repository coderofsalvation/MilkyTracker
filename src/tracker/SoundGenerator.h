#ifndef _SOUNDGENERATOR_H
#define _SOUNDGENERATOR_H

// synth based on Colin Diesh github example https://github.com/cmdcolin/freqmod/blob/master/freqmod.c

#define S_RATE  (44100)
#define max(a,b) (a>b?a:b)

#define cosFunc(pos, theta) cos(pos * 2 * PI + theta);
#define squareFunc(pos, theta)  cos(pos * 2 * PI + theta) <= 0 ? -1 : 1;
#define triangleFunc(pos, theta) 1 - fabs(fmod(pos + theta, 1.0) - 0.5) * 4;
#define noise(pos,theta) (float)rand()/(float)RAND_MAX;

//fmod equation
//x(t)=A(t)*[cos(2pi*fc*t + I(t)cos(2pi*fm*t+osc2_t) + osc1_t]
//A(t) time varying amplitude
//I(t) modulation index
//fc the carrier frequency
//fm the modulating frequency
//osc1_t,osc2_t phase constants

struct synth_state {
	float osc1;
	float osc2;
	float osc3;
	float amplitude; // 0..1
	float decay;
	int osc1_type;
	int osc2_type;
	int osc3_type;
	int third_mod;
	float depth;
};

class SoundGenerator {

public:
	struct synth_state s = { 40.,80.,40.,1.,0.0001,0,0,0,0,100.0}; // default

	void init(string cfg){
		//   params: <freq_osc1> <freq_osc2> <freq_osc3> <amp>  <decay> <osc1_type> <osc2_type> <osc3_type> <third_mod>  <mod_depth>
		// defaults:    40.0         80.0       40.0      1.0   0.0001       0           0           0           0          100.0
		// osctypes: 0=sine 1=square 2=triangle 3=noise 
		sscanf(
			cfg.c_str(), "%f %f %f %f %f %i %i %i %i %i %f",
			&s.osc1,
			&s.osc2,
			&s.osc3,
			&s.amplitude,
			&s.decay,
			&s.osc1_type,
			&s.osc2_type,
			&s.osc3_type,
			&s.third_mod,
			&s.depth);
	}

	int generateRandom(float* buf, int n, bool seamless) {
		int o1 = rand() % 4;
		int o2 = rand() % 4;
		int o3 = rand() % 4;
		int r3 = seamless ? 2 : (rand() % 3);
		s.osc1  = 80.0 * (float)(rand() % 4);
		s.osc2  = ((rand() % 2) ? s.osc1 : 80.0) * (float)(rand() % 4);
		s.osc3  = ((rand() % 2) ? s.osc2 : 80.0) * (float)(rand() % 4);
		switch (r3) {
			case 0: s.decay = 0.0001; break;
			case 1: s.decay = 0.00006; break;
			case 2: s.decay = 0.0; break;
		}
		s.osc1_type = o1;
		s.osc2_type = o2 != o1 ? o2 : rand() % 3;
		s.osc3_type = o3 != o1 ? o3 : rand() % 3;
		s.third_mod = rand() % 4;
		s.depth = (1.0f / 5) * (float)(rand() % 5);
		if (s.osc1_type == 3 || s.osc2_type == 3 || s.osc3_type == 3) s.decay = 0.0001; // noise means short sounds
		return generate(buf, n);
	}

	int generate( float *buf, int n) {
		float t = 0;
		float I_t = 0;
		int sign = 1;
		int i = 0;
		float A = s.amplitude;
		I_t = s.depth;
		for (int i = 0; i < n; i++) {
			t += 1.0 / S_RATE;
			float p1 = fmod(s.osc1 * t, 1.0);
			float p2 = fmod(s.osc2 * t, 1.0);
			float p3 = fmod(s.osc3 * t, 1.0);
			A = max(0, A - s.decay);

			if (s.third_mod == 0) {
				float c1 = cosFunc(p1, 0);
				if (s.osc1_type == 1) c1 = squareFunc(p1, 0);
				if (s.osc1_type == 2) c1 = triangleFunc(p1, 0);
				if (s.osc1_type == 3) c1 = noise(p1, 0);
				float c2 = cosFunc(p2, I_t * c1);
				if (s.osc2_type == 1) c2 = squareFunc(p2, I_t * c1);
				if (s.osc2_type == 2) c2 = triangleFunc(p2, I_t * c1);
				if (s.osc2_type == 3) c2 = noise(p2,0);
				buf[i]  = (float)(A * c2);
			}else if (s.third_mod == 1) {
				float c1 = cosFunc(p1, 0);
				if (s.osc1_type == 1) c1 = squareFunc(p1, 0);
				if (s.osc1_type == 2) c1 = triangleFunc(p1, 0);
				if (s.osc1_type == 3) c1 = noise(p1, 0);
				float c2 = cosFunc(p2, I_t * c1);
				if (s.osc2_type == 1) c2 = squareFunc(p2, I_t * c1);
				if (s.osc2_type == 2) c2 = triangleFunc(p2, I_t * c1);
				if (s.osc2_type == 3) c2 = noise(p2, 0);
				float c3 = cosFunc(p3, I_t * c2);
				if (s.osc3_type == 1) c3 = squareFunc(p3, I_t * c2);
				if (s.osc3_type == 2) c3 = triangleFunc(p3, I_t * c2);
				if (s.osc3_type == 3) c3 = noise(p3, 0);
				buf[i] = (float)(A * c3);
			}else if (s.third_mod == 2) {
				float c1 = cosFunc(p1, 0);
				if (s.osc1_type == 1) c1 = squareFunc(p1, 0);
				if (s.osc1_type == 2) c1 = triangleFunc(p1, 0);
				if (s.osc1_type == 3) c1 = noise(p1, 0);
				float c2 = cosFunc(p2, 0);
				if (s.osc2_type == 1) c2 = squareFunc(p2, 0);
				if (s.osc2_type == 2) c2 = triangleFunc(p2, 0);
				if (s.osc2_type == 3) c2 = noise(p2, 0);
				float c3 = cosFunc(p3, I_t * c2 + I_t * c1);
				if (s.osc3_type == 1) c3 = squareFunc(p3, I_t * c2 + I_t * c1);
				if (s.osc3_type == 2) c3 = triangleFunc(p3, I_t * c2 + I_t * c1);
				if (s.osc3_type == 3) c3 = noise(p3, 0);
				buf[i] = (float)(A * c3);
			}else if (s.third_mod == 3) {
				float c1 = cosFunc(p1, 0);
				if (s.osc1_type == 1) c1 = squareFunc(p1, 0);
				if (s.osc1_type == 2) c1 = triangleFunc(p1, 0);
				if (s.osc1_type == 3) c1 = noise(p1, 0);
				buf[i] = (float)(A * c1);
			}
		}
		return 0;
	}

};

#endif