/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf keywords.gperf  */
/* Computed positions: -k'1,4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "keywords.gperf"

#include "tokens.h"
#line 11 "keywords.gperf"
struct keyword {const char *name; StateType state;};

#define TOTAL_KEYWORDS 26
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 33
/* maximum key range = 32, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 21,  0, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 30,  0,
      34, 34, 34, 34, 34, 34, 34,  0, 25, 34,
      34, 34, 34, 34, 34, 34, 34, 10,  5,  0,
      20,  0,  5,  0, 34,  5, 34, 34, 10,  0,
       0,  0, 15,  5, 10,  0, 20,  0,  0, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
      34, 34, 34, 34, 34, 34
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const struct keyword wordlist[] =
  {
    {""}, {""},
#line 36 "keywords.gperf"
    {"OR", IDENTIFIER_OR},
#line 14 "keywords.gperf"
    {"obj", IDENTIFIER_OBJ},
#line 16 "keywords.gperf"
    {"enum", IDENTIFIER_ENUM},
#line 33 "keywords.gperf"
    {"const", TYPE_CONST},
#line 15 "keywords.gperf"
    {"struct", IDENTIFIER_STRUCT},
#line 39 "keywords.gperf"
    {"BITWISE", IDENTIFIER_BITWISE},
#line 28 "keywords.gperf"
    {"int", TYPE_INT},
#line 17 "keywords.gperf"
    {"namespace", IDENTIFIER_NAMESPACE},
#line 24 "keywords.gperf"
    {"quiet", PRAGMA_QUIET},
#line 25 "keywords.gperf"
    {"strict", PRAGMA_STRICT},
    {""},
#line 22 "keywords.gperf"
    {"optimize", PRAGMA_OPTIMIZE},
#line 29 "keywords.gperf"
    {"char", TYPE_CHAR},
#line 19 "keywords.gperf"
    {"macro", IDENTIFIER_MACRO},
#line 18 "keywords.gperf"
    {"return", IDENTIFIER_RETURN},
    {""},
#line 26 "keywords.gperf"
    {"ptr", IDENTIFIER_REFERENCE},
#line 32 "keywords.gperf"
    {"bool", TYPE_BOOL},
#line 30 "keywords.gperf"
    {"float", TYPE_FLOAT},
#line 20 "keywords.gperf"
    {"pragma", IDENTIFIER_PRAGMA},
#line 34 "keywords.gperf"
    {"private", TYPE_PRIVATE},
#line 27 "keywords.gperf"
    {"dfr", IDENTIFIER_DEFERENCE},
#line 37 "keywords.gperf"
    {"AND", IDENTIFIER_AND},
    {""},
#line 21 "keywords.gperf"
    {"targer", PRAGMA_TARGET},
    {""},
#line 38 "keywords.gperf"
    {"XOR", IDENTIFIER_XOR},
    {""},
#line 23 "keywords.gperf"
    {"diagnostic", PRAGMA_DIAGNOSTIC},
#line 31 "keywords.gperf"
    {"double", TYPE_DOUBLE},
    {""},
#line 35 "keywords.gperf"
    {"NOT", IDENTIFIER_NOT}
  };

const struct keyword *
in_word_set (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
