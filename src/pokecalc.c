#include <jsmn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define PKMN_TOKEN_COUNT 15
#define ERROR_NO_JSON_DATA -4

void log_msg(const char *fmt, ...) 
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	/*if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}*/
}

void log_error(const char *fmt, ...)
{
	fprintf(stderr, "FATAL: ");
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(-1);
}

void parse_or_die(int res)
{
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

int load_Pokemon(struct Pokemon *p, const char *json_string);
void print_Pokemon(struct Pokemon *p);


int load_Pokemon(struct Pokemon *p, const char *json_string) 
{
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
	struct Pokemon p;
	memset(&p, 0, sizeof(struct Pokemon));

	/* parse or die <your_favorite_insult> */
	parse_or_die(load_Pokemon(&p, argv[1]));

	print_Pokemon(&p);

	return 0;
}
