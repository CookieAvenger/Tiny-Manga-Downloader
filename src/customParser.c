#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*  Copyright 2012, 2016 Christoph Gärtner
    This file is under the Boost Software License, Version 1.0
*/

#include "customParser.h"

#include <errno.h>
#include <stdbool.h>

#define UNICODE_MAX 0x10FFFFul

static const char *const NAMED_ENTITIES[][2] = {
    { "AElig;", "Æ" },
    { "Aacute;", "Á" },
    { "Acirc;", "Â" },
    { "Agrave;", "À" },
    { "Alpha;", "Α" },
    { "Aring;", "Å" },
    { "Atilde;", "Ã" },
    { "Auml;", "Ä" },
    { "Beta;", "Β" },
    { "Ccedil;", "Ç" },
    { "Chi;", "Χ" },
    { "Dagger;", "‡" },
    { "Delta;", "Δ" },
    { "ETH;", "Ð" },
    { "Eacute;", "É" },
    { "Ecirc;", "Ê" },
    { "Egrave;", "È" },
    { "Epsilon;", "Ε" },
    { "Eta;", "Η" },
    { "Euml;", "Ë" },
    { "Gamma;", "Γ" },
    { "Iacute;", "Í" },
    { "Icirc;", "Î" },
    { "Igrave;", "Ì" },
    { "Iota;", "Ι" },
    { "Iuml;", "Ï" },
    { "Kappa;", "Κ" },
    { "Lambda;", "Λ" },
    { "Mu;", "Μ" },
    { "Ntilde;", "Ñ" },
    { "Nu;", "Ν" },
    { "OElig;", "Œ" },
    { "Oacute;", "Ó" },
    { "Ocirc;", "Ô" },
    { "Ograve;", "Ò" },
    { "Omega;", "Ω" },
    { "Omicron;", "Ο" },
    { "Oslash;", "Ø" },
    { "Otilde;", "Õ" },
    { "Ouml;", "Ö" },
    { "Phi;", "Φ" },
    { "Pi;", "Π" },
    { "Prime;", "″" },
    { "Psi;", "Ψ" },
    { "Rho;", "Ρ" },
    { "Scaron;", "Š" },
    { "Sigma;", "Σ" },
    { "THORN;", "Þ" },
    { "Tau;", "Τ" },
    { "Theta;", "Θ" },
    { "Uacute;", "Ú" },
    { "Ucirc;", "Û" },
    { "Ugrave;", "Ù" },
    { "Upsilon;", "Υ" },
    { "Uuml;", "Ü" },
    { "Xi;", "Ξ" },
    { "Yacute;", "Ý" },
    { "Yuml;", "Ÿ" },
    { "Zeta;", "Ζ" },
    { "aacute;", "á" },
    { "acirc;", "â" },
    { "acute;", "´" },
    { "aelig;", "æ" },
    { "agrave;", "à" },
    { "alefsym;", "ℵ" },
    { "alpha;", "α" },
    { "amp;", "&" },
    { "and;", "∧" },
    { "ang;", "∠" },
    { "apos;", "'" },
    { "aring;", "å" },
    { "asymp;", "≈" },
    { "atilde;", "ã" },
    { "auml;", "ä" },
    { "bdquo;", "„" },
    { "beta;", "β" },
    { "brvbar;", "¦" },
    { "bull;", "•" },
    { "cap;", "∩" },
    { "ccedil;", "ç" },
    { "cedil;", "¸" },
    { "cent;", "¢" },
    { "chi;", "χ" },
    { "circ;", "ˆ" },
    { "clubs;", "♣" },
    { "cong;", "≅" },
    { "copy;", "©" },
    { "crarr;", "↵" },
    { "cup;", "∪" },
    { "curren;", "¤" },
    { "dArr;", "⇓" },
    { "dagger;", "†" },
    { "darr;", "↓" },
    { "deg;", "°" },
    { "delta;", "δ" },
    { "diams;", "♦" },
    { "divide;", "÷" },
    { "eacute;", "é" },
    { "ecirc;", "ê" },
    { "egrave;", "è" },
    { "empty;", "∅" },
    { "emsp;", "\xE2\x80\x83" },
    { "ensp;", "\xE2\x80\x82" },
    { "epsilon;", "ε" },
    { "equiv;", "≡" },
    { "eta;", "η" },
    { "eth;", "ð" },
    { "euml;", "ë" },
    { "euro;", "€" },
    { "exist;", "∃" },
    { "fnof;", "ƒ" },
    { "forall;", "∀" },
    { "frac12;", "½" },
    { "frac14;", "¼" },
    { "frac34;", "¾" },
    { "frasl;", "⁄" },
    { "gamma;", "γ" },
    { "ge;", "≥" },
    { "gt;", ">" },
    { "hArr;", "⇔" },
    { "harr;", "↔" },
    { "hearts;", "♥" },
    { "hellip;", "…" },
    { "iacute;", "í" },
    { "icirc;", "î" },
    { "iexcl;", "¡" },
    { "igrave;", "ì" },
    { "image;", "ℑ" },
    { "infin;", "∞" },
    { "int;", "∫" },
    { "iota;", "ι" },
    { "iquest;", "¿" },
    { "isin;", "∈" },
    { "iuml;", "ï" },
    { "kappa;", "κ" },
    { "lArr;", "⇐" },
    { "lambda;", "λ" },
    { "lang;", "〈" },
    { "laquo;", "«" },
    { "larr;", "←" },
    { "lceil;", "⌈" },
    { "ldquo;", "“" },
    { "le;", "≤" },
    { "lfloor;", "⌊" },
    { "lowast;", "∗" },
    { "loz;", "◊" },
    { "lrm;", "\xE2\x80\x8E" },
    { "lsaquo;", "‹" },
    { "lsquo;", "‘" },
    { "lt;", "<" },
    { "macr;", "¯" },
    { "mdash;", "—" },
    { "micro;", "µ" },
    { "middot;", "·" },
    { "minus;", "−" },
    { "mu;", "μ" },
    { "nabla;", "∇" },
    { "nbsp;", "\xC2\xA0" },
    { "ndash;", "–" },
    { "ne;", "≠" },
    { "ni;", "∋" },
    { "not;", "¬" },
    { "notin;", "∉" },
    { "nsub;", "⊄" },
    { "ntilde;", "ñ" },
    { "nu;", "ν" },
    { "oacute;", "ó" },
    { "ocirc;", "ô" },
    { "oelig;", "œ" },
    { "ograve;", "ò" },
    { "oline;", "‾" },
    { "omega;", "ω" },
    { "omicron;", "ο" },
    { "oplus;", "⊕" },
    { "or;", "∨" },
    { "ordf;", "ª" },
    { "ordm;", "º" },
    { "oslash;", "ø" },
    { "otilde;", "õ" },
    { "otimes;", "⊗" },
    { "ouml;", "ö" },
    { "para;", "¶" },
    { "part;", "∂" },
    { "permil;", "‰" },
    { "perp;", "⊥" },
    { "phi;", "φ" },
    { "pi;", "π" },
    { "piv;", "ϖ" },
    { "plusmn;", "±" },
    { "pound;", "£" },
    { "prime;", "′" },
    { "prod;", "∏" },
    { "prop;", "∝" },
    { "psi;", "ψ" },
    { "quot;", "\"" },
    { "rArr;", "⇒" },
    { "radic;", "√" },
    { "rang;", "〉" },
    { "raquo;", "»" },
    { "rarr;", "→" },
    { "rceil;", "⌉" },
    { "rdquo;", "”" },
    { "real;", "ℜ" },
    { "reg;", "®" },
    { "rfloor;", "⌋" },
    { "rho;", "ρ" },
    { "rlm;", "\xE2\x80\x8F" },
    { "rsaquo;", "›" },
    { "rsquo;", "’" },
    { "sbquo;", "‚" },
    { "scaron;", "š" },
    { "sdot;", "⋅" },
    { "sect;", "§" },
    { "shy;", "\xC2\xAD" },
    { "sigma;", "σ" },
    { "sigmaf;", "ς" },
    { "sim;", "∼" },
    { "spades;", "♠" },
    { "sub;", "⊂" },
    { "sube;", "⊆" },
    { "sum;", "∑" },
    { "sup1;", "¹" },
    { "sup2;", "²" },
    { "sup3;", "³" },
    { "sup;", "⊃" },
    { "supe;", "⊇" },
    { "szlig;", "ß" },
    { "tau;", "τ" },
    { "there4;", "∴" },
    { "theta;", "θ" },
    { "thetasym;", "ϑ" },
    { "thinsp;", "\xE2\x80\x89" },
    { "thorn;", "þ" },
    { "tilde;", "˜" },
    { "times;", "×" },
    { "trade;", "™" },
    { "uArr;", "⇑" },
    { "uacute;", "ú" },
    { "uarr;", "↑" },
    { "ucirc;", "û" },
    { "ugrave;", "ù" },
    { "uml;", "¨" },
    { "upsih;", "ϒ" },
    { "upsilon;", "υ" },
    { "uuml;", "ü" },
    { "weierp;", "℘" },
    { "xi;", "ξ" },
    { "yacute;", "ý" },
    { "yen;", "¥" },
    { "yuml;", "ÿ" },
    { "zeta;", "ζ" },
    { "zwj;", "\xE2\x80\x8D" },
    { "zwnj;", "\xE2\x80\x8C" }
};

static int cmp(const void *key, const void *value)
{
    return strncmp((const char *)key, *(const char *const *)value,
        strlen(*(const char *const *)value));
}

static const char *get_named_entity(const char *name)
{
    const char *const *entity = (const char *const *)bsearch(name,
        NAMED_ENTITIES, sizeof NAMED_ENTITIES / sizeof *NAMED_ENTITIES,
        sizeof *NAMED_ENTITIES, cmp);

    return entity ? entity[1] : NULL;
}

static size_t putc_utf8(unsigned long cp, char *buffer)
{
    unsigned char *bytes = (unsigned char *)buffer;

    if(cp <= 0x007Ful)
    {
        bytes[0] = (unsigned char)cp;
        return 1;
    }

    if(cp <= 0x07FFul)
    {
        bytes[1] = (unsigned char)((2 << 6) | (cp & 0x3F));
        bytes[0] = (unsigned char)((6 << 5) | (cp >> 6));
        return 2;
    }

    if(cp <= 0xFFFFul)
    {
        bytes[2] = (unsigned char)(( 2 << 6) | ( cp       & 0x3F));
        bytes[1] = (unsigned char)(( 2 << 6) | ((cp >> 6) & 0x3F));
        bytes[0] = (unsigned char)((14 << 4) |  (cp >> 12));
        return 3;
    }

    if(cp <= 0x10FFFFul)
    {
        bytes[3] = (unsigned char)(( 2 << 6) | ( cp        & 0x3F));
        bytes[2] = (unsigned char)(( 2 << 6) | ((cp >>  6) & 0x3F));
        bytes[1] = (unsigned char)(( 2 << 6) | ((cp >> 12) & 0x3F));
        bytes[0] = (unsigned char)((30 << 3) |  (cp >> 18));
        return 4;
    }

    return 0;
}

static bool parse_entity(
    const char *current, char **to, const char **from)
{
    bool endPlus = false;
    const char *end = strchr(current, ';');
    if (!end && current[1] == '#') {
        end = strchr(current, ' ');
        if (end) {
            end = end - 1;
            endPlus = true;
        }
    }
    if(!end) return 0;

    if(current[1] == '#')
    {
        char *tail = NULL;
        int errno_save = errno;
        bool hex = current[2] == 'x' || current[2] == 'X';

        errno = 0;
        unsigned long cp = strtoul(
            current + (hex ? 3 : 2), &tail, hex ? 16 : 10);

        const char *toEnd = end;
        if (endPlus) {
           toEnd = end + 1; 
        }

        bool fail = errno || tail != toEnd || cp > UNICODE_MAX;
        errno = errno_save;
        if(fail) return 0;

        *to += putc_utf8(cp, *to);
        *from = end + 1;

        return 1;
    }
    else
    {
        const char *entity = get_named_entity(&current[1]);
        if(!entity) return 0;

        size_t len = strlen(entity);
        memcpy(*to, entity, len);

        *to += len;
        *from = end + 1;

        return 1;
    }
}

size_t decode_html_entities_utf8(char *dest, const char *src)
{
    if(!src) src = dest;

    char *to = dest;
    const char *from = src;

    for(const char *current; (current = strchr(from, '&'));)
    {
        memmove(to, from, (size_t)(current - from));
        to += current - from;

        if(parse_entity(current, &to, &from))
            continue;

        from = current;
        *to++ = *from++;
    }

    size_t remaining = strlen(from);

    memmove(to, from, remaining);
    to += remaining;
    *to = 0;

    return (size_t)(to - dest);
}

//NOTE: I know this looks wrong. It's meant to be.
unsigned long parse_hexadecimal_to_one_long(char *hexadecimal) {
    unsigned long decimal = 0;
    int value, i = 0, length = strlen(hexadecimal);
    if (--length > 15) {
        length = 15;
    }
    while (hexadecimal[i] != '\0' && length >= 0) {
        switch (hexadecimal[i]) {
            case '0':
                value = 0;
                break;
            case '1':
                value = 1;
                break;
            case '2':
                value = 2;
                break;
            case '3':
                value = 3;
                break;
            case '4':
                value = 4;
                break;
            case '5':
                value = 5;
                break;
            case '6':
                value = 6;
                break;
            case '7':
                value = 7;
                break;
            case '8':
                value = 8;
                break;
            case '9':
                value = 9;
                break;
            case 'a':
            case 'A':
                value = 10;
                break;
            case 'b':
            case 'B':
                value = 11;
                break;
            case 'c':
            case 'C':
                value = 12;
                break;
            case 'd':
            case 'D':
                value = 13;
                break;
            case 'e':
            case 'E':
                value = 14;
                break;
            case 'f':
            case 'F':
                value = 15;
                break;
            default:
                //char can only be 1 byte, we are after 4 least significant bits
                //so we bit mask it :)
                value = hexadecimal[i] & 0x0f;
                break;
        }
        decimal += value * pow(16, length);
        i++, length--;
    }
    return decimal;
}

//Below URL methods taken from http://www.geekhideout.com/urlcode.shtml
//Kudos to Fred Bullback

/* Converts a hex character to its integer value */
char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
    char *pstr = str, *buf = (char *) malloc(sizeof(char)
            * (strlen(str) * 3 + 1)), *pbuf = buf;
    if (buf == NULL) {
        exit(21);
    }
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
            *pbuf++ = *pstr;
        } else if (*pstr == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
    char *pstr = str, *buf = (char *) malloc(sizeof(char) 
            * (strlen(str) + 1)), *pbuf = buf;
    if (buf == NULL) {
        exit(21);
    }
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        } else if (*pstr == '+') { 
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}
