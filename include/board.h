/*
 * Copyright (c) 2018 Maxime Schmitt <max.schmitt@unistra.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOARD_H__
#define BOARD_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct gol_board_bounds {
  intmax_t upperX, upperY, lowerX, lowerY;
};

enum gol_rules {
  lifeRule = 0,
  highLifeRule,
  unknownRule,
};

extern char *gol_rule_string[unknownRule];

struct gol_board;

__attribute__((pure)) bool read_gol_board(intmax_t posX, intmax_t posY,
                                          const struct gol_board *b);

void write_gol_board(intmax_t posX, intmax_t posY, bool val,
                     struct gol_board *b);

struct gol_board *new_board(void);

__attribute__((pure)) struct gol_board_bounds
get_game_bounds(const struct gol_board *b);

void clean_board(struct gol_board *b);

void free_board(struct gol_board *b);

void set_offset(intmax_t offsetX, intmax_t offsetY, struct gol_board *b);

void set_game_rules(enum gol_rules rule, struct gol_board *b);

__attribute__((pure)) enum gol_rules get_game_rules(const struct gol_board *b);

void gol_copy_board(const struct gol_board *to_copy, struct gol_board *copy);

void gol_swap_board(struct gol_board *swap1, struct gol_board *swap2);

__attribute__((pure)) bool gol_same_board(const struct gol_board *b1,
                                          const struct gol_board *b2);

void get_offset(const struct gol_board *board, intmax_t *offsetX,
                intmax_t *offsetY);

void dump_board_ASCII(FILE *outStream, struct gol_board *b);

struct gol_game {
  struct gol_board *board;
  char *patternName;
  char *authorName;
  char **comments;
  size_t num_comments;
};

void clone_metadata(const struct gol_game *b1, struct gol_game *b2);

void free_game(struct gol_game *game);

void dump_ASCII(FILE *outStream, struct gol_game *b);

void add_comment(const char *comment, struct gol_game *b);

void set_author(const char *authorName, struct gol_game *b);

void set_pattern_name(const char *pName, struct gol_game *b);

#endif
