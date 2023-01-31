// SPDX-License-Identifier: LGPL-2.1-or-later
/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <stddef.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <dirent.h>

#include <zlib.h>

#include "libstardict.h"

__attribute__ ((format (printf, 1, 2)))
static char *sd_aprintf(const char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap)+1;
	va_end(ap);

	char *tmp = malloc(len);
	if (!tmp)
		return NULL;

	va_start(ap, fmt);
	vsprintf(tmp, fmt, ap);
	va_end(ap);

	return tmp;
}

__attribute__ ((format (printf, 1, 2)))
static void sd_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

#define GZ_MAGIC1 0x1f
#define GZ_MAGIC2 0x8b

#define GZ_METHOD_DEFLATE 0x08

#define GZ_FLAGS_CRC 0x02
#define GZ_FLAGS_EXTRA_FIELD 0x04
#define GZ_FLAGS_FNAME 0x08
#define GZ_FLAGS_COMMENT 0x10

#define DICT_DZ_MAGIC1 'R'
#define DICT_DZ_MAGIC2 'A'

#define HEADER_MAP_SIZE 4096

struct chunk_pos {
	uint16_t size;
	off_t offset;
};

#define CHUNK_CACHE_SIZE 3

struct cached_chunk {
	uint16_t idx;
	uint16_t hits;
	void *data;
};

struct dict_dz {
	int fd;
	uint16_t chunk_decomp_size;
	uint16_t chunk_cnt;
	struct cached_chunk chunk_cache[CHUNK_CACHE_SIZE];
	struct chunk_pos chunks[];
};

static void *dict_gz_inflate_chunk(struct dict_dz *self, uint16_t idx)
{
	z_stream stream = {};
	struct chunk_pos *chunk = &self->chunks[idx];
	void *buf, *res;

	if (inflateInit2(&stream, -15) != Z_OK) {
		sd_err("Failed to initialize infalte %s", stream.msg);
		return NULL;
	}

	buf = malloc(chunk->size);
	res = malloc(self->chunk_decomp_size);
	if (!buf || !res)
		goto err0;

	if (pread(self->fd, buf, chunk->size, chunk->offset) != chunk->size) {
		sd_err("Failed to read compressed data");
		goto err0;
	}

	stream.next_in = buf;
	stream.avail_in = chunk->size;
	stream.next_out = res;
	stream.avail_out = self->chunk_decomp_size;

	if (inflate(&stream, Z_PARTIAL_FLUSH) != Z_OK) {
		sd_err("Failed to inflate chunk %s", stream.msg);
		goto err0;
	}

	if (stream.avail_in) {
		sd_err("Input wasn't processed!");
		goto err0;
	}

	inflateEnd(&stream);
	free(buf);
	return res;
err0:
	inflateEnd(&stream);
	free(res);
	free(buf);
	return NULL;
}

static void *dict_gz_chunk_cache_lookup(struct dict_dz *self, uint16_t idx)
{
	uint16_t i;

	for (i = 0; i < CHUNK_CACHE_SIZE; i++) {
		if (!self->chunk_cache[i].data)
			continue;

		if (self->chunk_cache[i].idx == idx) {
			//TODO saturated increment
			self->chunk_cache[i].hits++;
			return self->chunk_cache[i].data;
		}
	}

	return NULL;
}

static void dict_gz_chunk_cache_insert(struct dict_dz *self, void *data, uint16_t idx)
{
	uint16_t i;
	uint16_t min_hits = self->chunk_cache[0].hits;
	uint16_t min_hits_i = 0;

	for (i = 0; i < CHUNK_CACHE_SIZE; i++) {
		if (!self->chunk_cache[i].data) {
			min_hits_i = i;
			goto insert;
		}

		if (self->chunk_cache[i].hits < min_hits) {
			min_hits = self->chunk_cache[i].hits;
			min_hits_i = 0;
		}
	}

insert:
	free(self->chunk_cache[min_hits_i].data);
	self->chunk_cache[min_hits_i].data = data;
	self->chunk_cache[min_hits_i].idx = idx;
	self->chunk_cache[min_hits_i].hits = 1;
}

static void dict_dz_chunk_cache_free(struct dict_dz *self)
{
	uint16_t i;

	for (i = 0; i < CHUNK_CACHE_SIZE; i++)
		free(self->chunk_cache[i].data);
}

static void dict_dz_chunk_cache_init(struct dict_dz *self)
{
	uint16_t i;

	for (i = 0; i < CHUNK_CACHE_SIZE; i++)
		self->chunk_cache[i].data = NULL;
}

/**
 * The file stars with gzip header that is:
 *
 * 0x1f, 0x8b          - gzip magic
 * 0x08                - compression method 0x08 == deflate
 * 0x..                - flags, set if optional fields are present
 * 0x.. 0x.. 0x.. 0x.. - modification time
 * 0x..                - compression flags
 * 0x..                - OS
 *
 * The flags has to have set the extra field bit 0x04 and the header is
 * followed by extra field with description of the compressed chunks
 *
 * 0x.. 0x..              - extra field size
 * 'R' 'A'                - dict.dz magic
 * 0x.. 0x..              - extra field sub size == extra field size - 2
 * 0x.. 0x..              - version must be set to 1
 * 0x.. 0x..              - chunk lenght
 * 0x.. 0x..              - number of chunks
 *
 * 0x.. 0x.. * chunks_cnt - chunk compressed sizes
 *
 * If flags have fname bit 0x08 original filename as null terminated string follows.
 *
 * If flags have comment bit 0x10 set a comment as a null terminated string follows.
 *
 * If flags have CRC bit 0x02 two bytes of CRC follows.
 *
 * [data chunk 1]
 * [data chunk 2]
 * ...
 */
static struct dict_dz *parse_dict_dz(const char *dict_path)
{
	int fd;

	fd = open(dict_path, O_RDONLY);
	if (!fd) {
		sd_err("Failed to open dict.dz file");
		return NULL;
	}

	uint8_t *header = mmap(NULL, HEADER_MAP_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
	if (header == MAP_FAILED) {
		sd_err("Failed to map dict.dz file");
		goto err0;
	}

	if (header[0] != GZ_MAGIC1 || header[1] != GZ_MAGIC2) {
		sd_err("File dict.dz has wrong gzip magic");
		goto err0;
	}

	if (header[2] != GZ_METHOD_DEFLATE) {
		sd_err("File dict.dz unsupported compression method");
		goto err0;
	}

	uint8_t flags = header[3];

	if (!(flags & GZ_FLAGS_EXTRA_FIELD)) {
		sd_err("File dict.dz does not have extra field");
		goto err0;
	}

	uint16_t extra_field_len = header[10] | header[11]<<8;

	if (header[12] != DICT_DZ_MAGIC1 || header[13] != DICT_DZ_MAGIC2) {
		sd_err("File dict.dz has wrong dz magic");
		goto err0;
	}

	uint16_t version = header[16] | header[17] << 8;
	uint16_t chunk_len = header[18] | header[19] << 8;
	uint16_t chunk_cnt = header[20] | header[21] << 8;

	if (version != 1)
		sd_err("Invalid version");

	if (chunk_cnt > (HEADER_MAP_SIZE-22) / 2) {
		sd_err("Too many chunks in dict.dz");
		goto err0;
	}

	struct dict_dz *res = malloc(sizeof(struct dict_dz) + sizeof(struct chunk_pos) * chunk_cnt);
	if (!res) {
		sd_err("Failed to allocate dict.dz description");
		goto err1;
	}

	res->fd = fd;
	res->chunk_cnt = chunk_cnt;
	res->chunk_decomp_size = chunk_len;

	dict_dz_chunk_cache_init(res);

	off_t offset = 10 + extra_field_len + 2;

	if (flags & GZ_FLAGS_FNAME) {
		while (offset < HEADER_MAP_SIZE && header[offset])
			offset++;
		offset++;
	}

	if (flags & GZ_FLAGS_COMMENT) {
		while (offset < HEADER_MAP_SIZE && header[offset])
			offset++;
		offset++;
	}

	if (flags & GZ_FLAGS_CRC)
		offset+=2;

	if (offset >= HEADER_MAP_SIZE) {
		sd_err("File dict.dz eader too long");
		goto err1;
	}

	uint16_t i;

	for (i = 0; i < chunk_cnt; i++) {
		res->chunks[i].size = header[22 + 2*i] | header[23 + 2*i] << 8;
		res->chunks[i].offset = offset;
		offset += res->chunks[i].size;
	}

	munmap(header, HEADER_MAP_SIZE);

	return res;
err1:
	free(res);
err0:
	munmap(header, HEADER_MAP_SIZE);
	close(fd);
	return NULL;
}

static void *dict_gz_get_chunk(struct dict_dz *self, uint16_t idx)
{
	void *chunk;

	chunk = dict_gz_chunk_cache_lookup(self, idx);
	if (chunk)
		return chunk;

	chunk = dict_gz_inflate_chunk(self, idx);
	if (!chunk)
		return NULL;

	dict_gz_chunk_cache_insert(self, chunk, idx);

	return chunk;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int dict_gz_read(struct dict_dz *self, char *buf, uint64_t offset, uint32_t size)
{
	uint32_t first_chunk = offset / self->chunk_decomp_size;
	uint32_t first_chunk_off = offset - first_chunk * self->chunk_decomp_size;
	uint32_t first_chunk_size = MIN(size, self->chunk_decomp_size - first_chunk_off);
	uint32_t last_chunk = (offset + size) / self->chunk_decomp_size;
	uint32_t i;

	if (first_chunk >= self->chunk_cnt || last_chunk >= self->chunk_cnt) {
		sd_err("[offset, offset + size] out of data");
		return 1;
	}

	/* Copy start of the data from the first chunk */
	void *chunk = dict_gz_get_chunk(self, first_chunk);
	if (!chunk)
		return 1;

	memcpy(buf, chunk + first_chunk_off, first_chunk_size);

	buf += first_chunk_size;
	size -= first_chunk_size;

	if (first_chunk == last_chunk)
		return 0;

	/* Copy middle chunks if read spans more than two chunks */
	for (i = first_chunk+1; i < last_chunk; i++) {
		chunk = dict_gz_get_chunk(self, i);
		if (!chunk)
			return 1;

		memcpy(buf, chunk, self->chunk_decomp_size);
		buf += self->chunk_decomp_size;
		size -= self->chunk_decomp_size;
	}

	/* Copy the rest from the last chunk */
	chunk = dict_gz_get_chunk(self, last_chunk);
	if (!chunk)
		return 1;

	memcpy(buf, chunk, size);

	return 0;
}

static void destroy_dict_dz(struct dict_dz *self)
{
	dict_dz_chunk_cache_free(self);
	close(self->fd);
	free(self);
}

static int parse_ifo(const char *path, const char *name, struct sd_dict *dict)
{
	char *ifo_path = sd_aprintf("%s/%s.ifo", path, name);
	FILE *ifo;
	char line[256];
	int ret = 1;

	if (!ifo_path)
		goto err0;

	ifo = fopen(ifo_path, "r");
	if (!ifo) {
		sd_err("Failed to open '%s': %s", ifo_path, strerror(errno));
		goto err0;
	}

	if (!fgets(line, sizeof(line), ifo))
		goto err1;

	if (strcmp(line, "StarDict's dict ifo file\n")) {
		sd_err("Invalid ifo file signature");
		goto err1;
	}

	while (fgets(line, sizeof(line), ifo)) {
		sscanf(line, "wordcount=%u\n", &dict->word_count);
		sscanf(line, "idxfilesize=%u\n", &dict->idx_filesize);
		sscanf(line, "sametypesequence=%c\n", &dict->entry_fmt);
	}

	if (!dict->word_count) {
		sd_err("Missing wordcount in ifo file");
		goto err1;
	}

	if (!dict->idx_filesize) {
		sd_err("Missing idxfilesize in ifo file");
		goto err1;
	}

	if (!dict->entry_fmt) {
		sd_err("Unsupported file wihout sametypesequence");
		goto err1;
	}

	ret = 0;
err1:
	fclose(ifo);
err0:
	free(ifo_path);
	return ret;
}

struct sd_dict *sd_open_dict(const char *path, const char *name)
{
	gzFile idx;
	char *idx_gz_path = sd_aprintf("%s/%s.idx.gz", path, name);
	char *idx_path = sd_aprintf("%s/%s.idx", path, name);
	char *dict_path = sd_aprintf("%s/%s.dict.dz", path, name);
	struct sd_dict *dict = malloc(sizeof(struct sd_dict));
	unsigned int i;
	void *p;

	if (!idx_gz_path || !idx_path || !dict_path || !dict) {
		sd_err("Failed to allocate dict");
		goto err0;
	}

	memset(dict, 0, sizeof(*dict));

	if (parse_ifo(path, name, dict))
		goto err0;

	dict->word_list = malloc(dict->word_count * sizeof(char *));
	dict->idx = malloc(dict->idx_filesize);

	if (!dict->word_list || !dict->idx) {
		sd_err("Failed to allocate idx or word_list");
		goto err0;
	}

	idx = gzopen(idx_gz_path, "rb");
	if (!idx)
		idx = gzopen(idx_path, "rb");

	if (!idx) {
		sd_err("Failed to open idx");
		goto err0;
	}

	if (gzread(idx, dict->idx, dict->idx_filesize) != (int)dict->idx_filesize) {
		sd_err("Failed to read index");
		goto err1;
	}

	gzclose(idx);

	p = dict->idx;

	for (i = 0; i < dict->word_count; i++) {
		dict->word_list[i] = p;
		p += strlen(p) + 1 + 8;
	}

	dict->dict_dz = parse_dict_dz(dict_path);

	free(dict_path);
	free(idx_path);
	free(idx_gz_path);

	return dict;
err1:
	free(dict->word_list);
	free(dict->idx);
err0:
	free(idx_path);
	free(idx_gz_path);
	free(dict_path);
	free(dict);
	return NULL;
}

static unsigned int binary_lookup(struct sd_dict *self, const char *prefix, int left)
{
	unsigned int l = 0;
	unsigned int r = self->word_count - 1;
	size_t prefix_len = strlen(prefix);

	for (;;) {
		unsigned int mid = (r + l) / 2;

		int ret = strncasecmp(prefix, self->word_list[mid], prefix_len);
		if (!ret) {
			if (left)
				r = mid;
			else
				l = mid;
		} else {
			if (ret < 0)
				r = mid;
			else
				l = mid;
		}

		if ((l - r) <= 1 || (r - l) <= 1) {
			int l_ret = strncasecmp(prefix, self->word_list[l], prefix_len);
			int r_ret = strncasecmp(prefix, self->word_list[r], prefix_len);

			if (l_ret && r_ret)
				return (unsigned int)-1;

			if (l_ret)
				return r;

			return l;
		}
	}
}

int sd_lookup_dict(struct sd_dict *self, const char *prefix, struct sd_lookup_res *res)
{
	res->min = binary_lookup(self, prefix, 1);

	if (res->min == (unsigned int)-1)
		return 0;

	res->max = binary_lookup(self, prefix, 0);

	return 1;
}

const char *sd_idx_to_word(struct sd_dict *self, unsigned int idx)
{
	if (idx >= self->word_count)
		return NULL;

	return self->word_list[idx];
}

struct sd_entry *sd_get_entry(struct sd_dict *self, unsigned int idx)
{
	if (idx >= self->word_count)
		return NULL;

	size_t off = strlen(self->word_list[idx]) + 1;

	uint8_t *bytes = (uint8_t*)self->word_list[idx] + off;

	uint32_t data_offset = bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
	uint32_t data_size = bytes[4] << 24 | bytes[5] << 16 | bytes[6] << 8 | bytes[7];

	struct sd_entry *res = malloc(sizeof(struct sd_entry) + data_size + 1);
	if (!res)
		return NULL;

	res->fmt = self->entry_fmt;

	if (dict_gz_read(self->dict_dz, res->data, data_offset, data_size)) {
		free(res);
		return NULL;
	}

	res->data[data_size] = 0;

	return res;
}

void sd_free_entry(struct sd_entry *entry)
{
	free(entry);
}

void sd_close_dict(struct sd_dict *dict)
{
	destroy_dict_dz(dict->dict_dz);
	free(dict->word_list);
	free(dict->idx);
	free(dict);
}

#define SD_DICT_DIR "/usr/share/stardict/dic"
#define SD_DICT_USER_DIR ".stardict/dic"

static struct sd_dict_path *dir_lookup(const char *dir_path, unsigned int *cnt)
{
	struct dirent *entry;
	DIR *dir = opendir(dir_path);
	struct sd_dict_path *ret = NULL;

	if (!dir)
		return NULL;

	while ((entry = readdir(dir))) {
		size_t len = strlen(entry->d_name);

		if (len < 4)
			continue;

		if (!strcmp(entry->d_name + len - 4, ".ifo")) {
			struct sd_dict_path *path = malloc(sizeof(struct sd_dict_path) + len - 3);

			if (!path)
				continue;

			memcpy(path->name, entry->d_name, len - 4);
			path->name[len - 4] = 0;

			path->dir = dir_path;

			path->next = ret;
			ret = path;

			(*cnt)++;
		}
	}

	closedir(dir);

	return ret;
}

void sd_dict_paths_lookup(struct sd_dict_paths *paths)
{
	struct sd_dict_path *tmp, *i;
	char *home = getenv("HOME");

	paths->dict_cnt = 0;

	if (home) {
		paths->home_sd_dir = sd_aprintf("%s/%s", home, SD_DICT_USER_DIR);

		if (paths->home_sd_dir)
			paths->paths = dir_lookup(paths->home_sd_dir, &paths->dict_cnt);

		if (!paths->paths) {
			free(paths->home_sd_dir);
			paths->home_sd_dir = NULL;
		}
	}

	tmp = dir_lookup(SD_DICT_DIR, &paths->dict_cnt);
	if (!tmp)
		return;

	if (paths->paths) {
		for (i = paths->paths; i->next; i = i->next);

		i->next = tmp;
	} else {
		paths->paths = tmp;
	}
}

void sd_dict_paths_free(struct sd_dict_paths *paths)
{
	struct sd_dict_path *i, *j;

	free(paths->home_sd_dir);

	i = paths->paths;

	while (i) {
		j = i;
		i = i->next;
		free(j);
	}
}
