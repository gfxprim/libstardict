sd_get_entry(3)

# NAME
sd_get_entry, sd_strip_entry, sd_free_entry - Retrives a dictionary entry for an index

# LIBRARY
Libstardict (_-lstardict_)

# SYNOPSIS
*\#include <libstardict.h>*

*struct sd_entry \*sd_get_entry(struct sd_dict *_\*dict_*, unsigned int *_idx_*);*

*int sd_strip_entry(struct sd_entry *_\*entry_*);*

*void sd_free_entry(struct sd_entry *_\*entry_*);*

# DESCRIPTION

*sd_get_entry()*
	The *sd_get_entry*() returns a dictionary entry (a translation) for a
	given index (a key).

```
struct sd_entry {
	char fmt;
	char data[];
};
```

	See sd_open_dict(3) for the description of the _fmt_. If the data holds
	a textual information the string is null terminated.

*sd_strip_entry()*
	The *sd_strip_entry*() strips any text formatting (e.g. HTML tags) from
	textual entries.

*sd_free_entry()*
	The *sd_free_entry*() frees the data previously returned by
	*sd_get_entry*(). The call is no-op for _NULL_ entry.

# RETURN VALUE

Upon succesful completion *sd_get_entry*() returns newly allocated _struct
sd_entry_. If index is out of dictionary index or if allocation failed _NULL_
is returned.

The *sd_strip_entry*() returns non-zero if the entry was in or was converted to
UTF8 plaintext.

# EXAMPLES

```
#include <stdio.h>
#include <libstardict.h>

static void print_entry(struct sd_dict *dict, unsigned int idx)
{
	struct sd_entry *entry;

	entry = sd_get_entry(dict, idx);
	if (!entry) {
		printf("Failed to read entry idx=%u\\n", idx);
		return;
	}

	printf("Key: %s\\n", sd_idx_to_word(dict, idx));
	if (sd_strip_entry(entry))
		printf("%s\\n", entry->data);

	sd_free_entry(entry);
}
```

# SEE ALSO
*sd_open_dict*(3), *sd_lookup*(3)
