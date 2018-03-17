#include <jsmn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

// these are used to help define natures 
#define ATTACK 1
#define DEFENSE 2
#define SPATTACK 3
#define SPDEFENSE 4
#define SPEED 5

#define PKMN_TOKEN_COUNT 15
#define PKMN_NATURE_COUNT 25
#define ERROR_NO_JSON_DATA -4
#define COMMA_ARG_LIMIT 3


struct Nature {
	const char *name;
	int beneficial; // will correspond with indexs in "stats" arrays
	int detrimental; // ^^^^^^^^^^^^^^
};

struct Pokemon {
	char name[12];
	int level;
	int stats[6];
	int IV[6];
	int EV[6];
	struct Nature *nature;
};

static char* stats_strings[] = {
	"hp",
	"attack",
	"defense",
	"spattack",
	"spdefense",
	"speed"
};

static int verbose_flag = 0;
static int from_file_flag = 0;
static char *json_string = NULL;
static struct Pokemon pokemon;
static struct Nature *natures;

/* utility */
void log_msg(const char *fmt, ...);
void log_error(const char *fmt, ...);
void parse_or_die(int res);
int parse_options(int argc, char **argv);
void parse_stat_option(const char *option);
char *load_token(jsmntok_t token, const char *json_string);
char *load_comma_arg(const char *arg, int size);
char *load_from_file(const char *filename);
int calc_stat(int base);

/* actually pokemon related */
void create_Nature(struct Nature *n, const char *name, int up, int bad);
double nature_multiplier(int stat);
int set_Pokemon(char *key, char *value);
int load_Pokemon(const char *json_string);
void grow_Pokemon(void);
void print_Pokemon(void);


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
	while ((c = getopt(argc, argv, "vp:f:l:n:s:")) != -1) {
		switch (c) {
		// options with regard to where we get our json
		case 'p':
			json_string = optarg;
			break;
		case 'f':
			from_file_flag = 1;
			json_string = load_from_file(optarg);
			break;
		// options with regard to our stats
		case 'l':
			//set_Pokemon("level", optarg);
			break;
		case 'n':
			//set_Pokemon("nature", optarg);
			break;
		case 's':
			parse_stat_option(optarg);
			break;
		// meta options
		case 'v':
			verbose_flag = 1;
			break;
		case '?':
			//if (optopt == 'p') log_error("Option -%c requires an argument.\n", optopt);
			//if (optopt == 'f') log_error("Option -%c requires an argument.\n", optopt);
			return -1;
			break;
		}
	}
	return 0;
} 

void parse_stat_option(const char *option)
{
	log_msg("parsing new stat\n");
	// find the position of the first two commas
	int positions[COMMA_ARG_LIMIT];
	for (int i = 0; i < COMMA_ARG_LIMIT; i++) positions[i] = -1;
	int i = 0, counted = 0;
	int size = strlen(option);
	
	while (counted < COMMA_ARG_LIMIT && i < size) {	// it werks, trust me
		if (option[i++] == ',') positions[counted++] = i;
	}
	if (counted < 1) { // ev's are required, therefore so is atleast one comma
		log_msg("invalid stat; missing a comma, ignoring");
		return;
	}

	char *stat_name = NULL;
	char *ev_amount = NULL;
	char *iv_amount = NULL;

	// read the first word into stat_name
	size = positions[0] - 1;  // length from start to first comma
	stat_name = load_comma_arg(option, size);
	log_msg("stat name: %s\n", stat_name);

	// read the ev count into ev_amount
	int end_of_arg = positions[1];
	if (end_of_arg < 0) end_of_arg = strlen(option)+1;
	size = (end_of_arg - positions[0]) - 1;
	ev_amount = load_comma_arg(option+positions[0], size);
	log_msg("ev count: %s\n", ev_amount);

	// if we have a third arg, read the iv count into iv_amount
	if (counted > 1) {
		log_msg("detected third comma arg");
		size = strlen(option) - positions[1];
		iv_amount = load_comma_arg(option+positions[1], size);
		log_msg("iv count: %s\n", iv_amount);
	}

	// loop through stat strings, look for match
	int stat = -1;
	for (int i = 0; i < 6; i++) {
		log_msg("checking %s against %s\n", stats_strings[i], stat_name);
		if (!strcmp(stats_strings[i], stat_name)) {
			stat = i;
			break;
		}
	}
	if (stat < 0) {
		log_msg("invalid stat, ignoring\n");
		return;
	}

	// the line below is awesome
	pokemon.EV[stat] = atoi(ev_amount);
	if (iv_amount) pokemon.IV[stat] = atoi(iv_amount);


	// clean up after ourselves
	free(stat_name);
	if (ev_amount) free(ev_amount);
	if (iv_amount) free(iv_amount);
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

char *load_comma_arg(const char *arg, int size)
{
	char *comma_arg = NULL;
	log_msg("alloc'ing %i bytes\n", size);
	comma_arg = malloc(sizeof(char) * size);
	if (!comma_arg) log_error("malloc():");

	memcpy(comma_arg, arg, size);
	comma_arg[size] = '\0';
	return comma_arg;
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

int calc_stat(int stat)
{
	return (((pokemon.stats[stat] * 2) + (pokemon.EV[stat] / 4) + pokemon.IV[stat]) * pokemon.level) / 100;
}

void create_Nature(struct Nature *n, const char *name, int up, int down)
{
	n->name 	= name;
	n->beneficial 	= up;
	n->detrimental 	= down;
}

double nature_multiplier(int stat)
{
	if (pokemon.nature->beneficial == stat) return 1.1;
	if (pokemon.nature->detrimental == stat) return 0.9;
	return 1.0;
}

int set_Pokemon(char *key, char *value)
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
			pokemon.stats[i] = atoi(local_value);
			return 0;
		}
	}

	// for now, test against string literals
	if (!strcmp(local_key, "name")) {
		snprintf(pokemon.name, value_size, local_value);
		pokemon.name[11] = '\0'; // no matter what, preserve null byte here
		return 0;
	}

	if (!strcmp(local_key, "level")) {
		int potential_level = atoi(local_value);
		if (potential_level > 0 && potential_level < 101)
			pokemon.level = atoi(local_value);
		else log_msg("level %s out of bounds\n", local_value);
	}

	if (!strcmp(local_key, "nature")) for (int i = 0; i < PKMN_NATURE_COUNT; i++) {
		log_msg("comparing %s to %s\n", local_value, natures[i].name);
		if (strcmp(local_value, natures[i].name) == 0) {
			pokemon.nature = &natures[i];
			return 0;
		}
	}

	return -1;
}

int load_Pokemon(const char *json_string) 
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
		if (set_Pokemon(key, value) < 0) log_msg("Invalid key!\n");
	}
	return 0;
}

void grow_Pokemon(void)
{
	pokemon.stats[0] = calc_stat(0) + 10 + pokemon.level;
	for (int i = 1; i < 6; i++) {
		// nature_multiplier decideds based on i (and pokemon.nature) whether the multiplier is 1, .9, or 1.1
		pokemon.stats[i] = (calc_stat(i) + 5) * nature_multiplier(i);
	}
}

void print_Pokemon(void)
{
	/* TODO: eventually, don't make repeated calls to fprintf */
	fprintf(stdout, "== %s ==\nlevel: %i\nnature: %s\n", pokemon.name, pokemon.level, pokemon.nature->name);
	for (int i = 0; i < 6; i++) 
		fprintf(stdout, "%s: %i\n", stats_strings[i], pokemon.stats[i]);
}

int main(int argc, char **argv)
{
	struct Nature natures_memory[25];
	natures = natures_memory;

	// excuse this boiler plate
	// create natures 	   |name|	|inc'd stat|	|decreased stat|
	create_Nature(&natures[0], "hardy", 	0, 		0);
	create_Nature(&natures[1], "lonely", 	ATTACK, 	DEFENSE);
	create_Nature(&natures[2], "brave", 	ATTACK, 	SPEED);
	create_Nature(&natures[3], "adamant", 	ATTACK, 	SPATTACK);
	create_Nature(&natures[4], "naughty", 	ATTACK, 	SPDEFENSE);

	create_Nature(&natures[5], "bold", 	DEFENSE,	ATTACK);
	create_Nature(&natures[6], "docile", 	0, 		0);
	create_Nature(&natures[7], "relaxed", 	DEFENSE, 	SPEED);
	create_Nature(&natures[8], "impish", 	DEFENSE, 	SPATTACK);
	create_Nature(&natures[9], "lax", 	DEFENSE, 	SPDEFENSE);

	create_Nature(&natures[10], "timid", 	SPEED, 		ATTACK);
	create_Nature(&natures[11], "hasty", 	SPEED,		DEFENSE);
	create_Nature(&natures[12], "serious", 	0, 		0);
	create_Nature(&natures[13], "jolly", 	SPEED, 		SPATTACK);
	create_Nature(&natures[14], "naive", 	SPEED, 		DEFENSE);

	create_Nature(&natures[15], "modest", 	SPATTACK, 	ATTACK);
	create_Nature(&natures[16], "mild", 	SPATTACK, 	DEFENSE);
	create_Nature(&natures[17], "quiet", 	SPATTACK, 	SPEED);
	create_Nature(&natures[18], "bashful", 	0,		0);
	create_Nature(&natures[19], "rash", 	SPATTACK, 	SPDEFENSE);

	create_Nature(&natures[20], "calm", 	SPDEFENSE, 	ATTACK);
	create_Nature(&natures[21], "gentle", 	SPDEFENSE, 	DEFENSE);
	create_Nature(&natures[22], "sassy", 	SPDEFENSE, 	SPEED);
	create_Nature(&natures[23], "careful", 	SPDEFENSE, 	SPATTACK);
	create_Nature(&natures[24], "quirky", 	0,		0);

	// zero out our memory
	memset(&pokemon, 0, sizeof(struct Pokemon));

	// set default pokemon values
	pokemon.level = 100;
	pokemon.nature = &natures[12];
	for (int i = 0; i < 6; i++) pokemon.IV[i] = 31;

	// parse options
	if (parse_options(argc, argv) < 0) return -1;

	// parse json
	parse_or_die(load_Pokemon(json_string));

	// calculate stats
	grow_Pokemon();

	// print stats
	print_Pokemon();

	return 0;
}
