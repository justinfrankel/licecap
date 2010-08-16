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
** A fast hash map implementation similar to STL hash_map<char*,void*>.  It's
** very important to note that the keys in the hash map are just stored as
** pointers, therefore you can't reuse the same string pointer for inserting
** different keys.  It's also not a good idea to change the value of a key
** while it's still in a hash map.
**
** $Id: svg_hash.h,v 1.1 2005/02/14 17:26:26 pb Exp $
*/

#ifndef _STRHMAP_H
#define _STRHMAP_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type used to store items in the hash map */
typedef struct svg_hash_item
{
   const char* key;
   const void* item;
   size_t next;
} svg_hash_item_t;

/* Hash map type.  "items" is an array that can be used for iteration. */
/* When iterating, test the item->key value for NULL (erased items).   */
typedef struct svg_xml_hash_table
{
   size_t* hash_tbl;
   svg_hash_item_t *items;
   size_t deleted;
} svg_xml_hash_table_t;

svg_xml_hash_table_t *_svg_xml_hash_create (size_t size);
void  _svg_xml_hash_free (svg_xml_hash_table_t *hashmap);
void* _svg_xml_hash_lookup (svg_xml_hash_table_t *hashmap, const char *key);
int _svg_xml_hash_add_entry (svg_xml_hash_table_t *hashmap, const char *key, const void *item);

#ifdef __cplusplus
}
#endif

#endif /* _STRHMAP_H */
