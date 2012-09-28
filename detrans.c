/* Copyright (c) 2010-2012, Artem Shinkarov <artyom.shinkaroff@gmail.com>

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#include "detrans.h"
#include "trie.h"

/* A structure to static replacements.  Used to store
   correspondence between russian small and capital
   letters.  */
struct symbol
{
  const char *str;
  const char *repl;
};


/* In order to optimise the search turn-arounds we are
   going to use a trie to keep a match between the translit
   word/letter and russian word.  */
static struct trie *detrans_trie = NULL;


/* List of russian words, attached to the DETRANS_TRIE.  We
   allocate it in a separate array, as trie doesn't know that
   we keep pointers as an attached info, so it wouldn't free it.
   XXX We can teach trie free-ing function of course, but
       that is a lazy variant.  */
static char **detrans_words = NULL;
static size_t detrans_size = 32, detrans_pos = 0;


/* Match small and capital russian letters.
   In order to minimize a number of words in the trie, we will do
   comparison in lowercase.  However, after a word/letter is
   being replaced, we may want to capitalize it.  In that case
   we need this table.

   XXX Keep in mind, that in utf-8 a letter may be longer than
       sizeof (char), so we use strings here.  Again, one may
       use ints, and do the encoding, but for the time being
       we do not do that.   */
static const struct symbol ru_cap[] = {
#define INPUT(a, b) {a, b},
#include "ru-capital-letters.def"
#undef INPUT
};

/* Legth of RU_CAP.  */
static size_t ru_cap_length = sizeof (ru_cap) / sizeof (struct symbol);

/* Length of one russian leetter in bytes.
   XXX We assume that all the russian letters have the same length,
       it is true in utf-8.  If it doesn't hold on your system,
       please adjust the binary search on RU_CAP.  */
static size_t ru_cap_str_length;


/* Add a word to the DETRANS table, and expand it if needed.  */
static char *
add_special_word (const char *word)
{
  char *ret = NULL;

  if (detrans_words == NULL)
    detrans_words = (char **) malloc (sizeof (char *) * detrans_size);

  if (detrans_pos == detrans_size - 1)
    {
      detrans_size *= 2;
      detrans_words =
	(char **) realloc (detrans_words, sizeof (char *) * detrans_size);
    }

  ret = strdup (word);
  detrans_words[detrans_pos++] = ret;
  return ret;
}


/* A helper function that coppies from INP to OUT until
   either it meets STOP character, or INP is exhausted.  */
static inline size_t
copy_until_character (char *inp, char *out, char stop)
{
  size_t len = 0;
  char *cpy = inp;

  while (*inp != stop && *inp != '\0')
    inp++;

  len = inp - cpy + 1;
  memcpy (out, cpy, len);
  return len;
}

/* Wrapper around COPY_UNTIL_CHARACTER.  */
#define copyuntil(__in, __out, __stop)                          \
do {                                                            \
  size_t __len = copy_until_character (__in, __out, __stop);    \
  __in += __len;                                                \
  __out += __len;                                               \
} while (0)


/* Build the trie and set the ru_cap_str_length.  */
void
detrans_init ()
{
  /* Set global length of russian small letter.  */
  ru_cap_str_length = strlen (ru_cap[0].str);

  /* Init the de-transliteration trie.  */
  detrans_trie = trie_new ();

  /* Fill the trie with data.
     1. ru-special-words.def is a list of exceptions.
     2. ru-replcament.def is a correspondence between the transliterated
     and russian letters.  */
#define INPUT(__a, __b) do {                                            \
     char * __w = add_special_word (__b);                               \
     trie_add_word (detrans_trie, __a, strlen (__a), (ssize_t)__w);     \
   } while (0);
#include "ru-special-words.def"
#include "ru-replacement.def"
#undef INPUT
}


/* Deallocate memory.  */
void
detrans_free ()
{
  size_t i;

  for (i = 0; i < detrans_pos; i++)
    free (detrans_words[i]);

  free (detrans_words);
  trie_free (detrans_trie);
}

/* Helper for binary search on RU_CAP.  */
static inline int
cmp_symbol (const void *k1, const void *k2)
{
  struct symbol *s1 = (struct symbol *) k1;
  struct symbol *s2 = (struct symbol *) k2;
  return strncmp (s1->str, s2->str, ru_cap_str_length);
}

/* Search RU_CAP for a small letter W.  */
static struct symbol *
search_capital_letter (const char *w)
{
  struct symbol s, *res;
  s.str = w;

  res = (struct symbol *) bsearch (&s, ru_cap, ru_cap_length,
				   sizeof (struct symbol), cmp_symbol);
  return res;
}

/* A helper structure to implement TRIE_MATCH_MAX.  */
struct trie_match_info
{
  struct trie *trie;
  ssize_t last;
  size_t len;
};

/* Find a longest prefix of WORD in the trie.  Function returns
   struct trie_match_info, where
        .trie is a trie that follows the prefix.
        .last is an info attached to the prefix.
        .len is the length of the prefix.  */
static struct trie_match_info
trie_match_max (struct trie_match_info tl,
		struct trie_match_info last_success, const char *word)
{
  struct child *child;

  if (tl.trie == NULL || *word == '\0')
    return last_success;

  child = trie_search_child (tl.trie, tolower (word[0]));

  if (!child)
    return last_success;

  if (child->last != TRIE_NOT_LAST)
    last_success = (struct trie_match_info)
                   {
                     .trie = child->next,
                     .last = child->last,
                     .len = tl.len + 1
                   };

  return trie_match_max ((struct trie_match_info)
			 {
                           .trie=child->next,
                           .last= child->last,
                           .len = tl.len + 1
                         },
                         last_success,
                         &word[1]);
}


/* Before de-transliteration we remove HTMML apostrophe,
   as this symbol is an essential part in ISO-9 codemap.  */
static char *
remove_apostrophes (const char *in)
{
  char *ret = (char *) malloc (strlen (in) + 1);
  char *retptr = ret;

  while (*in != '\0')
    if (*in == '&' && !strncmp (in, "&apos;", 6))
      *retptr++ = '\'', in += 6;
    else
      *retptr++ = *in++;

  *retptr = '\0';
  return ret;
}


/* Actual de-transliteration.  */
char *
detrans (char *inp)
{
  char *out = malloc (strlen (inp) * 10);
  char *outptr = out;

  char *inptr = remove_apostrophes (inp);
  char *in = inptr;

  while (*in != 0)
    {
      bool capital = isupper (*in);
      struct trie_match_info x, y;

      /* Couple of things with a special treatement.
         -- HTML tags.  */
      if (*in == '<')
	{
	  copyuntil (in, outptr, '>');
	  continue;
	}
      /* -- Naiive attempt to save URLs.  */
      else if (!strncmp (in, "http://", 7)
	       || !strncmp (in, "https://", 8)
	       || !strncmp (in, "www.", 4))
	{
	  copyuntil (in, outptr, ' ');
	  continue;
	}
      /* -- &xxxx; encoded symbols.
         XXX could it be that we will have '&' without
         terminating ';'?  Normally it doesn't happen
         but who knows...   */
      else if (*in == '&')
	{
	  copyuntil (in, outptr, ';');
	  continue;
	}


      /* Construct a data-structure for trie max-match.  */
      x = (struct trie_match_info)
          {.trie=detrans_trie, .last=TRIE_NOT_LAST, .len=0};

      /* Find the longest match in the trie.  */
      y = trie_match_max (x, x, in);

      /* The word is in the trie.  */
      if (y.last != TRIE_NOT_LAST)
	{
	  char *repl = (char *) y.last;
	  in += y.len;
	  /* Replace the first letter, if it's capital.
	     XXX yeah, we potentially loose the case inside
	     the word, if a special word was matched but then,
	     why the should have mixed case in it?  It is wrong
	     anyway, so we simply ignore it.  */
	  if (capital)
	    {
	      struct symbol *s = search_capital_letter (repl);
	      if (s)
		{
		  memcpy (outptr, s->repl, strlen (repl));
		  outptr += strlen (s->repl);
		  repl += strlen (s->str);
		}
	    }

	  /* Copy the rest of the word in case we had
	     a first capital, or all the word.  */
	  memcpy (outptr, repl, strlen (repl));
	  outptr += strlen (repl);
	}
      else
	{
	  *outptr++ = *in++;
	}
    }

  *outptr = '\0';
  free (inptr);

  return out;
}


#ifdef _DETRANS_BINARY
int
main (int argc, char *argv[])
{

  detrans_init ();

  #if defined (_CMD_TOOL)
    char * xtrans;
    if (argc < 2)
      {
        fprintf (stderr, "usage: %s <input-string>\n", argv[0]);
        goto out;
      }

    xtrans = detrans (argv[1]);
    fprintf (stdout, "xdetrans '%s' = '%s'\n", argv[1], xtrans);
    free (xtrans);

  #elif defined (_READ_FROM_FILE)
    FILE *f;
    char in_ru[256], in_tr[256];

    if (argc < 2)
      {
        fprintf (stderr, "usage: %s <file-name>\n", argv[0]);
        goto out;
      }

     f = fopen (argv[1], "r");

     while (fscanf (f, "%s\t%s", in_ru, in_tr) != EOF)
       {
         char *out = detrans (in_tr);
         if (strcmp (in_ru, out))
	        fprintf (stdout, "INPUT (\"%s\",\t\"%s\")\n", in_tr, out);
         free (out);
       }

     fclose (f);
   #endif

out:
  detrans_free ();
  return EXIT_SUCCESS;
}
#endif

