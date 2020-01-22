/* Originally released by the musl project (http://www.musl-libc.org/) under the
 * MIT license. Taken from the file src/stdio/vfprintf.c */

#include <ctype.h>
#include "floatscan.h"
#include "vfprintf.h"
//int isdigit(int);
//#define isdigit(a) (0 ? isdigit(a) : ((unsigned)(a)-'0') < 10)

long double frexpl(long double x, int *e);
//#include <math.h>
#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double frexpl(long double x, int *e)
{
	return frexp(x, e);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double frexpl(long double x, int *e)
{
	union ldshape u = {x};
	int ee = u.i.se & 0x7fff;

	if (!ee) {
		if ((_Bool)x) {
			x = frexpl(x*0x1p120, e);
			*e -= 120;
		} else *e = 0;
		return x;
	} else if (ee == 0x7fff) {
		return x;
	}

	*e = ee - 0x3ffe;
	u.i.se &= 0x8000;
	u.i.se |= 0x3ffe;
	return u.f;
}
#endif

int __signbitl(long double x);
#if (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
int __signbitl(long double x)
{
	union ldshape u = {x};
	return u.i.se >> 15;
}
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
int __signbitl(long double x)
{
	return __signbit(x);
}
#endif

static __inline unsigned __FLOAT_BITS(float __f)
{
	union {float __f; unsigned __i;} __u;
	__u.__f = __f;
	return __u.__i;
}
static __inline unsigned long long __DOUBLE_BITS(double __f)
{
	union {double __f; unsigned long long __i;} __u;
	__u.__f = __f;
	return __u.__i;
}

#define signbit(x) ( \
	sizeof(x) == sizeof(float) ? (int)(__FLOAT_BITS((float)x)>>31) : \
	sizeof(x) == sizeof(double) ? (int)(__DOUBLE_BITS((double)x)>>63) : \
	__signbitl(x) )

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4


int __fpclassifyl(long double x);
#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
int __fpclassifyl(long double x)
{
	return __fpclassify(x);
}
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
int __fpclassifyl(long double x)
{
	union ldshape u = {x};
	int e = u.i.se & 0x7fff;
	int msb = (int)(u.i.m>>63);
	if (!e && !msb)
		return u.i.m ? FP_SUBNORMAL : FP_ZERO;
	if (!msb)
		return FP_NAN;
	if (e == 0x7fff)
		return u.i.m << 1 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384
int __fpclassifyl(long double x)
{
	union ldshape u = {x};
	int e = u.i.se & 0x7fff;
	u.i.se = 0;
	if (!e)
		return u.i2.lo | u.i2.hi ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0x7fff)
		return u.i2.lo | u.i2.hi ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}
#endif

#define isfinite(x) ( \
	sizeof(x) == sizeof(float) ? (__FLOAT_BITS((float)x) & 0x7fffffff) < 0x7f800000 : \
	sizeof(x) == sizeof(double) ? (__DOUBLE_BITS((double)x) & -1ULL>>1) < 0x7ffULL<<52 : \
	__fpclassifyl(x) > FP_INFINITE)

#include "vfprintf.h"
/* Some useful macros */

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define ALT_FORM   (1U<<('#'-' '))
#define ZERO_PAD   (1U<<('0'-' '))
#define LEFT_ADJ   (1U<<('-'-' '))
#define PAD_POS    (1U<<(' '-' '))
#define MARK_POS   (1U<<('+'-' '))
#define GROUPED    (1U<<('\''-' '))

#define FLAGMASK (ALT_FORM|ZERO_PAD|LEFT_ADJ|PAD_POS|MARK_POS|GROUPED)

/* State machine to accept length modifiers + conversion specifiers.
 * Result is 0 on failure, or an argument type to pop on success. */

enum {
	BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE,
	ZTPRE, JPRE,
	STOP,
	PTR, INT, UINT, ULLONG,
	LONG, ULONG,
	SHORT, USHORT, CHAR, UCHAR,
	LLONG, SIZET, IMAX, UMAX, PDIFF, UIPTR,
	DBL, LDBL,
	NOARG,
	MAXSTATE
};

#define S(x) [(x)-'A']
/*
static const unsigned char states[]['z'-'A'+1] = {
	{ // 0: bare types 
		S('d') = INT, S('i') = INT,
		S('o') = UINT, S('u') = UINT, S('x') = UINT, S('X') = UINT,
		S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
		S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,
		S('c') = CHAR, S('C') = INT,
		S('s') = PTR, S('S') = PTR, S('p') = UIPTR, S('n') = PTR,
		S('m') = NOARG,
		S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE,
		S('z') = ZTPRE, S('j') = JPRE, S('t') = ZTPRE,
	}, { // 1: l-prefixed 
		S('d') = LONG, S('i') = LONG,
		S('o') = ULONG, S('u') = ULONG, S('x') = ULONG, S('X') = ULONG,
		S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
		S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,
		S('c') = INT, S('s') = PTR, S('n') = PTR,
		S('l') = LLPRE,
	}, { // 2: ll-prefixed 
		S('d') = LLONG, S('i') = LLONG,
		S('o') = ULLONG, S('u') = ULLONG,
		S('x') = ULLONG, S('X') = ULLONG,
		S('n') = PTR,
	}, { //3: h-prefixed 
		S('d') = SHORT, S('i') = SHORT,
		S('o') = USHORT, S('u') = USHORT,
		S('x') = USHORT, S('X') = USHORT,
		S('n') = PTR,
		S('h') = HHPRE,
	}, { // 4: hh-prefixed 
		S('d') = CHAR, S('i') = CHAR,
		S('o') = UCHAR, S('u') = UCHAR,
		S('x') = UCHAR, S('X') = UCHAR,
		S('n') = PTR,
	}, { // 5: L-prefixed 
		S('e') = LDBL, S('f') = LDBL, S('g') = LDBL, S('a') = LDBL,
		S('E') = LDBL, S('F') = LDBL, S('G') = LDBL, S('A') = LDBL,
		S('n') = PTR,
	}, { // 6: z- or t-prefixed (assumed to be same size) 
		S('d') = PDIFF, S('i') = PDIFF,
		S('o') = SIZET, S('u') = SIZET,
		S('x') = SIZET, S('X') = SIZET,
		S('n') = PTR,
	}, { //7: j-prefixed 
		S('d') = IMAX, S('i') = IMAX,
		S('o') = UMAX, S('u') = UMAX,
		S('x') = UMAX, S('X') = UMAX,
		S('n') = PTR,
	}
};
*/
#define OOB(x) ((unsigned)(x)-'A' > 'z'-'A')



static void out(char **sp, const char *s, size_t l)
{
	//if (!(f->flags & F_ERR)) __fwritex((void *)s, l, f);
        while (l--) {
                **sp = *s;
                (*sp)++;
                s++;
        }
}

static void pad(char **sp, char c, int w, int l, int fl)
{
	char pad[256];
	if ((unsigned int)fl & (LEFT_ADJ | ZERO_PAD) || l >= w) return;
	l = w - l;
	memset(pad, c, (long unsigned int)l>sizeof pad ? sizeof pad : (long unsigned int)l);
	for (; (long unsigned int)l >= sizeof pad; l = l - (int)(sizeof pad))
		out(sp, pad, sizeof pad);
	out(sp, pad, (size_t)l);
}

static const char xdigits[17] = {"0123456789ABCDEF"};

/*
static char *fmt_x(uintmax_t x, char *s, int lower)
{
	for (; x; x>>=4) *--s = xdigits[(x&15)]|lower;
	return s;
}

static char *fmt_o(uintmax_t x, char *s)
{
	for (; x; x>>=3) *--s = '0' + (x&7);
	return s;
}
*/
static char *fmt_u(uintmax_t x, char *s)
{
	unsigned long y;
	for (   ; x>ULONG_MAX; x/=10) *--s = (char)('0' + x%10);
	for (y=x;           y; y/=10) *--s = (char)('0' + y%10);
	return s;
}

/* Do not override this check. The floating point printing code below
 * depends on the float.h constants being right. If they are wrong, it
 * may overflow the stack. */
#if LDBL_MANT_DIG == 53
typedef char compiler_defines_long_double_incorrectly[9-(int)sizeof(long double)];
#endif


/*
 * w = string width, p = precision, fl = flags, t = type. "%20.5g gives w=20, p=5, fl=0, t='g'
 */
int fmt_fp(char *output, long double y, int w, int p, int fl, int t)
{
    char* sp = output;
	uint32_t big[(LDBL_MANT_DIG+28)/29 + 1          // mantissa expansion
		+ (LDBL_MAX_EXP+LDBL_MANT_DIG+28+8)/9]; // exponent expansion
	uint32_t *a, *d, *r, *z;
	int e2=0, e, i, j, l;
	char buf[9+LDBL_MANT_DIG/4], *s;
	const char *prefix="-0X+0X 0X-0x+0x 0x";
	int pl;
	char ebuf0[3*sizeof(int)], *ebuf=&ebuf0[3*sizeof(int)], *estr = NULL;

	pl=1;
	if (signbit(y)) {
		y=-y;
	} else if ((unsigned int)fl & MARK_POS) {
		prefix+=3;
	} else if ((unsigned int)fl & PAD_POS) {
		prefix+=6;
	} else prefix++, pl=0;

	if (!isfinite(y)) {
		s = (t&32)?"inf":"INF";
		if (y!=y) s=(t&32)?"nan":"NAN";
		pad(&sp, ' ', w, 3+pl, (int)((unsigned int)fl &~ ZERO_PAD));
		out(&sp, prefix, (size_t)pl);
		out(&sp, s, 3);
		pad(&sp, ' ', w, 3+pl, (int)((unsigned int)fl^LEFT_ADJ));
		return MAX(w, 3+pl);
	}

	y = frexpl(y, &e2) * 2;
	if ((_Bool)y) e2--;

	if ((t|32)=='a') {
		long double round = 8.0;
		int re;

		if (t&32) prefix += 9;
		pl += 2;

		if (p<0 || p>=LDBL_MANT_DIG/4-1) re=0;
		else re=LDBL_MANT_DIG/4-1-p;

		if (re) {
			while (re--) round*=16;
			if (*prefix=='-') {
				y=-y;
				y-=round;
				y+=round;
				y=-y;
			} else {
				y+=round;
				y-=round;
			}
		}

		estr=fmt_u((uintmax_t )(e2<0 ? -e2 : e2), ebuf);
		if (estr==ebuf) *--estr='0';
		*--estr = (e2<0 ? '-' : '+');
		*--estr = (char)(t+('p'-'a'));

		s=buf;
		do {
			int x=(int)y;
			*s++=(char)(xdigits[x]|(t&32));
			y=16*(y-x);
			if (s-buf==1 && ((_Bool)y||p>0||((unsigned int)fl&ALT_FORM))) *s++='.';
		} while ((_Bool)y);

		if (p > INT_MAX-2-(ebuf-estr)-pl)
			return -1;
		if (p && s-buf-2 < p)
			l = (int)((p+2) + (ebuf-estr));
		else
			l = (int)((s-buf) + (ebuf-estr));

		pad(&sp, ' ', w, pl+l, fl);
		out(&sp, prefix, (size_t)pl);
		pad(&sp, '0', w, pl+l, (int)((unsigned int)fl^ZERO_PAD));
		out(&sp, buf, (size_t)(s-buf));
		pad(&sp, '0', (int)(l-(ebuf-estr)-(s-buf)), 0, 0);
		out(&sp, estr, (size_t)(ebuf-estr));
		pad(&sp, ' ', w, pl+l, (int)((unsigned int)fl^LEFT_ADJ));
		return MAX(w, pl+l);
	}
	if (p<0) p=6;

	if ((_Bool)y) y *= 0x1p28, e2-=28;

	if (e2<0) a=r=z=big;
	else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

	do {
		*z = (uint32_t)y;
		y = 1000000000*(y-*z++);
	} while ((_Bool)y);

	while (e2>0) {
		uint32_t carry=0;
		int sh=MIN(29,e2);
		for (d=z-1; d>=a; d--) {
			uint64_t x = ((uint64_t)*d<<sh)+carry;
			*d = (uint32_t)(x % 1000000000);
			carry = (uint32_t)(x / 1000000000);
		}
		if (carry) *--a = carry;
		while (z>a && !z[-1]) z--;
		e2-=sh;
	}
	while (e2<0) {
		uint32_t carry=0, *b;
		int sh=MIN(9,-e2), need=(int)(1+((unsigned int)p+LDBL_MANT_DIG/3U+8)/9);
		for (d=a; d<z; d++) {
			uint32_t rm = (*d & (uint32_t)((1<<sh)-1));
			*d = (*d>>sh) + carry;
			carry = ((uint32_t)(1000000000>>sh) * rm);
		}
		if (!*a) a++;
		if (carry) *z++ = carry;
		/* Avoid (slow!) computation past requested precision */
		b = (t|32)=='f' ? r : a;
		if (z-b > need) z = b+need;
		e2+=sh;
	}

	if (a<z) for (i=10, e=(int)(9*(r-a)); *a>=(unsigned)i; i*=10, e++);
	else e=0;

	/* Perform rounding: j is precision after the radix (possibly neg) */
	j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
	if (j < 9*(z-r-1)) {
		uint32_t x;
		/* We avoid C's broken division of negative numbers */
		d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
		j += 9*LDBL_MAX_EXP;
		j %= 9;
		for (i=10, j++; j<9; i*=10, j++);
		x = (*d % (uint32_t)i);
		/* Are there any significant digits past j? */
		if (x || d+1!=z) {
			long double round = 2/LDBL_EPSILON;
			long double small;
			if ((*d/(uint32_t)(i) & 1) || (i==1000000000 && d>a && (d[-1]&1)))
				round += 2;
			if (x<(unsigned)i/2) small=0x0.8p0;
			else if (x==(unsigned)i/2 && d+1==z) small=0x1.0p0;
			else small=0x1.8p0;
			if (pl && *prefix=='-') round*=-1, small*=-1;
			*d -= x;
			/* Decide whether to round by probing round+small */
			if (round+small != round) {
				*d = *d + (uint32_t)i;
				while (*d > 999999999) {
					*d--=0;
					if (d<a) *--a=0;
					(*d)++;
				}
				for (i=10, e=(int)(9*(r-a)); *a>=(unsigned)i; i*=10, e++);
			}
		}
		if (z>d+1) z=d+1;
	}
	for (; z>a && !z[-1]; z--);
	
	if ((t|32)=='g') {
		if (!p) p++;
		if (p>e && e>=-4) {
			t--;
			p-=e+1;
		} else {
			t-=2;
			p--;
		}
		if (!((uint32_t)fl&ALT_FORM)) {
			/* Count trailing zeros in last place */
			if (z>a && z[-1]) for (i=10, j=0; (z[-1]%(uint32_t)i)==0; i*=10, j++);
			else j=9;
			if ((t|32)=='f')
				p = (int)MIN(p,MAX(0,9*(z-r-1)-j));
			else
				p = (int)MIN(p,MAX(0,9*(z-r-1)+e-j));
		}
	}
	if (p > INT_MAX-1-(p || ((unsigned int)fl&ALT_FORM)))
		return -1;
	l = 1 + p + (p || ((unsigned int)fl&ALT_FORM));
	if ((t|32)=='f') {
		if (e > INT_MAX-l) return -1;
		if (e>0) l+=e;
	} else {
		estr=fmt_u((uintmax_t)(e<0 ? -e : e), ebuf);
		while(ebuf-estr<2) *--estr='0';
		*--estr = (e<0 ? '-' : '+');
		*--estr = (char)t;
		if (ebuf-estr > INT_MAX-l) return -1;
		l += (int)(ebuf-estr);
	}

	if (l > INT_MAX-pl) return -1;
	pad(&sp, ' ', w, pl+l, fl);
	out(&sp, prefix, (size_t)pl);
	pad(&sp, '0', w, pl+l, (int)((unsigned int)fl^ZERO_PAD));

	if ((t|32)=='f') {
		if (a>r) a=r;
		for (d=a; d<=r; d++) {
			s = fmt_u(*d, buf+9); //@@@
			if (d!=a) while (s>buf) *--s='0';
			else if (s==buf+9) *--s='0';
			out(&sp, s, (size_t)(buf+9-s));
		}
		if (p || ((unsigned int)fl&ALT_FORM)) out(&sp, ".", 1);
		for (; d<z && p>0; d++, p-=9) {
			s = fmt_u(*d, buf+9); //@@@
			while (s>buf) *--s='0';
			out(&sp, s, (size_t)(MIN(9,p)));
		}
		pad(&sp, '0', p+9, 9, 0);
	} else {
		if (z<=a) z=a+1;
		for (d=a; d<z && p>=0; d++) {
			s = fmt_u(*d, buf+9); //@@@
			if (s==buf+9) *--s='0';
			if (d!=a) while (s>buf) *--s='0';
			else {
				out(&sp, s++, 1);
				if (p>0||((unsigned int)fl&ALT_FORM)) out(&sp, ".", 1);
			}
			out(&sp, s, (size_t)(MIN(buf+9-s, p)));
			p -= (int)(buf+9-s);
		}
		pad(&sp, '0', p+18, 18, 0);
		out(&sp, estr, (size_t)(ebuf-estr));
	}

	pad(&sp, ' ', w, pl+l, (int)((unsigned int)fl^LEFT_ADJ));

	return MAX(w, pl+l);
}

/*
static int getint(char **s) {
	int i;
	for (i=0; isdigit(**s); (*s)++) {
		if (i > INT_MAX/10U || **s-'0' > INT_MAX-10*i) i = -1;
		else i = 10*i + (**s-'0');
	}
	return i;
}
*/
