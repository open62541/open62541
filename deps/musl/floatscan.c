/* Originally released by the musl project (http://www.musl-libc.org/) under the
 * MIT license. Taken from the file src/internal/floatscan.c*/

#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "floatscan.h"
#include "vfprintf.h"
int shgetc(char* input, int *index);
void shunget(int *index);
int shlim(int a, int b);

int shgetc(char* input, int *index){
    int res = input[*index];
    (*index)++;
    return res;
}

void shunget(int *index){
    (*index)--;
}
int shlim(int a, int b){
    return '0';
}


#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double fmodl(long double x, long double y)
{
	return fmod(x, y);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double fmodl(long double x, long double y)
{
	union ldshape ux = {x}, uy = {y};
	int ex = ux.i.se & 0x7fff;
	int ey = uy.i.se & 0x7fff;
	int sx = ux.i.se & 0x8000;

	if (y == 0 || isnan(y) || ex == 0x7fff)
		return (x*y)/(x*y);
	ux.i.se = (uint16_t)ex;
	uy.i.se = (uint16_t)ey;
	if (ux.f <= uy.f) {
		if (ux.f == uy.f)
			return 0*x;
		return x;
	}

	/* normalize x and y */
	if (!ex) {
		ux.f *= 0x1p120f;
		ex = ux.i.se - 120;
	}
	if (!ey) {
		uy.f *= 0x1p120f;
		ey = uy.i.se - 120;
	}

	/* x mod y */
#if LDBL_MANT_DIG == 64
	uint64_t i, mx, my;
	mx = ux.i.m;
	my = uy.i.m;
	for (; ex > ey; ex--) {
		i = mx - my;
		if (mx >= my) {
			if (i == 0)
				return 0*x;
			mx = 2*i;
		} else if (2*mx < mx) {
			mx = 2*mx - my;
		} else {
			mx = 2*mx;
		}
	}
	i = mx - my;
	if (mx >= my) {
		if (i == 0)
			return 0*x;
		mx = i;
	}
	for (; mx >> 63 == 0; mx *= 2, ex--);
	ux.i.m = mx;
#elif LDBL_MANT_DIG == 113
	uint64_t hi, lo, xhi, xlo, yhi, ylo;
	xhi = (ux.i2.hi & -1ULL>>16) | 1ULL<<48;
	yhi = (uy.i2.hi & -1ULL>>16) | 1ULL<<48;
	xlo = ux.i2.lo;
	ylo = uy.i2.lo;
	for (; ex > ey; ex--) {
		hi = xhi - yhi;
		lo = xlo - ylo;
		if (xlo < ylo)
			hi -= 1;
		if (hi >> 63 == 0) {
			if ((hi|lo) == 0)
				return 0*x;
			xhi = 2*hi + (lo>>63);
			xlo = 2*lo;
		} else {
			xhi = 2*xhi + (xlo>>63);
			xlo = 2*xlo;
		}
	}
	hi = xhi - yhi;
	lo = xlo - ylo;
	if (xlo < ylo)
		hi -= 1;
	if (hi >> 63 == 0) {
		if ((hi|lo) == 0)
			return 0*x;
		xhi = hi;
		xlo = lo;
	}
	for (; xhi >> 48 == 0; xhi = 2*xhi + (xlo>>63), xlo = 2*xlo, ex--);
	ux.i2.hi = xhi;
	ux.i2.lo = xlo;
#endif

	/* scale result */
	if (ex <= 0) {
		ux.i.se = (uint16_t)((ex+120)|sx);
		ux.f *= (uint16_t)(0x1p-120f);
	} else
		ux.i.se = (uint16_t)(ex|sx);
	return ux.f;
}
#endif


#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double copysignl(long double x, long double y)
{
	return copysign(x, y);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double copysignl(long double x, long double y)
{
	union ldshape ux = {x}, uy = {y};
	ux.i.se &= 0x7fff;
	ux.i.se = (uint16_t)(ux.i.se | (uy.i.se & 0x8000));
	return ux.f;
}
#endif

double scalbn(double x, int n)
{
	union {double f; uint64_t i;} u;
	double_t y = x;

	if (n > 1023) {
		y *= 0x1p1023;
		n -= 1023;
		if (n > 1023) {
			y *= 0x1p1023;
			n -= 1023;
			if (n > 1023)
				n = 1023;
		}
	} else if (n < -1022) {
		/* make sure final n < -53 to avoid double
		   rounding in the subnormal range */
		y *= 0x1p-1022 * 0x1p53;
		n += 1022 - 53;
		if (n < -1022) {
			y *= 0x1p-1022 * 0x1p53;
			n += 1022 - 53;
			if (n < -1022)
				n = -1022;
		}
	}
	u.i = (uint64_t)(0x3ff+n)<<52;
	x = y * u.f;
	return x;
}

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024

#define LD_B1B_DIG 2
#define LD_B1B_MAX 9007199, 254740991
#define KMAX 128

#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384

#define LD_B1B_DIG 3
#define LD_B1B_MAX 18, 446744073, 709551615
#define KMAX 2048

#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384

#define LD_B1B_DIG 4
#define LD_B1B_MAX 10384593, 717069655, 257060992, 658440191
#define KMAX 2048

#else
#error Unsupported long double representation
#endif

#define MASK (KMAX-1)

#define CONCAT2(x,y) x ## y
#define CONCAT(x,y) CONCAT2(x,y)


#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double scalbnl(long double x, int n)
{
	return scalbn(x, n);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double scalbnl(long double x, int n)
{
	union ldshape u;

	if (n > 16383) {
		x *= 0x1p16383L;
		n -= 16383;
		if (n > 16383) {
			x *= 0x1p16383L;
			n -= 16383;
			if (n > 16383)
				n = 16383;
		}
	} else if (n < -16382) {
		x *= 0x1p-16382L * 0x1p113L;
		n += 16382 - 113;
		if (n < -16382) {
			x *= 0x1p-16382L * 0x1p113L;
			n += 16382 - 113;
			if (n < -16382)
				n = -16382;
		}
	}
	u.f = 1.0;
	u.i.se = (uint16_t)(0x3fff + n);
	return x * u.f;
}
#endif

static long long scanexp(char* input, int *index, int pok)
{
	int c;
	int x;
	long long y;
	int neg = 0;
	
	c = shgetc(input, index);
	if (c=='+' || c=='-') {
		neg = (c=='-');
		c = shgetc(input, index);
		if ((unsigned)(c-'0')>=10U && pok) shunget(index);
	}
	if ((unsigned)(c-'0')>=10U) {
		shunget(index);
		return LLONG_MIN;
	}
	for (x=0; (unsigned)(c-'0')<10U && x<INT_MAX/10; c = shgetc(input, index))
		x = 10*x + c-'0';
	for (y=x; (unsigned)(c-'0')<10U && y<LLONG_MAX/100; c = shgetc(input, index))
		y = 10*y + c-'0';
	for (; (unsigned)(c-'0')<10U; c = shgetc(input, index));
	shunget(index);
	return neg ? -y : y;
}


static long double decfloat(char *input, int *index, int c, int bits, int emin, int sign, int pok)
{
	uint32_t x[KMAX];
	static const uint32_t th[] = { LD_B1B_MAX };
	int i, j, k, a, z;
	long long lrp=0, dc=0;
	long long e10=0;
	int lnz = 0;
	int gotdig = 0, gotrad = 0;
	int rp;
	int e2;
	int emax = -emin-bits+3;
	int denormal = 0;
	long double y;
	long double frac=0;
	long double bias=0;
	static const int p10s[] = { 10, 100, 1000, 10000,
		100000, 1000000, 10000000, 100000000 };

	j=0;
	k=0;

	/* Don't let leading zeros consume buffer space */
	for (; c=='0'; c = shgetc(input, index)) gotdig=1;
	if (c=='.') {
		gotrad = 1;
		for (c = shgetc(input, index); c=='0'; c = shgetc(input, index)) gotdig=1, lrp--;
	}

	x[0] = 0;
	for (; (unsigned)(c-'0')<10U || c=='.'; c = shgetc(input, index)) {
		if (c == '.') {
			if (gotrad) break;
			gotrad = 1;
			lrp = dc;
		} else if (k < KMAX-3) {
			dc++;
			if (c!='0') lnz = (int)dc;
			if (j) x[k] = (x[k]*10 + (uint32_t)(c-'0'));
			else x[k] = (uint32_t)(c-'0');
			if (++j==9) {
				k++;
				j=0;
			}
			gotdig=1;
		} else {
			dc++;
			if (c!='0') {
				lnz = (KMAX-4)*9;
				x[KMAX-4] |= 1;
			}
		}
	}
	if (!gotrad) lrp=dc;

	if (gotdig && (c|32)=='e') {
		e10 = scanexp(input, index, pok);
		if (e10 == LLONG_MIN) {
			if (pok) {
				shunget(index);
			} else {
				//shlim(f, 0);
				return 0;
			}
			e10 = 0;
		}
		lrp += e10;
	} else if (c>=0) {
		shunget(index);
	}
	if (!gotdig) {
		errno = EINVAL;
		//shlim(f, 0);
		return 0;
	}

	/* Handle zero specially to avoid nasty special cases later */
	if (!x[0]) return sign * 0.0;

	/* Optimize small integers (w/no exponent) and over/under-flow */
	if (lrp==dc && dc<10 && (bits>30 || x[0]>>bits==0))
		return sign * (long double)x[0];
	if (lrp > -emin/2) {
		errno = ERANGE;
		return sign * LDBL_MAX * LDBL_MAX;
	}
	if (lrp < emin-2*LDBL_MANT_DIG) {
		errno = ERANGE;
		return sign * LDBL_MIN * LDBL_MIN;
	}

	/* Align incomplete final B1B digit */
	if (j) {
		for (; j<9; j++) x[k]*=10;
		k++;
		//j=0;
	}

	a = 0;
	z = k;
	e2 = 0;
	rp = (int)lrp;

	/* Optimize small to mid-size integers (even in exp. notation) */
	if (lnz<9 && lnz<=rp && rp < 18) {
		if (rp == 9) return sign * (long double)x[0];
		if (rp < 9) return sign * (long double)x[0] / p10s[8-rp];
		int bitlim = bits-3*(int)(rp-9);
		if (bitlim>30 || x[0]>>bitlim==0)
			return sign * (long double)x[0] * p10s[rp-10];
	}

	/* Drop trailing zeros */
	for (; !x[z-1]; z--);

	/* Align radix point to B1B digit boundary */
	if (rp % 9) {
		int rpm9 = rp>=0 ? rp%9 : rp%9+9;
		int p10 = p10s[8-rpm9];
		uint32_t carry = 0;
		for (k=a; k!=z; k++) {
			uint32_t tmp = (x[k] % (uint32_t)p10);
			x[k] = x[k]/(uint32_t)p10 + carry;
			carry = 1000000000/(uint32_t)p10 * tmp;
			if (k==a && !x[k]) {
				a = ((a+1) & MASK);
				rp -= 9;
			}
		}
		if (carry) x[z++] = carry;
		rp += 9-rpm9;
	}

	/* Upscale until desired number of bits are left of radix point */
	while (rp < 9*LD_B1B_DIG || (rp == 9*LD_B1B_DIG && x[a]<th[0])) {
		uint32_t carry = 0;
		e2 -= 29;
		for (k=((z-1) & MASK); ; k=((k-1) & MASK)) {
			uint64_t tmp = ((uint64_t)x[k] << 29) + carry;
			if (tmp > 1000000000) {
				carry = (uint32_t)(tmp / 1000000000);
				x[k] = (uint32_t)(tmp % 1000000000);
			} else {
				carry = 0;
				x[k] = (uint32_t)tmp;
			}
			if (k==((z-1) & MASK) && k!=a && !x[k]) z = k;
			if (k==a) break;
		}
		if (carry) {
			rp += 9;
			a = ((a-1) & MASK);
			if (a == z) {
				z = ((z-1) & MASK);
				x[(z-1) & MASK] |= x[z];
			}
			x[a] = carry;
		}
	}

	/* Downscale until exactly number of bits are left of radix point */
	for (;;) {
		uint32_t carry = 0;
		int sh = 1;
		for (i=0; i<LD_B1B_DIG; i++) {
			k = ((a+i) & MASK);
			if (k == z || x[k] < th[i]) {
				i=LD_B1B_DIG;
				break;
			}
			if (x[(a+i) & MASK] > th[i]) break;
		}
		if (i==LD_B1B_DIG && rp==9*LD_B1B_DIG) break;
		/* FIXME: find a way to compute optimal sh */
		if (rp > 9+9*LD_B1B_DIG) sh = 9;
		e2 += sh;
		for (k=a; k!=z; k=((k+1) & MASK)) {
			uint32_t tmp = (x[k] & (uint32_t)((1<<sh)-1));
			x[k] = (x[k]>>sh) + carry;
			carry = ((uint32_t)(1000000000>>sh) * tmp);
			if (k==a && !x[k]) {
				a = ((a+1) & MASK);
				i--;
				rp -= 9;
			}
		}
		if (carry) {
			if (((z+1) & MASK) != a) {
				x[z] = carry;
				z = ((z+1) & MASK);
			} else x[(z-1) & MASK] |= 1;
		}
	}

	/* Assemble desired bits into floating point variable */
	for (y=i=0; i<LD_B1B_DIG; i++) {
		if (((a+i) & MASK)==z) x[(z=((z+1) & MASK))-1] = 0;
		y = 1000000000.0L * y + x[(a+i) & MASK];
	}

	y *= sign;

	/* Limit precision for denormal results */
	if (bits > LDBL_MANT_DIG+e2-emin) {
		bits = LDBL_MANT_DIG+e2-emin;
		if (bits<0) bits=0;
		denormal = 1;
	}

	/* Calculate bias term to force rounding, move out lower bits */
	if (bits < LDBL_MANT_DIG) {
		bias = copysignl(scalbn(1, 2*LDBL_MANT_DIG-bits-1), y);
		frac = fmodl(y, scalbn(1, LDBL_MANT_DIG-bits));
		y -= frac;
		y += bias;
	}

	/* Process tail of decimal input so it can affect rounding */
	if (((a+i) & MASK) != z) {
		uint32_t t = x[(a+i) & MASK];
		if (t < 500000000 && (t || ((a+i+1) & MASK) != z))
			frac += 0.25*sign;
		else if (t > 500000000)
			frac += 0.75*sign;
		else if (t == 500000000) {
			if (((a+i+1) & MASK) == z)
				frac += 0.5*sign;
			else
				frac += 0.75*sign;
		}
		//if (LDBL_MANT_DIG-bits >= 2 && !fmodl(frac, 1)) implicit conversion turns floating-point number into integer:
                if (LDBL_MANT_DIG-bits >= 2 && !((_Bool)fmodl(frac, 1)))
			frac++;
	}

	y += frac;
	y -= bias;

	if (((e2+LDBL_MANT_DIG) & INT_MAX) > emax-5) {
		if (fabs((double)y) >= CONCAT(0x1p, LDBL_MANT_DIG)) {
			if (denormal && bits==LDBL_MANT_DIG+e2-emin)
				denormal = 0;
			y *= 0.5;
			e2++;
		}
                //if (e2+LDBL_MANT_DIG>emax || (denormal && frac)) implicit conversion turns floating-point number into integer:
		if (e2+LDBL_MANT_DIG>emax || ((_Bool)denormal && (_Bool)frac))
			errno = ERANGE;
	}

	return scalbnl(y, e2);
}

static long double hexfloat(char *input, int *index, int bits, int emin, int sign, int pok)
{
	uint32_t x = 0;
	long double y = 0;
	long double scale = 1;
	long double bias = 0;
	int gottail = 0, gotrad = 0, gotdig = 0;
	long long rp = 0;
	long long dc = 0;
	long long e2 = 0;
	int d;
	int c;

	c = shgetc(input, index);

	/* Skip leading zeros */
	for (; c=='0'; c = shgetc(input, index)) gotdig = 1;

	if (c=='.') {
		gotrad = 1;
		c = shgetc(input, index);
		/* Count zeros after the radix point before significand */
		for (rp=0; c=='0'; c = shgetc(input, index), rp--) gotdig = 1;
	}

	for (; (unsigned)(c-'0')<10U || (unsigned)((c|32)-'a')<6U || c=='.'; c = shgetc(input, index)) {
		if (c=='.') {
			if (gotrad) break;
			rp = dc;
			gotrad = 1;
		} else {
			gotdig = 1;
			if (c > '9') d = (c|32)+10-'a';
			else d = c-'0';
			if (dc<8) {
				x = (x*16 + (uint32_t)d);
			} else if (dc < LDBL_MANT_DIG/4+1) {
				y += d*(scale/=16);
			} else if (d && !gottail) {
				y += 0.5*scale;
				gottail = 1;
			}
			dc++;
		}
	}
	if (!gotdig) {
		shunget(index);
		if (pok) {
			shunget(index);
			if (gotrad) shunget(index);
		} else {
			//shlim(f, 0);
		}
		return sign * 0.0;
	}
	if (!gotrad) rp = dc;
	while (dc<8) x *= 16, dc++;
	if ((c|32)=='p') {
		e2 = scanexp(input, index, pok);
		if (e2 == LLONG_MIN) {
			if (pok) {
				shunget(index);
			} else {
				//shlim(f, 0);
				return 0;
			}
			e2 = 0;
		}
	} else {
		shunget(index);
	}
	e2 += 4*rp - 32;

	if (!x) return sign * 0.0;
	if (e2 > -emin) {
		errno = ERANGE;
		return sign * LDBL_MAX * LDBL_MAX;
	}
	if (e2 < emin-2*LDBL_MANT_DIG) {
		errno = ERANGE;
		return sign * LDBL_MIN * LDBL_MIN;
	}

	while (x < 0x80000000) {
		if (y>=0.5) {
			x += x + 1;
			y += y - 1;
		} else {
			x += x;
			y += y;
		}
		e2--;
	}

	if (bits > 32+e2-emin) {
		bits =(int)(32+e2-emin);
		if (bits<0) bits=0;
	}

	if (bits < LDBL_MANT_DIG)
		bias = copysignl(scalbn(1, 32+LDBL_MANT_DIG-bits-1), sign);

	if (bits<32 && (_Bool)y && !(x&1)) x++, y=0;

	y = bias + sign*(long double)x + sign*y;
	y -= bias;

	if (!((_Bool)y)) errno = ERANGE;

	return scalbnl(y, (int)e2);
}

long double __floatscan(char* input, int prec, int pok)
{
    int index = 0;
	int sign = 1;
	//size_t i;
        int i;
	int bits;
	int emin;
	int c;

	switch (prec) {
	case 0:
		bits = FLT_MANT_DIG;
		emin = FLT_MIN_EXP-bits;
		break;
	case 1:
		bits = DBL_MANT_DIG;
		emin = DBL_MIN_EXP-bits;
		break;
	case 2:
		bits = LDBL_MANT_DIG;
		emin = LDBL_MIN_EXP-bits;
		break;
	default:
		return 0;
	}

	while (isspace((c=shgetc(input, &index))));

	if (c=='+' || c=='-') {
		sign -= 2*(c=='-');
		c = shgetc(input, &index);
	}

	for (i=0; i<8 && (c|32)=="infinity"[i]; i++)
		if (i<7) c = shgetc(input, &index);
	if (i==3 || i==8 || (i>3 && pok)) {
		if (i!=8) {
			shunget(&index);
			if (pok) for (; i>3; i--) shunget(&index);
		}
		return ((float)sign * INFINITY);
	}
	if (!i) for (i=0; i<3 && (c|32)=="nan"[i]; i++)
		if (i<2) c = shgetc(input, &index);
	if (i==3) {
		if (shgetc(input, &index) != '(') {
			shunget(&index);
			return NAN;
		}
		for (i=1; ; i++) {
			c = shgetc(input, &index);
			if ((unsigned)(c-'0')<10U || (unsigned)(c-'A')<26U || (unsigned)(c-'a')<26U || c=='_')
				continue;
			if (c==')') return NAN;
			shunget(&index);
			if (!pok) {
				errno = EINVAL;
				//shlim(0, 0);
				return 0;
			}
			while (i--) shunget(&index);
			return NAN;
		}
		return NAN;
	}

	if (i) {
		shunget(&index);
		errno = EINVAL;
		//shlim(0, 0);
		return 0;
	}

	if (c=='0') {
		c = shgetc(input, &index);
		if ((c|32) == 'x')
			return hexfloat(input, &index, bits, emin, sign, pok);
		shunget(&index);
		c = '0';
	}

	return decfloat(input, &index, c, bits, emin, sign, pok);
}
