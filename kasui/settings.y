%{
#include "settings.h"

#include "guava2d/rgb.h"
#include "guava2d/file.h"

#include "theme.h"
#include "panic.h"

#include <cstdio>
#include <cstring>
#include <cassert>

#include <ctype.h>

int yylex();
void yyerror(const char *str);

enum field_type {
	FT_MAIN_COLOR,
	FT_ALT_COLOR,
	FT_BG_GRADIENT,
	FT_TEXT_GRADIENT,
};

struct field {
	field(field_type type)
	: type(type) { }

	field_type type;
};

struct gradient_field : public field {
	gradient_field(field_type type, gradient *value)
	: field(type), value(value)
	{ }

	gradient *value;
};

struct rgb_field : public field {
	rgb_field(field_type type, g2d::rgb *value)
	: field(type), value(value)
	{ }

	g2d::rgb *value;
};

struct field_list {
	field_list(field *value, field_list *next)
	: value(value), next(next)
	{ }

	field *value;
	field_list *next;
};

static void
set_color_scheme(const char *name, const color_scheme& cs)
{
	struct name_to_theme {
		const char *name;
		int index;
	} name_to_themes[NUM_THEMES] = {
		{ "clouds", THEME_CLOUDS },
		{ "falling-leaves", THEME_FALLING_LEAVES },
		{ "flowers", THEME_FLOWERS },
	};

    for (const auto& p : name_to_themes) {
		if (!strcmp(p.name, name)) {
			cur_settings.color_schemes[p.index] = cs;
			break;
		}
	}
}

color_scheme
parse_color_scheme(field_list *fields)
{
	color_scheme cs;

	for (field_list *p = fields; p; p = p->next) {
		field *f = p->value;

		switch (f->type) {
			case FT_MAIN_COLOR:
				cs.main_color = *static_cast<rgb_field *>(f)->value;
				break;

			case FT_ALT_COLOR:
				cs.alternate_color = *static_cast<rgb_field *>(f)->value;
				break;

			case FT_BG_GRADIENT:
				cs.background_gradient = *static_cast<gradient_field *>(f)->value;
				break;

			case FT_TEXT_GRADIENT:
				cs.text_gradient = *static_cast<gradient_field *>(f)->value;
				break;

			default:
				assert(0);
		}
	}

	return cs;
}

%}

%union {
	int int_val;
	char *str_val;
	g2d::rgb *rgb_val;
	gradient *gradient_val;
	field *field_val;
	field_list *field_list_val;
}

%token GAME ANIMATION COLOR_SCHEMES
%token LEVEL_SECS TICS_TO_DROP BAKUDAN_PERIOD HINT_PERIOD
%token DROP_TICS SOLVE_TICS SWAP_TICS MOVE_TICS
%token MAIN_COLOR ALT_COLOR BG_GRADIENT TEXT_GRADIENT

%token <int_val> INTEGER
%token <str_val> IDENTIFIER

%type <rgb_val> rgb
%type <gradient_val> gradient
%type <field_val> color_scheme_field bg_gradient_field text_gradient_field main_color_field alt_color_field
%type <field_list_val> color_scheme_fields

%%

input
: settings_list
| /* nothing */
;

settings_list
: settings_list settings
| settings
;

settings
: color_scheme_settings
| game_settings
| animation_settings
;

animation_settings
: ANIMATION '{' animation_setting_list '}'
;

animation_setting_list
: animation_setting_list animation_setting
| animation_setting
;

animation_setting
: DROP_TICS INTEGER				{ cur_settings.animation.drop_tics = $2; }
| SOLVE_TICS INTEGER				{ cur_settings.animation.solve_tics = $2; }
| SWAP_TICS INTEGER				{ cur_settings.animation.swap_tics = $2; }
| MOVE_TICS INTEGER				{ cur_settings.animation.move_tics = $2; }
;

game_settings
: GAME '{' game_setting_list '}'
;

game_setting_list
: game_setting_list game_setting
| game_setting
;

game_setting
: LEVEL_SECS INTEGER				{ cur_settings.game.level_secs = $2; }
| TICS_TO_DROP INTEGER				{ cur_settings.game.tics_to_drop = $2; }
| BAKUDAN_PERIOD INTEGER			{ cur_settings.game.bakudan_period = $2; }
| HINT_PERIOD INTEGER				{ cur_settings.game.hint_period = $2; }
;

color_scheme_settings
: COLOR_SCHEMES '{' color_scheme_list '}'

color_scheme_list
: color_scheme_list color_scheme
| color_scheme
;

color_scheme
: IDENTIFIER '{' color_scheme_fields '}'	{ set_color_scheme($1, parse_color_scheme($3)); }
;

color_scheme_fields
: color_scheme_fields color_scheme_field	{ $$ = new field_list($2, $1); }
| color_scheme_field				{ $$ = new field_list($1, nullptr); }
;

color_scheme_field
: bg_gradient_field
| text_gradient_field
| main_color_field
| alt_color_field
;

bg_gradient_field
: BG_GRADIENT gradient				{ $$ = new gradient_field(FT_BG_GRADIENT, $2); }
;

text_gradient_field
: TEXT_GRADIENT gradient			{ $$ = new gradient_field(FT_TEXT_GRADIENT, $2); }
;

main_color_field
: MAIN_COLOR rgb				{ $$ = new rgb_field(FT_MAIN_COLOR, $2); }
;

alt_color_field
: ALT_COLOR rgb					{ $$ = new rgb_field(FT_ALT_COLOR, $2); }
;

gradient
: rgb rgb 					{ $$ = new gradient{*$1, *$2}; }
;

rgb
: '<' INTEGER INTEGER INTEGER '>'		{ $$ = new g2d::rgb($2 / 255.0f, $3 / 255.0f, $4 / 255.0f); }
;

%%

static const char *SETTINGS_FILE_PATH = "data/settings";

g2d::file_input_stream *settings_file;

void
yyerror(const char *str)
{
	panic("error parsing settings file: %s", str);
}

void
load_settings()
{
	settings_file = new g2d::file_input_stream(SETTINGS_FILE_PATH);
	yyparse();
	delete settings_file;
}
