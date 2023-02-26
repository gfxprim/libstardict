sd_open_dict(3)

# NAME
sd_open_dict, sd_close_dict - Opens a stardict dictionary

# LIBRARY
Libstardict (_-lstardict_)

# SYNOPSIS
*\#include <libstardict.h>*

*struct sd_dict \*sd_open_dict(const char *_\*path_*, const char *_\*name_*);*

*void sd_close_dict(struct sd_dict *_\*self_*);*
# DESCRIPTION

*sd_open_dict()*

	The *sd_open_dict*() opens a stadict dictionary and returns a handle
	for word lookups.

	The _path_ is a path to a directory with the dictionary files and
	_name_ is the dictionary filename without a suffix. The actual
	dictionary consists of several files with the same name and different
	suffixes. The list of the dictionaries installed on the system can be
	looked up by *sd_lookup_dict_paths*(3).

```
struct sd_dict {
	char entry_fmt;
	unsigned int word_count;
	char book_name[SD_DICT_BOOKNAME_MAX];
	...
}
```

	The _entry_fmt_ if set describes the format of the data returned by a
	word lookup. Stardict format supports dictionaries with mixed format
	entries, but these seems to be really rare, so you can expect the
	entry_fmt to be set most of the time.

	The formats are:

	- *SD_ENTRY_UTF8_TEXT* UTF8 plaintext

	- *SD_ENTRY_PANGO_MARKUP* Pango markup, UTF8 HTML-like markup however whitespaces are interpreted literaly

	- *SD_ENTRY_HTML* UTF8 HTML

	- *SD_ENTRY_XDXF* HTML-like dictionary markup see http://xdxf.sourceforge.net

	- *SD_ENTRY_CHINESE_JAPANESE* Chinese or Japanse UTF8 text

	The _word_count_ is a number of words in the dictionary.

	The _book_name_ is an UTF8 string with the dictionary name.

*sd_close_dict()*
	Closes a dictionary and frees the memory. Passing _NULL_ to the call is a no-op.

# RETURN VALUE

The *sd_open_dict*() returns a handle to a dictionary or NULL in a case of a failure.

# EXAMPLES

```
#include <stdio.h>
#include <libstardict.h>

int main(int argc, const char *argv[])
{
	struct sd_dict *dict;

	if (argc != 3) {
		printf("Usage: %s dict_path dict_filename\\n", argv[0]);
		return 1;
	}

	dict = sd_open_dict(argv[1], argv[2]);

	if (!dict) {
		printf("Failed to open dictionary\\n");
		return 1;
	}

	printf("Loaded dictionary '%s' with %u words in index\\n",
	       dict->book_name, dict->word_count);

	sd_close_dict(dict);

	return 0;
}
```

# SEE ALSO
	*sd_lookup_dict_paths*(3), *sd_lookup*(3), *sd_get_entry*(3)
