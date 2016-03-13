#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#ifndef DECODER
#ifndef ENCODER
#error "Define DECODER or ENCODER before building"
#endif
#endif

#ifdef DECODER
#ifdef ENCODER
#error "Don't define both, dammit!"
#endif
#endif

// this is for ensuring that the encoder/decoder doesn't go AWOL
// higher shift means weaker filter
#define USE_HPF
#define HPF_SHIFT 6

struct shadpcm {
	int32_t level;
	int32_t step0;
	int32_t step1;
};

int32_t step_tab[8] = {
	// still experimenting with good curves.

	//1, 2, 6, 14, 35, 86, 210, 512 // int(floor(pow(512.0, i/7.0)+0.5))
	//4, 8, 16, 32, 64, 128, 256, 512 // int(floor(4.0*pow(512.0/4.0, i/7.0)+0.5))
	//8, 14, 26, 48, 86, 156, 283, 512 // int(floor(8.0*pow(512.0/8.0, i/7.0)+0.5))
	//16, 26, 43, 71, 116, 190, 312, 512 // int(floor(16.0*pow(512.0/16.0, i/7.0)+0.5))
	//16, 29, 53, 95, 172, 312, 565, 1024 // int(floor(16.0*pow(1024.0/16.0, i/7.0)+0.5))
	32, 48, 71, 105, 156, 232, 345, 512 // int(floor(32.0*pow(512.0/32.0, i/7.0)+0.5))
};

int get_step(struct shadpcm *ct)
{
	int32_t step0 = ct->step0;
	int32_t step1 = ct->step1;

	// still experimenting with good ratios, too!

	return (step0 + step1*3 + 2)>>2;
	//return (step0 + step1*2 + 1)/3;
	//return (step0 + step1 + 1)>>1;
	//return (step0*2 + step1 + 1)/3;
	//return (step0*3 + step1 + 2)>>2;
}

void add_sample_dec(struct shadpcm *ct, int32_t v)
{
	assert(v >= 0 && v <= 15);

	int amp = (v&7);

	int32_t delta_step = get_step(ct);
	//int32_t delta_step = ct->step0;
	int32_t delta = delta_step*((amp<<1)+1);
	ct->step1 = ct->step0;
	ct->step0 = step_tab[amp];

#ifdef USE_HPF
	ct->level = (ct->level*((1<<HPF_SHIFT)-1) + (1<<(HPF_SHIFT-1)))>>HPF_SHIFT;
#endif
	ct->level += ((v&8) ? -delta : delta);
}

int add_sample_enc(struct shadpcm *ct, int32_t v)
{
	assert(v >= -0x8000 && v <= 0x7FFF);

	int32_t delta_step = get_step(ct);
	//int32_t delta_step = ct->step0;
	int32_t rlevel = ct->level;
#ifdef USE_HPF
	rlevel = (rlevel*((1<<HPF_SHIFT)-1) + (1<<(HPF_SHIFT-1)))>>HPF_SHIFT;
#endif
	int32_t delta = v - rlevel;

	int dsign = delta < 0 ? -1 : 1;
	int dsignbit = delta < 0 ? 8 : 0;
	delta = (dsignbit ? -delta : delta);
	int32_t oval = (delta + delta_step+1)/(delta_step*2);

	//oval = (oval - 15 + 1)/2;
	if(oval < 0) { oval = 0; }
	if(oval > 7) { oval = 7; }
	oval |= dsignbit;
	add_sample_dec(ct, oval);
	return oval;
}

int main(int argc, char *argv[])
{
#ifdef ENCODER
	struct shadpcm enc;
	memset(&enc, 0, sizeof(struct shadpcm));
	enc.step0 = step_tab[0];
	enc.step1 = step_tab[0];
#endif

#ifdef DECODER
	struct shadpcm dec;
	memset(&dec, 0, sizeof(struct shadpcm));
	dec.step0 = step_tab[0];
	dec.step1 = step_tab[0];
#endif

#ifdef ENCODER
	int bc = 0;
	for(;;) {
		int32_t v;
		v = fgetc(stdin);
		v |= fgetc(stdin)<<8;
		if(v < 0) break;
		v = (int32_t)(int16_t)v;

		v = add_sample_enc(&enc, v);
		assert(v >= 0 && v <= 15);
		if(bc == 0) {
			bc = (v)|0x100;
		} else {
			bc |= (v<<4);
			bc &= 0xFF;
			fputc(bc, stdout);
			bc = 0;
		}
	}
#endif

#ifdef DECODER
	int bc = 1;
	for(;;) {
		if(bc == 1) {
			bc = fgetc(stdin);
			if(bc < 0) { break; }
			bc |= 0x100;
		}

		int32_t v = (bc&15);
		bc >>= 4;
		assert(v >= 0 && v <= 15);
		add_sample_dec(&dec, v);
		v = dec.level;
		if(v < -0x8000) { v = -0x8000; }
		if(v >  0x7FFF) { v =  0x7FFF; }
		fputc((v>>0)&0xFF, stdout);
		fputc((v>>8)&0xFF, stdout);
	}
#endif

	return 0;
}

