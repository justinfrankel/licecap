/*
** This file is part of the C Utility Toolkit (CUTK)
** Copyright (C) 2002-2005 Chris Osgood
** http://www.cutk.org
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** Please report all bugs and problems to "bugs at cutk.org".
*/

/*
** See StrHmap.h for more information.
**
** $Id: svg_hash.c,v 1.1 2005/02/14 17:26:26 pb Exp $
*/

/* Inline */
#ifdef _MSC_VER
#define INLINE_X static __inline
#else
#define INLINE_X static inline
#endif

#ifdef NO_INLINE
#define INLINE static
#else
#define INLINE INLINE_X
#endif

#include <stdlib.h>
#include "svg_hash.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* Utility macro for getting the element size */
#define _svg_hash_aryesize(x) ((*((svg_array_t**)(void*)(x))-1)->esize)

/* Macros for low overhead API calls */
#define _svg_hash_arysize(x)         ((*((svg_array_t**)(void*)(x))-1)->size)
#define _svg_hash_arymax(x)          ((*((svg_array_t**)(void*)(x))-1)->max)
#define _svg_hash_aryalloc(x)        _svg_hash_aryalloc_(sizeof(x))
#define _svg_hash_aryallocsize(x,y)  _svg_hash_aryallocsize_(sizeof(x),y)
#define _svg_hash_aryclear(x)        (((*((svg_array_t**)(void*)(x))-1)->size)=0)

/* Main array type (not used externally but arysize/max macros use it) */
typedef struct svg_array_t
{
  size_t size;   /* Number of elements in array       */
  size_t max;    /* Allocated memory (max # elements) */
  size_t esize;  /* Element size; sizeof(ElementType) */
  char ary[];    /* Raw array data                    */
} svg_array_t;

/**
 * Allocate a new empty array.  Normally this function is not called directly.
 * Use the aryalloc() macro instead.  This allows you to specify the element
 * type rather than the element size.  Example: "int* array = aryalloc(int);"
 *
 * @param esize Element size.  This is the size (in bytes) of each element in
 *              the array.  This is normally just the sizeof(ElementType).
 * @return This returns a pointer to the array data (not the array structure)
 *         or NULL on failure.
 */
static void * 
_svg_hash_aryalloc_(size_t esize)
{
  svg_array_t* ary = (svg_array_t*)malloc(sizeof(svg_array_t));
  if (ary == NULL)
    return NULL;

  ary->max = 0;
  ary->size = 0;
  ary->esize = esize;

  return &ary->ary;
}

/**
 * Allocate a new array of a specified size.  Normally this function is not
 * called directly.  Use the aryallocsize() macro instead.
 * Example: "int* array = aryallocsize(int, 2000);"
 *
 * @param esize Element size.  This is the size (in bytes) of each element in
 *              the array.  This is normally just the sizeof(ElementType).
 * @param size Number of items in array
 * @return This returns a pointer to the array data (not the array structure)
 *         or NULL on failure.
 */
static void * 
_svg_hash_aryallocsize_(size_t esize, size_t size)
{
  svg_array_t* ary = (svg_array_t*)malloc(sizeof(svg_array_t) + esize * size);
  if (ary == NULL)
    return NULL;

  ary->max = size;
  ary->size = size;
  ary->esize = esize;

  return &ary->ary;
}

/**
 * This releases all memory held by an array.
 *
 * @param ary Pointer to an array pointer
 */
static void 
_svg_hash_aryfree(void* ary)
{
  free(*(svg_array_t**)ary - 1);
  *(void**)ary = NULL;
}

/**
 * This preallocates memory for a certain maximum number of elements.
 * Note that this does not change the size of the array unless max
 * is less than the current size of the array.
 *
 * @param ary Pointer to an array pointer
 * @param max Number of elements to preallocate
 * @return 0 on success; non-zero on failure
 */
static int 
_svg_hash_aryreserve (void* ary, size_t max)
{
  if (_svg_hash_arymax (ary) == max)
    return 0;
  else
    {
      void* temp = realloc( *(svg_array_t**)ary - 1,
			    sizeof(svg_array_t) + max * _svg_hash_aryesize(ary) );

      if (temp == 0)
	return 1;

      *(void**)ary = &((svg_array_t*)temp)->ary;
    }

  _svg_hash_arymax(ary) = max;

  if (_svg_hash_arymax(ary) < _svg_hash_arysize(ary))
    _svg_hash_arysize(ary) = _svg_hash_arymax(ary);

  return 0;
}

/**
 * This resizes an array.  Note that this does not free any memory
 * held by the array if the size is smaller than the current size.
 *
 * @param ary Pointer to an array pointer
 * @param size New size of array
 * @return 0 on success; non-zero on failure
 */
static int 
_svg_hash_aryresize(void* ary, size_t size)
{
  if (size > _svg_hash_arymax(ary) && _svg_hash_aryreserve (ary, size) != 0)
    return 1;

  _svg_hash_arysize(ary) = size;

  return 0;
}

/**
 * This adds and copies a new element to the end of the array.
 *
 * @param ary Pointer to an array pointer
 * @param element This value is copied into the new value at the end of the
 *                array
 * @return 0 on success; non-zero on failure
 */
static int 
_svg_hash_aryappend(void* ary, const void* element)
{
  if ((_svg_hash_arysize(ary) + 1 > _svg_hash_arymax(ary) &&
       _svg_hash_aryreserve(ary, _svg_hash_arysize(ary) * 2) != 0) ||
      _svg_hash_aryresize(ary, _svg_hash_arysize(ary) + 1) != 0)
    {
      return 1;
    }

  memcpy (*(char**)ary + (_svg_hash_arysize(ary) - 1) * _svg_hash_aryesize(ary),
	  element, _svg_hash_aryesize(ary));

  return 0;
}

/* Some prime values to use when sizing the internal hash table */
static const size_t hash_primelist[] =
{
   53u, 97u, 193u, 389u, 769u, 1543u, 3079u, 6151u, 12289u, 24593u,
   49157u, 98317u, 196613u, 393241u, 786433u, 1572869u, 3145739u,
   6291469u, 12582917u, 25165843u, 50331653u, 100663319u, 201326611u,
   402653189u, 805306457u, 1610612741u, 3221225473u, 4294967291u
};

/* Returns the next prime larger than the specified size */
INLINE size_t 
_svg_hash_next_prime (size_t size)
{
  size_t i;

  for(i = 0; hash_primelist[i] < size &&
	i < sizeof (hash_primelist) / sizeof (size_t); i++);

  return hash_primelist[i];
}

/* Fast string hashing routine */
INLINE size_t 
_svg_hash_str_hash (const svg_xml_hash_table_t * map, const char* s)
{
  size_t i;
  for (i = 0; *s; s++)
    i = 31 * i + *s;
  return i % _svg_hash_arysize (&map->hash_tbl);
}

/* Fast string equality test.  In most cases this is faster than strcmp */
INLINE char 
_svg_hash_streq (const char* s1, const char* s2)
{
  if (s1 == s2) return 1;
  if (!s1 || !s2) return 0;
  while (*s1 != '\0' && *s1 == *s2)
    s1++, s2++;
  return *s1 == *s2;
}

/**
 * Reallocates a hash map.  If the size is smaller than the actual
 * number of elements in the hash map then the effect will be to reduce
 * memory usage if possible.  The size can be larger than the actual number
 * of elements in the hash map which will allow lots of insertions to go
 * faster since the table won't need to be resized as often.
 * @param hashmap Pointer to hash map
 * @param size Number of elements to preallocate
 * @return 0 on success; non-zero on failure
 */
static int 
_svg_hash_table_realloc (svg_xml_hash_table_t *hashmap, size_t size)
{
   size_t i;
   size_t hash;

   if (size <= _svg_hash_arysize(&hashmap->hash_tbl))
      return 0;

   if (_svg_hash_aryresize(&hashmap->hash_tbl, _svg_hash_next_prime (size)))
      return 1;

   memset(hashmap->hash_tbl, 0xFF,
          _svg_hash_arysize(&hashmap->hash_tbl) * sizeof(size_t));

   for (i = 0; i < _svg_hash_arysize(&hashmap->items); i++)
   {
      if (hashmap->items[i].key != NULL)
      {
         hash = _svg_hash_str_hash (hashmap, hashmap->items[i].key);
         hashmap->items[i].next = hashmap->hash_tbl[hash];
         hashmap->hash_tbl[hash] = i;
      }
   }

   return 0;
}

/* Internal function used by StrHmapInsert and StrHmapReplace */
INLINE int 
_str_hash_insert_new (svg_xml_hash_table_t * hashmap, const char* key, const void* item, size_t hash)
{
  size_t newitemnum;
  svg_hash_item_t newitem;

  newitem.key = key;
  newitem.item = item;

  if (hashmap->deleted != SIZE_MAX)
    {
      newitemnum = hashmap->deleted;
      hashmap->deleted = hashmap->items[newitemnum].next;
      hashmap->items[newitemnum] = newitem;
    }
  else
    {
      newitemnum = _svg_hash_arysize(&hashmap->items);
      if (_svg_hash_aryappend(&hashmap->items, &newitem))
	return 1;
    }

  hashmap->items[newitemnum].next = hashmap->hash_tbl[hash];
  hashmap->hash_tbl[hash] = newitemnum;

  /* For the following it's not really an insert error if rehash fails
     so we don't return an error if it does.  However, the hash table will
     become slow if rehash keeps failing.  Not sure how to notify the
     application of such a condition.  Other more serious errors will
     probably be happening if rehash fails (eg. out of memory). */
  if (_svg_hash_arysize(&hashmap->items) > _svg_hash_arysize(&hashmap->hash_tbl))
    _svg_hash_table_realloc (hashmap, _svg_hash_arysize (&hashmap->items)); 

  return 0;   
}

/* External API */

/**
 * Allocate a new empty hash map.
 * @param size Specifies intial expected number of items in the hash map (can
 *             be 0).
 * @return Returns a pointer to the new hash map or NULL on failure.
 */
svg_xml_hash_table_t *
_svg_xml_hash_create (size_t size)
{
  svg_xml_hash_table_t * hashmap = (svg_xml_hash_table_t *)malloc (sizeof (svg_xml_hash_table_t));
  if (hashmap == NULL)
    return NULL;

  hashmap->items = _svg_hash_aryalloc(svg_hash_item_t);
  if (hashmap->items == NULL)
    {
      free(hashmap);
      return NULL;
    }
   
  hashmap->hash_tbl = _svg_hash_aryallocsize(size_t, _svg_hash_next_prime(size));

  if (hashmap->hash_tbl != NULL)
    memset(hashmap->hash_tbl, 0xFF,
	   _svg_hash_arysize(&hashmap->hash_tbl) * sizeof(size_t));

  if (hashmap->hash_tbl == NULL)
    {
      _svg_hash_aryfree(&hashmap->items);
      free(hashmap);
      return NULL;
    }

  hashmap->deleted = SIZE_MAX;
   
  return hashmap;
}

/**
 * Releases all memory held by a hash map.
 * @param hashmap Pointer to a hash map
 */
void 
_svg_xml_hash_free (svg_xml_hash_table_t * hashmap)
{
  _svg_hash_aryfree(&hashmap->hash_tbl);
  _svg_hash_aryfree(&hashmap->items);
  hashmap->hash_tbl = NULL;
  hashmap->items = NULL;
  free(hashmap);
}


/**
 * Finds an element in the hash map.
 * @param hashmap Pointer to hash map
 * @param key Key to search for
 * @return Pointer to element in hash map or NULL if key not found
 */
void *
_svg_xml_hash_lookup (svg_xml_hash_table_t * hashmap, const char* key)
{
  size_t hash = _svg_hash_str_hash (hashmap, key);
  size_t itemnum = hashmap->hash_tbl[hash];

  for (; itemnum != SIZE_MAX; itemnum = hashmap->items[itemnum].next)
    {
      if (_svg_hash_streq (hashmap->items[itemnum].key, key))
	return (void*)hashmap->items[itemnum].item;
    }

  return NULL;
}

/**
 * Inserts a new element into the hash map.  This will return an error if the
 * key already exists.
 * @param hashmap Pointer to hash map
 * @param key Key to insert element under
 * @param item Item pointer to insert into hash map
 * @return 0 on success; non-zero on failure
 */
int 
_svg_xml_hash_add_entry (svg_xml_hash_table_t *hashmap, const char* key, const void* item)
{
  size_t hash = _svg_hash_str_hash (hashmap, key);
  size_t itemnum = hashmap->hash_tbl[hash];

  for (; itemnum != SIZE_MAX; itemnum = hashmap->items[itemnum].next)
    {
      if (_svg_hash_streq (hashmap->items[itemnum].key, key))
	return 1; /* key exists */
    }

  return _str_hash_insert_new (hashmap, key, item, hash);
}
