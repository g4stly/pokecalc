#include <jsmn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define PKMN_TOKEN_COUNT 15
#define ERROR_NO_JSON_DATA -4

struct Pokemon {
	char name[12];
	int stats[6];
};

static char* stats_strings[] = {
	"hp",
	"attack",
	"defense",
	"spattack",
	"spdefense",
	"speed"
};

static char * jsmn_types_strings[] = {
	"jsmn_undefined",
	"jsmn_object",
	"jsmn_array",
	"jsmn_string",
	"jsmn_primitive"
};

static int verbose_flag = 0;
static int from_file_flag = 0;
static char *json_string = NULL;

/* utility */
void log_msg(const char *fmt, ...);
void log_error(const char *fmt, ...);
void parse_or_die(int res);
int parse_options(int argc, char **argv);
char *load_from_file(const char *filename);

/* actually pokemon related */
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

int load_Pokemon(struct Pokemon *p, const char *json_string) 
{
	log_msg("now parsing json\n");

	 /* parse json into tokens array */
	int res;
	int token_count;
	jsmn_parser parser;	

	if (!json_string) return ERROR_NO_JSON_DATA;
	log_msg("using string: %s\n", json_string);

	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, json_string, strlen(json_string), NULL, 0);
	log_msg("token count = %i\n", token_count);

	jsmntok_t tokens[token_count]; // var array, consider making it compile time constant
	jsmn_init(&parser); // reset parser here in order to re use it
	res = jsmn_parse(&parser, json_string, strlen(json_string), tokens, token_count);

	if (res < 0) return res;

	 /* parse token array into pokemon struct */
	char *token;
	int size, target = -1;
	for (int i = 1; i < token_count; i++) {
		log_msg("loop iteration: %i\n", i);

		/* only process strings and primitives */
		log_msg("token type: %s\n", jsmn_types_strings[tokens[i].type]);
		if (tokens[i].type != JSMN_STRING && tokens[i].type != JSMN_PRIMITIVE) continue;

		/* calculate size, allocate size bytes for
		 * string representation of token,
		 * be sure to append null byte */
		size = tokens[i].end - tokens[i].start;
		log_msg("size of current token: %i\n", size);
		token = malloc(sizeof(char)*size+1);
		snprintf(token, size+1, json_string + tokens[i].start);
		token[size+1] = '\0';
		log_msg("token = %s\n\n", token);

		/* if target > 0, then the last token was a key, this is a value */
		if (target >= 0) {
			if (tokens[i].type != JSMN_PRIMITIVE) return JSMN_ERROR_INVAL;
			p->stats[target] = atoi(token);
			target = -1;
			continue;
		/* if target == -420, then the last token was the name key
		 * TODO: find a better way to do this */
		} else if (target == -420) {
			memcpy(p->name, token, size+1);
			target = -1;
			continue;
		}

		/* if target isn't set, this token should be a key.
		 * if key is name, set target to -420 so we don't try
		 * to index our p->stats array with it,
		 * otherwise, loop over stats_strings array */
		if (strcmp(token, "name") == 0) {
			target = -420;
			continue;
		}

		for (int j = 0; j < 6; j++) {
			if (strcmp(token, stats_strings[j]) == 0) {
				target = j;
				break;
			}
		}
		free(token);
	}
	return 0;
}

void print_Pokemon(struct Pokemon *p)
{
	/* TODO: eventually, don't make repeated calls to fprintf */
	fprintf(stdout, "== %s ==\n@ level 50\n", p->name);
	for (int i = 0; i < 6; i++) 
		fprintf(stdout, "%s: %i\n", stats_strings[i], p->stats[i]);
}

int main(int argc, char **argv)
{
	if (parse_options(argc, argv) < 0) return -1;

	struct Pokemon p;
	memset(&p, 0, sizeof(struct Pokemon));

	/* parse or die <your_favorite_insult> */
	parse_or_die(load_Pokemon(&p, json_string));

	print_Pokemon(&p);

	return 0;
}
