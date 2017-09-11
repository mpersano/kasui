%{
#include "settings.h"

// some forward declarations for stuff in settings_parser.h
struct field;
struct field_list;
#include "settings_parser.h"

#include "guava2d/file.h"

extern g2d::file_input_stream *settings_file;

#define YY_NO_UNPUT 1

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) { result = settings_file->read(buf, max_size); }

extern void yyerror(const char *str);

%}
%%

[0-9]+			{ yylval.int_val = atol(yytext); return INTEGER; }
game			{ return GAME; }
animation		{ return ANIMATION; }
tics-to-drop		{ return TICS_TO_DROP; }
level-secs		{ return LEVEL_SECS; }
bakudan-period		{ return BAKUDAN_PERIOD; }
hint-period		{ return HINT_PERIOD; }
drop-tics		{ return DROP_TICS; }
solve-tics		{ return SOLVE_TICS; }
swap-tics		{ return SWAP_TICS; }
move-tics		{ return MOVE_TICS; }
color-schemes		{ return COLOR_SCHEMES; }
main-color		{ return MAIN_COLOR; }
alt-color		{ return ALT_COLOR; }
bg-gradient		{ return BG_GRADIENT; }
text-gradient		{ return TEXT_GRADIENT; }
[a-zA-Z_][a-zA-Z0-9_-]*	{
				yylval.str_val = new char[strlen(yytext) + 1];
				strcpy(yylval.str_val, yytext);
				return IDENTIFIER;
			}
"{"|"}"|"<"|">"|"="	{ return *yytext; }
"#"[^\n]*		{ }
[ \t\r\n]+		{ /* eat up whitespace */ }
.			{ yyerror("invalid character"); }
			
%%
int
yywrap(void)
{
	return 1;
}
