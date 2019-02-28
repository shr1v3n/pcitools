/*
 * For the final layer of fuzzing:
 *
 * Fuzzing modifications (introduce known bad values [8, 16, 32, 64 bit])
 * as well as only OR'ing in the signed bit half the time, if we aren't
 * using known bad values, which already account for that).
 *
 * Also, we read a specific amount of random numbers every runthrough,
 * this is because in many fuzzers, when looping, previous state can affect
 * the number of times you call your PRNG (rand()) function, which can
 * affect reproducability.  We want reproducability, even though this may
 * not apply here, it can't hurt. 
 *
 * Other fuzzing techniques and TODOs:  
 *    Remove a '\0' from packet (string extension).
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>

/*
 * charx = known shaky values for bytes
 * shortx = known shaky values for shorts
 * intx = known shaky values for ints
 * longx = known shaky values for longs
 * e.g., where +1, +2, or -1, -2, or +0 could 
 * cause an integer overflow.
 *
 */
unsigned char charx[] = {
  0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
  0x9, 0xa, 0xe, 0xf, 0x10, 0x1e, 0x1f, 0x20,
  0x21, 0x22, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x62,
  0x63, 0x64, 0x65, 0x66, 0x7e, 0x7f, 0x80, 0x81,
  0x82, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xfa, 0xfb,
  0xfc, 0xfd, 0xfe, 0xff
};

unsigned short shortx[] = {
  0x0, 0x1, 0x2, 0xfe, 0xff, 0x100, 0x101, 0x102, 0x1fe, 0x1ff, 0x200,
  0x201, 0x202, 0x3fe, 0x3ff, 0x400, 0x401, 0x402, 0x7fe,
  0x7ff, 0x800, 0x801, 0x802, 0xffe, 0xfff, 0x1000, 0x1001,
  0x1002, 0x1ffe, 0x1fff, 0x2000, 0x2001, 0x2002, 0x3ffe, 0x3fff,
  0x4000, 0x4001, 0x4002, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0x8002,
  0xfffe, 0xffff, 0x6, 0x7, 0x8,
  0x9, 0xa, 0xe, 0xf, 0x10, 0x11, 0x12, 0x1e,
  0x1f, 0x20, 0x21, 0x22, 0x3e, 0x3f, 0x40, 0x41,
  0x42, 0x62, 0x63, 0x64, 0x65, 0x66, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
  0x8fd, 0x8fe, 0x8ff, 0x900, 0x901, 0xafd, 0xafe, 0xaff,
  0xb00, 0xb01, 0x10fd, 0x10fe, 0x10ff, 0x1100, 0x1101, 0x14fd,
  0x14fe, 0x14ff, 0x1500, 0x1501, 0x20fd, 0x20fe, 0x20ff, 0x2100,
  0x2101, 0x40fd, 0x40fe, 0x40ff, 0x4100, 0x4101, 0x64fd, 0x64fe,
  0x64ff, 0x6500, 0x6501, 0xc7fe, 0xc7ff, 0xc800, 0xc801, 0xc802,
  0xfefe, 0xfeff, 0xff00, 0xff01, 0xff02
};

unsigned int intx[] = {
  0x0, 0x1, 0x2, 0x10000,
  0x10001, 0x100, 0x101, 0x102, 0x1fe, 0x1ff, 0x200, 0x201,
  0x202, 0x3fe, 0x3ff, 0x400, 0x401, 0x402, 0x7fe, 0x7ff,
  0x800, 0x801, 0x802, 0xffe, 0xfff, 0x1000, 0x1001, 0x1002,
  0x1ffe, 0x1fff, 0x2000, 0x2001, 0x2002, 0x3ffe, 0x3fff, 0x4000,
  0x4001, 0x4002, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0x8002, 0xfffe,
  0xffff, 0x6, 0x7, 0x8, 0x9,
  0xa, 0xe, 0xf, 0x10, 0x11, 0x12, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x3e, 0x3f, 0x40, 0x41, 0x42,
  0x62, 0x63, 0x64, 0x65, 0x66, 0x7e, 0x7f, 0x80,
  0x81, 0x82, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x8fd, 0x8fe, 0x8ff, 0x900, 0x901,
    0xafd, 0xafe, 0xaff, 0xb00,
  0xb01, 0x10fd, 0x10fe, 0x10ff, 0x1100, 0x1101, 0x14fd, 0x14fe,
  0x14ff, 0x1500, 0x1501, 0x20fd, 0x20fe, 0x20ff, 0x2100, 0x2101,
  0x40fd, 0x40fe, 0x40ff, 0x4100, 0x4101, 0x64fd, 0x64fe, 0x64ff,
  0x6500, 0x6501, 0xc7fe, 0xc7ff, 0xc800, 0xc801, 0xc802, 0xfefe,
  0xfeff, 0xff00, 0xff01, 0xff02, 0x8fffd, 0x8fffe, 0x8ffff, 0x90000,
  0x90001, 0xafffd, 0xafffe, 0xaffff, 0xb0000, 0xb0001, 0x10fffd, 0x10fffe,
  0x10ffff, 0x110000, 0x110001, 0x14, 0x14fffd, 0x14fffe, 0x14ffff, 0x150000,
  0x150001, 0x20fffd, 0x20fffe, 0x20ffff, 0x210000, 0x210001, 0x40fffd,
    0x40fffe,
  0x40ffff, 0x410000, 0x410001, 0x64fffd, 0x64fffe, 0x64ffff, 0x650000,
    0x650001,
  0x80fffd, 0x80fffe, 0x80ffff, 0x810000, 0x810001, 0xc8fffd, 0xc8fffe,
    0xc8ffff,
  0xc90000, 0xc90001, 0xfffffd, 0xfffffe, 0xffffff, 0x1000000, 0x1000001
};

#if 0
typedef uint64_t unsigned long
#elseif
typedef uint64_t unsigned long long
#endif
unsigned long long longx[] = { 0x0, 0x1, 0x2,
  0x100, 0x101, 0x102, 0x1fe, 0x1ff, 0x200, 0x201,
  0x202, 0x3fe, 0x3ff, 0x400, 0x401, 0x402, 0x7fe, 0x7ff,
  0x800, 0x801, 0x802, 0xffe, 0xfff, 0x1000, 0x1001, 0x1002,
  0x1ffe, 0x1fff, 0x2000, 0x2001, 0x2002, 0x3ffe, 0x3fff, 0x4000,
  0x4001, 0x4002, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0x8002, 0xfffe,
  0xffff, 0x6, 0x7, 0x8, 0x9,
  0xa, 0xe, 0xf, 0x10, 0x11, 0x12, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x3e, 0x3f, 0x40, 0x41, 0x42,
  0x62, 0x63, 0x64, 0x65, 0x66, 0x7e, 0x7f, 0x80,
  0x81, 0x82, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x8fd,
  0x8fe, 0x8ff, 0x900, 0x901, 0xafd, 0xafe, 0xaff, 0xb00,
  0xb01, 0x10fd, 0x10fe, 0x10ff, 0x1100, 0x1101, 0x14fd, 0x14fe,
  0x14ff, 0x1500, 0x1501, 0x20fd, 0x20fe, 0x20ff, 0x2100, 0x2101,
  0x40fd, 0x40fe, 0x40ff, 0x4100, 0x4101, 0x64fd, 0x64fe, 0x64ff,
  0x6500, 0x6501, 0xc7fe, 0xc7ff, 0xc800, 0xc801, 0xc802, 0xfefe,
  0xfeff, 0xff00, 0xff01, 0xff02,
  0x100, 0x101, 0x102,
  0x1fe, 0x1ff, 0x200, 0x201,
  0x202, 0x3fe, 0x3ff, 0x400,
  0x401, 0x402, 0x7fe, 0x7ff,
  0x800, 0x801, 0x802, 0xffe,
  0xfff, 0x1000, 0x1001, 0xfffffffd,
  0xfffffffe, 0xffffffff, 0x100000000, 0x100000001, 0x8fffffffd, 0x8fffffffe,
    0x8ffffffff, 0x900000000,
  0x900000001, 0xafffffffd, 0xafffffffe, 0xaffffffff, 0xb00000000,
    0xb00000001, 0x10fffffffd, 0x10fffffffe,
  0x10ffffffff, 0x1100000000, 0x1100000001, 0x14fffffffd, 0x14fffffffe,
    0x14ffffffff, 0x1500000000, 0x1500000001,
  0x20fffffffd, 0x20fffffffe, 0x20ffffffff, 0x2100000000, 0x2100000001,
    0x40fffffffd, 0x40fffffffe, 0x40ffffffff,
  0x4100000000, 0x4100000001, 0x64fffffffd, 0x64fffffffe, 0x64ffffffff,
    0x6500000000, 0x6500000001, 0x80fffffffd,
  0x80fffffffe, 0x80ffffffff, 0x8100000000, 0x8100000001, 0xc8fffffffd,
    0xc8fffffffe, 0xc8ffffffff, 0xc900000000,
  0xc900000001, 0xfffffffffd, 0xfffffffffe, 0xffffffffff, 0x10000000000,
    0x10000000001, 0x7ffffffffffffffd, 0x7ffffffffffffffe,
  0x7fffffffffffffff, 0x8000000000000000, 0x8000000000000001,
    0xfffffffffffffffd, 0xfffffffffffffffe, 0xffffffffffffffff
};

/*
 * Quick bit population function.
 * O(1).
 */
int
bitpop (int i)
{
  i = i - ((i >> 1) & 0x55555555);
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
  return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

typedef enum
{ BYTE, INT16, INT32, INT64, FLOAT } type_t;

type_t
decide (int v, int v2)
{
  v = bitpop (v);
  v += bitpop (v2);



#if 0
  /*
   * Expects v to be a bitpop(x) and not x
   * so 0 <= v < 64
   *
   * Linear distribution == 12.8 != integer, so we give 12 to BYTE and 13 to the rest
   * 
   */

  if (v < 12)
    return BYTE;
  if (v < 25)
    return INT16;
  if (v < 38)
    return INT32;
  if (v < 51)
    return INT64;

  return FLOAT;
#endif

  /*
   * Don't account for floats/doubles.
   * Instead of 12.8, use a linear distribution (64 / 4 == 16).
   * 0  -> 15 == charx
   * 16 -> 31 == shortx
   * 32 -> 47 == intx
   * 48 -> 64 == longx
   */

  if (v < 16)
    return BYTE;
  if (v < 32)
    return INT16;
  if (v < 48)
    return INT32;

  return INT64;

}

#define ROUND_DOWN(x, t) (((unsigned long)(x)) & (~(sizeof(t)-1)))

#define HIGH64(v)       (*rp++ & 1) ? (((long long)(v)) | 0x8000000000000000) : (long long)(v)
#define HIGH32(v)       (*rp++ & 1) ? ((unsigned int)(v) | 0x80000000) : (unsigned int)(v)
#define HIGH16(v)       (*rp++ & 1) ? ((unsigned short)(v) | 0x8000) : (unsigned short)(v)
#define HIGH8(v)        (*rp++ & 1) ? ((unsigned char)(v) | 0x80) : (unsigned char)(v)

/*
 * Macros for longx, longx, charx, shortx (badly handled values).
 */

#define PUT_INT64X() do { *(long long *)buf = longx[r % (sizeof(longx)/sizeof(longx)[0])]; \
			   nb -= sizeof(long long); buf += sizeof(long long); } while(0)
#define PUT_INT32X() do { *(unsigned int *)buf = intx[r % (sizeof(intx)/sizeof(intx)[0])]; \
			   nb -= sizeof(int); buf += sizeof(int); } while(0)
#define PUT_INT16X() do { *(unsigned short *)buf = shortx[r % (sizeof(shortx)/sizeof(shortx)[0])]; \
			   nb -= sizeof(short); buf += sizeof(short); } while(0)
#define PUT_BYTEX() do { *(unsigned char *)buf = charx[r % (sizeof(charx)/sizeof(charx)[0])]; \
			   nb -= sizeof(char); buf += sizeof(char); } while(0)
/*
 * Put random values.
 */
#define PUT_RND_FLOAT() do { *(float *)buf = (float)HIGH32(r); nb -= sizeof(float); \
                             buf += sizeof(float); } while(0)
#define PUT_RND_INT64() do { *(long long *)buf = (long long)HIGH64(((long long)(r) << 32) | *rp++); nb -= sizeof(long long); \
                             buf += sizeof(long long); } while(0)
#define PUT_RND_INT32() do { *(unsigned int *)buf = (unsigned int)HIGH32(r); nb -= sizeof(unsigned int); \
                                buf += sizeof(int); } while(0)
#define PUT_RND_INT16() do { *(unsigned short *)buf = (unsigned short)HIGH16((unsigned short)(r) & 0xffff); nb -= sizeof(short); \
                                buf += sizeof(short); } while(0)
#define PUT_RND_BYTE()  do { *(unsigned char *)buf = (unsigned char)HIGH8((unsigned char)(r) & 0xff); nb -= sizeof(char); \
                                buf += sizeof(char); } while(0)

void
fuzz (char *obuf, unsigned int nb, unsigned int maxchg)
{
  unsigned int r[16];
  unsigned int *rp = r;
  unsigned char *buf = obuf;
  unsigned int ofs;
  int i;

  /*
   * maxchg must be less than nb.
   */
  maxchg %= nb;

  /*
   * Read sizeof(r) / sizeof(r)[0] + 1 rands.
   */
  for (i = 0; i < sizeof (r) / sizeof (r)[0]; i++)
    r[i] = rand ();

  ofs = rand () % nb;		/* + 1 on rand count. */

  /*
   * Index into the buffer somewhat.
   */

  if (*rp++ & 1)
    {
      /* 
       * Half the time, round the ofs to a multiple of the word size.
       */
      printf ("Rounding offset down from %d ", ofs);
      ofs = ROUND_DOWN (ofs, int);
      printf ("to %d\n", ofs);
    }

  buf += ofs;
  printf
    ("buf is size %d, incrementing %d bytes max (if this says, 8, you might get 5, 6, 7, or 8. it's a limit.) into obuf\n",
     nb, ofs);

  while (nb > 0 && maxchg > 0)
    {
      unsigned int r, s;

      r = *rp++;
      s = *rp++;

      switch (decide (*rp++, *rp++))
	{
	case FLOAT:
	  if (nb >= sizeof (float) && !((uintptr_t) buf % sizeof (float)))
	    {
	      /*
	       * sizeof(float) == sizeof(int) == 4 on this platform with gcc.
	       */
	      if (s & 1)
		PUT_RND_FLOAT ();
	      else
		PUT_INT32X ();
	      break;
	    }
	case INT64:
	  if (nb >= sizeof (long long)
	      && !((uintptr_t) buf % sizeof (long long)))
	    {
	      maxchg--;
	      if (s & 1)
		PUT_RND_INT64 ();
	      else
		PUT_INT64X ();
	      break;
	    }
	case INT32:
	  if (nb >= sizeof (int) && !((uintptr_t) buf % sizeof (int)))
	    {
	      maxchg--;
	      if (s & 1)
		PUT_RND_INT32 ();
	      else
		PUT_INT32X ();
	      break;
	    }
	case INT16:
	  if (nb >= sizeof (short) && !(uintptr_t) buf % sizeof (short))
	    {
	      maxchg--;
	      if (s & 1)
		PUT_RND_INT16 ();
	      else
		PUT_INT16X ();
	      break;
	    }
	case BYTE:
	  if (nb >= 1)
	    {
	      maxchg--;
	      if (s & 1)
		PUT_RND_BYTE ();
	      else
		PUT_BYTEX ();
	      break;
	    }
	}
    }
}

#ifdef TEST

int
main (int argc, char **argv)
{
  int n;
  char *buf;

  if (argv[1])
    {
      buf = strdup (argv[1]);
      n = strlen (buf);
      fuzz (buf, n, 5);
      if (!strcmp (buf, argv[1]))
	printf ("Fuzz changed nothing.\n");
      else
	printf ("Fuzz changed.\n");
      while (n--)
	{
	  printf ("%.02hhx   ", *buf++ & 0xff);
	  if (!(n % 10))
	    printf ("\n");
	}

    }
}

#endif
