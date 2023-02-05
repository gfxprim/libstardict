// SPDX-License-Identifier: LGPL-2.1-or-later
/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef LIBSTARDICT_H__
#define LIBSTARDICT_H__

#include <stdint.h>

struct dict_dz;

enum sd_entry_fmt {
	/* Plain text in UTF8 */
	SD_ENTRY_UTF8_TEXT = 'm',
	/* Pango html-like markup UTF8 */
	SD_ENTRY_PANGO_MARKUP = 'g',
	/* HTML markup */
	SD_ENTRY_HTML = 'h',
	/* http://xdxf.sourceforge.net */
	SD_ENTRY_XDXF = 'x',
	/* Chinese YinBiao or Japanese Kana utf8 string */
	SD_ENTRY_CHINESE_JAPANESE = 'y',
};

#define SD_DICT_BOOKNAME_MAX 64

struct sd_dict {
	/* set if sametypesequence= is set in the ifo file */
	char entry_fmt;
	/* number of words in the index file */
	unsigned int word_count;
	/* (uncompressed) index file size */
	unsigned int idx_filesize;

	/* book name */
	char book_name[SD_DICT_BOOKNAME_MAX];

	/*
	 * Pointer to compressed dictionary, set if dictionary is compressed.
	 *
	 * DO NOT TOUCH
	 */
	struct dict_dz *dict_dz;

	/* dictionary index and word list lookup array */
	void *idx;
	char **word_list;
};

/**
 * @brief Opens a stardict format dictionary.
 *
 * @path Path to a dictionary directory.
 * @name A dictionary name.
 *
 * @return A dictionary or NULL in a case of a failure.
 */
struct sd_dict *sd_open_dict(const char *path, const char *name);

/**
 * @brief Closes a dictionary.
 *
 * @self A dictionary.
 */
void sd_close_dict(struct sd_dict *self);

/**
 * A result range.
 */
struct sd_lookup_res {
	unsigned int min;
	unsigned int max;
};

/**
 * @brief Looks up a range in the sorted index.
 *
 * @self A dictionary.
 * @prefix An utf8 string prefix to look for.
 * @res A range to store the result into.
 *
 * @return Returns zero if prefix wasn't found, non-zero otherwise.
 */
int sd_lookup_dict(struct sd_dict *self, const char *prefix, struct sd_lookup_res *res);

/**
 * @brief Returns a number of entries in look result range.
 */
static inline unsigned int sd_lookup_res_cnt(struct sd_lookup_res *res)
{
	return res->max - res->min + 1;
}

/**
 * @brief Returns a string for a given index
 *
 * @dict A dictionary
 * @idx An index into a word lookup table
 * @return A string or NULL if index is out of bounds.
 */
const char *sd_idx_to_word(struct sd_dict *dict, unsigned int idx);

struct sd_entry {
	/* sd_entry_fmt */
	char fmt;
	/* entry data */
	char data[];
};

/**
 * @brief Loads an entry at index.
 *
 * @dict A dictionary.
 * @idx An entry index.
 *
 * @return Newly allocated entry or NULL on a failure.
 */
struct sd_entry *sd_get_entry(struct sd_dict *dict, unsigned int idx);

/**
 * @brief Frees an entry.
 *
 * @entry An entry returned from sd_get_entry()
 */
void sd_free_entry(struct sd_entry *entry);

struct sd_dict_path {
	struct sd_dict_path *next;
	const char *dir;
	char book_name[SD_DICT_BOOKNAME_MAX];
	char fname[];
};

struct sd_dict_paths {
	unsigned int dict_cnt;
	char *home_sd_dir;
	struct sd_dict_path **paths;
};

/**
 * @brief Scans system for stardict dictionaries.
 *
 * @paths A strucutre to fill the scanned directories into.
 */
void sd_lookup_dict_paths(struct sd_dict_paths *paths);

/**
 * @brief Frees stardict dictionary paths.
 *
 * @paths A structure previously filled by the lookup.
 */
void sd_free_dict_paths(struct sd_dict_paths *paths);

#endif /* LIBSTARDICT_H__ */
