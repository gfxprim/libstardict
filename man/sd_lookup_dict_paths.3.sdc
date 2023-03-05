sd_lookup_dict_paths(3)

# NAME
sd_lookup_dict_paths, sd_free_dict_paths - Looks up for stardict dictionaries in standard paths

# LIBRARY
Libstardict (_-lstardict_)

# SYNOPSIS
*\#include <libstardict.h>*

*void sd_lookup_dict_paths(struct sd_dict_paths* _\*paths_*);*

*void sd_free_dict_paths(struct sd_dict_paths* _\*paths_*);*

# DESCRIPTION

*sd_lookup_dict_paths*()
	The *sd_lookup_dict_paths*() looks for stardict dictionaries in
	stardict system path e.g. _/usr/share/stardict/dic/_ and in the
	user home in _.stardict/dic/_ directory.

	A stardict dictionary consists of a few files with the same name and
	different suffixes, from these the .ifo file is parsed during the
	lookup and if this operation succeeds the dictionary is added to the
	_paths_ array of dictionaries.

```
struct sd_dict_paths {
	unsigned int dict_cnt;
	struct sd_dict_path **paths;
};
```

	The _paths_ pointer passed to the function is used to store the
	_dict_cnt_ a number of discovered dictionaries and the _paths_ pointer
	is use to store an array of _struct sd_dict_path_ dictionaries.

```
struct sd_dict_path {
	const char *dir;
	char book_name[SD_DICT_BOOKNAME_MAX];
	char fname[];
};
```

	The _struct sd_dict_path_ describes a single dictionary.

	The _dir_ is a directory the dictionary files are stored in and the
	_fname_ is a file name, without a suffix, for the dictionary files.

	The _book_name_ is a name parsed from the .ifo description and should
	be displayed in the user inteface to describe the dictionary.

*sd_free_dict_paths()*
	The *sd_free_dict_paths*() frees memory allocated by the *sd_lookup_dict_paths*().

# EXAMPLES

```
#include <stdio.h>
#include <libstardict.h>

int main(void)
{
	struct sd_dict_paths paths;

	sd_lookup_dict_paths(&paths);

	if (!paths.paths) {
		printf("No dictionaries found!\\n");
		return 1;
	}

	printf("Found %u dictionaries:\\n", paths.dict_cnt);

	for (i = 0; i < paths.dict_cnt; i++)
		printf(" %2i '%s' - %s.ifo\\n", i, paths.paths[i]->book_name, paths.paths[i]->fname);
	printf("\n");

	sd_free_dict_paths(&paths);

	return 0;
}
```

# SEE ALSO
*sd_open_dict*(3)
