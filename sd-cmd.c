
// SPDX-License-Identifier: LGPL-2.1-or-later
/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <stdio.h>
#include "libstardict.h"

int main(int argc, char *argv[])
{
	struct sd_dict_paths paths;
	struct sd_dict_path *path;
	struct sd_dict *dict;

	sd_dict_paths_lookup(&paths);
	if (!paths.paths) {
		printf("No dictionaries found\n");
		return 1;
	}

	printf("Found %u dictionaries\n", paths.dict_cnt);

	for (path = paths.paths; path; path = path->next)
		printf(" dict '%s'\n", path->name);
	printf("\n");

	printf("Opening dict '%s'\n", paths.paths->name);

	dict = sd_open_dict(paths.paths->dir, paths.paths->name);
	if (!dict) {
		printf("Failed to load dict!\n");
		return 1;
	}

	sd_dict_paths_free(&paths);

	printf("Dict loaded word count=%u\n", dict->word_count);

	struct sd_lookup_res res;

	if (!argv[1])
		return 0;

	printf("Lookup '%s' ... ", argv[1]);

	int ret = sd_lookup_dict(dict, argv[1], &res);

	if (!ret) {
		printf("none\n");
		return 0;
	} else {
		printf("%i\n", sd_lookup_res_cnt(&res));
	}

	printf("Result %u - %u\n", res.min, res.max);
	printf("%s - %s\n", sd_idx_to_word(dict, res.min), sd_idx_to_word(dict, res.max));

	for (unsigned int i = res.min; i <= res.max; i++) {
		printf("%s\n", sd_idx_to_word(dict, i));
	}

	struct sd_entry *entry = sd_get_entry(dict, res.min);

	if (entry)
		printf("%s\n", entry->data);

	sd_free_entry(entry);
	sd_close_dict(dict);

	return 0;
}
