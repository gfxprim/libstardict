sd_lookup(3)

# NAME
sd_lookup, sd_lookup_res_cnt, sd_idx_to_word - Looks up words by a prefix

# LIBRARY
Libstardict (_-lstardict_)

# SYNOPSIS
*\#include <libstardict.h>*

*unsigned int sd_lookup(struct sd_dict *_\*self_*, const char *_\*prefix_*, struct sd_lookup_res *_\*res_*);*

*unsigned int sd_lookup_res_cnt(struct sd_lookup_res *_\*res_*);*

*const char \*sd_idx_to_word(struct sd_dict *_\*dict_*, unsigned int *_idx_*);*

# DESCRIPTION

*sd_lookup()*
	The stardict dictionary format stores all keys in an index i.e. sorted
	array of keywords. The lookup function returns a range of indexes for
	keywords that starts with a given _prefix_. Each index can be
	translated into a keyword by *sd_idx_to_word*() and the corresponding
	entry (translation) can be retrieved by calling *sd_get_entry*(3).

```
struct sd_lookup_res {
	unsigned int min;
	unsigned int max;
};
```

*sd_idx_to_word()*
	The *sd_idx_to_word*() function can translate an index into a keyword
	(an UTF8 string).

# RETURN VALUE

The *sd_lookup*() returns number of words in the index matching the
prefix, the range for the index is stored into the _res_. If zero is returned
the range in _res_ is not valid.

The *sd_lookup_res_cnt*() returns the number of words in the _res_ range.

The *std_idx_to_word*() returns UTF8 string for a given index.

# SEE ALSO
*sd_get_entry*(3), *sd_open_dict*(3)
