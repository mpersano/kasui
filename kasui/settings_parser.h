/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     GAME = 258,
     ANIMATION = 259,
     COLOR_SCHEMES = 260,
     LEVEL_SECS = 261,
     TICS_TO_DROP = 262,
     BAKUDAN_PERIOD = 263,
     HINT_PERIOD = 264,
     DROP_TICS = 265,
     SOLVE_TICS = 266,
     SWAP_TICS = 267,
     MOVE_TICS = 268,
     MAIN_COLOR = 269,
     ALT_COLOR = 270,
     BG_GRADIENT = 271,
     TEXT_GRADIENT = 272,
     INTEGER = 273,
     IDENTIFIER = 274
   };
#endif
/* Tokens.  */
#define GAME 258
#define ANIMATION 259
#define COLOR_SCHEMES 260
#define LEVEL_SECS 261
#define TICS_TO_DROP 262
#define BAKUDAN_PERIOD 263
#define HINT_PERIOD 264
#define DROP_TICS 265
#define SOLVE_TICS 266
#define SWAP_TICS 267
#define MOVE_TICS 268
#define MAIN_COLOR 269
#define ALT_COLOR 270
#define BG_GRADIENT 271
#define TEXT_GRADIENT 272
#define INTEGER 273
#define IDENTIFIER 274




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 112 "settings.y"

	int int_val;
	char *str_val;
	g2d::rgb *rgb_val;
	gradient *gradient_val;
	field *field_val;
	field_list *field_list_val;



/* Line 2068 of yacc.c  */
#line 99 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


