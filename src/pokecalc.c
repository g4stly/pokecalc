#include <jsmn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define PKMN_TOKEN_COUNT 15

void
log_msg(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	/*if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}*/
}

struct Pokemon {
	char name[12];
	int stats[6];
};

int load_Pokemon(struct Pokemon *p, const char *json_string);


int
load_Pokemon(struct Pokemon *p, const char *json_string) {
	/*
	 * parse json into tokens array
	 */

	int res;
	int token_count;
	jsmn_parser parser;	

	log_msg("using string: %s\n", json_string);
	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, json_string, strlen(json_string), NULL, 0);
	log_msg("token_count = %i\n", token_count);

	jsmntok_t tokens[token_count];
	jsmn_init(&parser); /* reset parser here in order to re use it */
	res = jsmn_parse(&parser, json_string, strlen(json_string), tokens, token_count);
	log_msg("res = %i\n", res);

	if (res < 0) return res;

	/*
	 * parse token array into pokemon struct
	 */

	int size;
	int target = -1;
	char *token;
	for (int i = 1; i < token_count; i++) {

		/* skip object and array tokens */
		log_msg("loop iteration: %i\n", i);
		log_msg("token type: %i\n", tokens[i].type);
		if (tokens[i].type != JSMN_STRING && tokens[i].type != JSMN_PRIMITIVE) continue;

		/* calculate size, allocate size bytes for
		 * string representation of token,
		 * be sure to append null byte */
		size = tokens[i].end - tokens[i].start;
		log_msg("size of current token: %i\n", size);
		token = malloc(sizeof(char)*size+1);
		snprintf(token, size+1, json_string + tokens[i].start);
		token[size+1] = '\0';
		log_msg("token = %s\n", token);

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

		/* if target isn't set, this token should be a key. */
		char* stats[] = {
			"hp",
			"attack",
			"defense",
			"spattack",
			"spdefense",
			"speed"
		};

		/* if key is name, set target to -420 so we don't try
		 * to index our p->stats array with it,
		 * otherwise, loop over stats array above
		 * and assign the target to the index of that array,
		 * corresponding to the stats array in the pokemon struct*/
		if (strcmp(token, "name") == 0) {
			target = -420;
			continue;
		}

		for (int j = 0; j < 6; j++) {
			if (strcmp(token, stats[j]) == 0) {
				target = j;
				break;
			}
		}
		free(token);
	}
	return 0;
}

int 
main(int argc, char **argv) {
	
	struct Pokemon p;
	char *string = argv[1] ? argv[1] : "{\"name\":\"bulba\"}";
	int res = load_Pokemon(&p, string);
	if (res < 0) {
		switch(res) {
		case JSMN_ERROR_INVAL:
			printf("Invalid JSON.\n");
			break;
		case JSMN_ERROR_NOMEM:
			printf("Json string is too large.\n");
			break;
		case JSMN_ERROR_PART:
			printf("Expecting more json data.\n");
		default:
			printf("Unkown error.");
		}
		return -1;
	}

	printf("name: %s\n", p.name);

	return 0;
}
