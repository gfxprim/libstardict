// SPDX-License-Identifier: LGPL-2.1-or-later
/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "libstardict.h"

int main(int argc, char *argv[])
{
	struct sd_dict_paths paths;
	struct sd_dict *dict;
	unsigned int i, d_idx = 0, raw_entry = 0;
	int opt;

	while ((opt = getopt(argc, argv, "d:r")) != -1) {
		switch (opt) {
		case 'd':
			d_idx = atoi(optarg);
		break;
		case 'r':
			raw_entry = 1;
		break;
		default:
			printf("Invalid option %c\n", opt);
		}
	}

	sd_lookup_dict_paths(&paths);
	if (!paths.paths) {
		printf("No dictionaries found\n");
		return 1;
	}

	printf("Found %u dictionaries\n", paths.dict_cnt);

	for (i = 0; i < paths.dict_cnt; i++)
		printf(" %2i '%s' - %s.ifo\n", i, paths.paths[i]->book_name, paths.paths[i]->fname);
	printf("\n");

	if (d_idx >= paths.dict_cnt) {
		printf("Dict index too large %i\n", d_idx);
		return 1;
	}

	printf("Opening dict '%s'\n", paths.paths[d_idx]->fname);

	dict = sd_open_dict(paths.paths[d_idx]->dir, paths.paths[d_idx]->fname);
	if (!dict) {
		printf("Failed to load dict!\n");
		return 1;
	}

	printf("Dict name '%s'\n", dict->book_name);

	sd_free_dict_paths(&paths);

	printf("Dict loaded word count=%u\n", dict->word_count);

	struct sd_lookup_res res;

	if (!argv[optind])
		return 0;

	printf("Lookup '%s' ... ", argv[optind]);

	int ret = sd_lookup_dict(dict, argv[optind], &res);

	if (!ret) {
		printf("none\n");
		return 0;
	} else {
		printf("%i\n", sd_lookup_res_cnt(&res));
	}

	printf("Result %u - %u\n", res.min, res.max);
	printf("%s - %s\n", sd_idx_to_word(dict, res.min), sd_idx_to_word(dict, res.max));

	for (unsigned int i = res.min; i <= res.max; i++)
		printf("%s\n", sd_idx_to_word(dict, i));

	struct sd_entry *entry = sd_get_entry(dict, res.min);
	if (entry) {
		if (!raw_entry)
			sd_strip_entry(entry);
		printf("%s\n", entry->data);
	}

	sd_free_entry(entry);
	sd_close_dict(dict);

	return 0;
}
