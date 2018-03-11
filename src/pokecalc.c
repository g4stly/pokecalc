#include <jsmn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define PKMN_TOKEN_COUNT 15
#define ERROR_NO_JSON_DATA -4

enum pokemon_nature {
	ADAMANT,
	MODEST,
	CAREFUL
};

struct Pokemon {
	char name[12];
	int stats[6];
	enum pokemon_nature nature;
};

static char* stats_strings[] = {
	"hp",
	"attack",
	"defense",
	"spattack",
	"spdefense",
	"speed"
};

static char *nature_strings[] = {
	"adamant",
	"modest",
	"careful"
};

static int verbose_flag = 0;
static int from_file_flag = 0;
static char *json_string = NULL;

/* utility */
void log_msg(const char *fmt, ...);
void log_error(const char *fmt, ...);
void parse_or_die(int res);
int parse_options(int argc, char **argv);
char *load_token(jsmntok_t token, const char *json_string);
char *load_from_file(const char *filename);

/* actually pokemon related */
int set_Pokemon(struct Pokemon *p, char *key, char *value);
int load_Pokemon(struct Pokemon *p, const char *json_string);
void print_Pokemon(struct Pokemon *p);


/* function definitions */
void log_msg(const char *fmt, ...) 
{
	if (!verbose_flag) return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void log_error(const char *fmt, ...)
{
	fprintf(stderr, "FATAL: ");
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(-1);
}

void parse_or_die(int res)
{
	if (from_file_flag) free(json_string);
	if (res < 0) {
		switch(res) {
		case JSMN_ERROR_INVAL:
			log_error("Invalid JSON.\n");
			break;
		case JSMN_ERROR_NOMEM:
			log_error("JSON string is too large.\n");
			break;
		case JSMN_ERROR_PART:
			log_error("Expecting more JSON data.\n");
			break;
		case ERROR_NO_JSON_DATA:
			log_error("No JSON data provided.\n");
			break;
		default:
			log_error("Unkown error.");
		}
	}
}

int parse_options(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, "vp:f:")) != -1) {
		switch (c) {
		case 'p':
			json_string = optarg;
			break;
		case 'f':
			from_file_flag = 1;
			json_string = load_from_file(optarg);
			break;
		case 'v':
			verbose_flag = 1;
			break;
		case '?':
			if (optopt == 'p') log_error("Option -%c requires an argument.\n", optopt);
			if (optopt == 'f') log_error("Option -%c requires an argument.\n", optopt);
			return -1;
			break;
		}
	}
	return 0;
}

char *load_token(jsmntok_t token, const char *json_string)
{
	int size;
	char *token_string = NULL;
	size = (token.end - token.start) + 1;
	log_msg("size of token: %i\n", size);

	token_string = malloc(sizeof(char)*size+1);
	if (!token_string) log_error("malloc():");

	snprintf(token_string, size, json_string + token.start);
	token_string[size] = '\0';
	return token_string;
}

char *load_from_file(const char *filename)
{
	int res;
	long length;
	char *string = NULL;
	FILE *file = fopen(filename, "r");

	res = fseek(file, 0, SEEK_END);
	if (res < 0) log_error("fseek:");
	log_msg("first fseek returned %i\n", res);

	length = ftell(file);
	if (length < 0) log_error("ftell:");
	log_msg("file length: %li\n", length);

	res = fseek(file, 0, SEEK_SET);
	if (res < 0) log_error("fseek:");
	log_msg("second fseek returned %i\n", res);

	string = malloc(sizeof(char) * length + 1);
	if (!string) log_error("malloc():");

	size_t bytes_read = fread(string, sizeof(char), length, file);
	log_msg("read %i bytes from %s\n", bytes_read, filename);
	if (bytes_read != length) log_error("bytes read did not match length of file...");

	fclose(file);

	return string;
}

int set_Pokemon(struct Pokemon *p, char *key, char *value)
{
	int key_size = strlen(key)+1;
	int value_size = strlen(value)+1;

	char local_key[key_size+1];
	char local_value[value_size+1];

	snprintf(local_key, key_size, key);
	snprintf(local_value, value_size, value);

	local_key[key_size] = '\0';
	local_value[value_size] = '\0';

	free(key);
	free(value);

	log_msg("setting %s to %s\n", local_key, local_value);

	for (int i = 0; i < 6; i++) {
		if (!strcmp(stats_strings[i], local_key)) {
			p->stats[i] = atoi(local_value);
			return 0;
		}
	}

	if (!strcmp(local_key, "name")) {
		snprintf(p->name, value_size, local_value);
		p->name[11] = '\0'; // no matter what, preserve null byte here
		return 0;
	}

	if (!strcmp(local_key, "nature")) for (int i = 0; i < 3; i++) {
		if (strcmp(value, nature_strings[i]) == 0) {
			p->nature = i;
			return 0;
		}
	}

	return -1;
}

int load_Pokemon(struct Pokemon *p, const char *json_string) 
{
	log_msg("now parsing json\n");

	int res;
	int token_count;
	jsmn_parser parser;	

	if (!json_string) return ERROR_NO_JSON_DATA;
	log_msg("using string: %s\n", json_string);

	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, json_string, strlen(json_string), NULL, 0);
	log_msg("token count = %i\n", token_count);

	jsmntok_t tokens[token_count]; 
	jsmn_init(&parser); 
	res = jsmn_parse(&parser, json_string, strlen(json_string), tokens, token_count);

	if (res < 0) return res;

	char *key, *value;
	for (int i = 1; i < token_count; i += 2) {
		log_msg("\nloop iteration: %i\n", i);

		/* confirm key is string and value is string or primitive */
		if (tokens[i].type != JSMN_STRING) continue;
		if (tokens[i+1].type != JSMN_PRIMITIVE && tokens[i+1].type != JSMN_STRING) continue;

		/* load_token calls malloc */
		key = load_token(tokens[i], json_string);
		log_msg("key: %s\n", key);
		value = load_token(tokens[i+1], json_string);
		log_msg("value: %s\n", value);

		/* set_Pokemon calls free */
		if (set_Pokemon(p, key, value) < 0) log_msg("Invalid key!\n");
	}
	return 0;
}

void print_Pokemon(struct Pokemon *p)
{
	/* TODO: eventually, don't make repeated calls to fprintf */
	fprintf(stdout, "== %s ==\nlevel: 50\nnature: %s\n", p->name, nature_strings[p->nature]);
	for (int i = 0; i < 6; i++) 
		fprintf(stdout, "%s: %i\n", stats_strings[i], p->stats[i]);
}

int main(int argc, char **argv)
{
	if (parse_options(argc, argv) < 0) return -1;

	struct Pokemon p;
	memset(&p, 0, sizeof(struct Pokemon));

	parse_or_die(load_Pokemon(&p, json_string));

	print_Pokemon(&p);

	return 0;
}
