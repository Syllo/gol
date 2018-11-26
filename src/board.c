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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"

#define uintbis(a) uint##a##_t
#define uint(a) uintbis(a)

#define uintdefbis(a, b) UINT##a##_C(b)
#define uintdef(a, b) uintdefbis(a, b)
#define intdefbis(a, b) INT##a##_C(b)
#define intdef(a, b) intdefbis(a, b)

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#ifndef BLOCKSIZE
#define BLOCKSIZE 32
#endif

#define block_type uint(BLOCKSIZE)

char *gol_rule_string[unknownRule] = {
    [lifeRule] = "B3/S23",
    [highLifeRule] = "B36/S23",
};

struct basic_block {
  block_type values[BLOCKSIZE];
};

__attribute__((pure)) static inline bool
is_empty_block(const struct basic_block *b) {
  bool continue_search = true;
  for (size_t i = 0; i < BLOCKSIZE && continue_search; ++i) {
    continue_search = b->values[i] == uintdef(BLOCKSIZE, 0);
  }
  return continue_search;
}

__attribute__((pure)) static inline bool
read_in_block(size_t x, size_t y, const struct basic_block *b) {
  return b->values[y] & (uintdef(BLOCKSIZE, 1) << x);
}

static inline void write_in_block(struct basic_block *b, size_t x, size_t y,
                                  bool value) {
  if (value)
    b->values[y] |= uintdef(BLOCKSIZE, 1) << x;
  else
    b->values[y] &= ~(uintdef(BLOCKSIZE, 1) << x);
}

enum bb_direction {
  bb_ne = 0,
  bb_se,
  bb_nw,
  bb_sw,
  bb_all_dirs,
};

struct gol_board {
  struct basic_block **bb_buffer[bb_all_dirs];
  size_t size_bb_buffer[bb_all_dirs];
  intmax_t offsetX;
  intmax_t offsetY;
  struct gol_board_bounds board_bounds;
  enum gol_rules rule;
};

struct board_position {
  size_t bbPositionX;
  size_t bbPositionY;
  size_t position_in_bb;
  enum bb_direction direction;
};

__attribute__((const)) static inline struct board_position
position_in_board_structure(intmax_t posX, intmax_t posY) {
  struct board_position bp;
  bp.direction = 0;
  if (posX < 0) {
    posX = -(posX + 1);
    bp.direction += 2;
  }
  if (posY < 0) {
    posY = -(posY + 1);
    bp.direction++;
  }
  intmax_t divX = posX / intdef(MAX, BLOCKSIZE);
  bp.bbPositionX = (size_t)(posX % intdef(MAX, BLOCKSIZE));
  intmax_t divY = posY / intdef(MAX, BLOCKSIZE);
  bp.bbPositionY = (size_t)(posY % intdef(MAX, BLOCKSIZE));
  if (posX < posY) {
    bp.position_in_bb = (size_t)(divY * divY + divX);
  } else {
    bp.position_in_bb = (size_t)(divX * divX + intdef(MAX, 2) * divX - divY);
  }
  return bp;
}

bool read_gol_board(intmax_t posX, intmax_t posY, const struct gol_board *b) {
  struct board_position pos =
      position_in_board_structure(posX + b->offsetX, posY + b->offsetY);
  struct basic_block *bb_to_search =
      b->size_bb_buffer[pos.direction] > pos.position_in_bb
          ? b->bb_buffer[pos.direction][pos.position_in_bb]
          : NULL;
  return bb_to_search != NULL &&
         read_in_block(pos.bbPositionX, pos.bbPositionY, bb_to_search);
}

static inline void realloc_bb_buffer(size_t new_size, size_t *current_size,
                                     struct basic_block ***buffer) {
  if (new_size > *current_size) {
    *buffer = realloc(*buffer, new_size * sizeof(**buffer));
    memset(&(*buffer)[*current_size], 0,
           (new_size - *current_size) * sizeof(**buffer));
    *current_size = new_size;
  }
}

static inline struct basic_block *get_new_empty_bb(struct gol_board *b) {
  (void)b;
  struct basic_block *bb = calloc(1, sizeof(*bb));
  return bb;
}

void write_gol_board(intmax_t posX, intmax_t posY, bool val,
                     struct gol_board *b) {
  struct board_position pos =
      position_in_board_structure(posX + b->offsetX, posY + b->offsetY);
  realloc_bb_buffer(pos.position_in_bb + 1, &b->size_bb_buffer[pos.direction],
                    &b->bb_buffer[pos.direction]);
  if (b->bb_buffer[pos.direction][pos.position_in_bb] == NULL)
    b->bb_buffer[pos.direction][pos.position_in_bb] = get_new_empty_bb(b);
  write_in_block(b->bb_buffer[pos.direction][pos.position_in_bb],
                 pos.bbPositionX, pos.bbPositionY, val);
  if (val) {
    b->board_bounds.upperX = max(b->board_bounds.upperX, posX);
    b->board_bounds.lowerX = min(b->board_bounds.lowerX, posX);
    b->board_bounds.upperY = max(b->board_bounds.upperY, posY);
    b->board_bounds.lowerY = min(b->board_bounds.lowerY, posY);
  }
}

struct gol_board *new_board(void) {
  struct gol_board *board = calloc(1, sizeof(*board));
  return board;
}

void clean_board(struct gol_board *b) {
  for (size_t i = 0; i < bb_all_dirs; ++i) {
    for (size_t j = 0; j < b->size_bb_buffer[i]; ++j) {
      if (b->bb_buffer[i][j] != NULL) {
        free(b->bb_buffer[i][j]);
        b->bb_buffer[i][j] = NULL;
      }
    }
  }
  memset(&b->board_bounds, 0, sizeof(b->board_bounds));
}

void free_game(struct gol_game *game) {
  if (!game)
    return;
  free_board(game->board);
  free(game->patternName);
  free(game->authorName);
  for (size_t i = 0; i < game->num_comments; ++i)
    free(game->comments[i]);
  free(game->comments);
  free(game);
}

void free_board(struct gol_board *b) {
  if (!b)
    return;
  clean_board(b);
  for (size_t i = 0; i < bb_all_dirs; ++i) {
    free(b->bb_buffer[i]);
  }
  free(b);
}

struct gol_board_bounds get_game_bounds(const struct gol_board *b) {
  return b->board_bounds;
}

void add_comment(const char *comment, struct gol_game *b) {
  b->num_comments++;
  b->comments = realloc(b->comments, b->num_comments * sizeof(*b->comments));
  size_t size = strlen(comment);
  b->comments[b->num_comments - 1] = malloc((size + 1) * sizeof(**b->comments));
  memcpy(b->comments[b->num_comments - 1], comment, size);
  b->comments[b->num_comments - 1][size - 1] = '\0';
}

void set_author(const char *authorName, struct gol_game *b) {
  if (b->authorName)
    free(b->authorName);
  size_t size = strlen(authorName);
  b->authorName = malloc((size + 1) * sizeof(*b->authorName));
  memcpy(b->authorName, authorName, size);
  b->authorName[size] = '\0';
}

void set_pattern_name(const char *pName, struct gol_game *b) {
  if (b->patternName)
    free(b->patternName);
  size_t size = strlen(pName);
  b->patternName = malloc((size + 1) * sizeof(*b->patternName));
  memcpy(b->patternName, pName, size);
  b->patternName[size] = '\0';
}

void set_game_rules(enum gol_rules rule, struct gol_board *b) {
  b->rule = rule;
}

void set_offset(intmax_t offsetX, intmax_t offsetY, struct gol_board *b) {
  b->offsetX = offsetX;
  b->offsetY = offsetY;
}

void dump_ASCII(FILE *outStream, struct gol_game *game) {
  if (game->authorName)
    fprintf(outStream, "Author: %s\n", game->authorName);
  if (game->patternName)
    fprintf(outStream, "Pattern name: %s\n", game->patternName);
  if (game->board->offsetX != 0 || game->board->offsetY != 0)
    fprintf(outStream, "Shift from origin: (%" PRIdMAX ", %" PRIdMAX ")\n",
            game->board->offsetX, game->board->offsetY);
  if (game->num_comments)
    fprintf(outStream, "Info:\n");
  for (size_t i = 0; i < game->num_comments; ++i) {
    fprintf(outStream, "%s\n", game->comments[i]);
  }
  if (game->num_comments)
    fprintf(outStream, "\n");
  fprintf(outStream, "Pattern:\n");
  dump_board_ASCII(outStream, game->board);
}

void dump_board_ASCII(FILE *outStream, struct gol_board *b) {
  const struct gol_board_bounds bounds = get_game_bounds(b);
  for (intmax_t j = bounds.lowerY; j <= bounds.upperY; ++j) {
    for (intmax_t i = bounds.lowerX; i <= bounds.upperX; ++i) {
      fprintf(outStream, read_gol_board(i, j, b) ? "O" : " ");
    }
    fprintf(outStream, "\n");
  }
}

bool gol_same_board(const struct gol_board *b1, const struct gol_board *b2) {
  struct gol_board_bounds b1bounds = get_game_bounds(b1),
                          b2bounds = get_game_bounds(b2);

  if (b1bounds.upperX != b2bounds.upperX ||
      b1bounds.lowerX != b2bounds.lowerX ||
      b1bounds.upperY != b2bounds.upperY || b1bounds.lowerY != b2bounds.lowerY)
    return false;
  if (b1->offsetX == b2->offsetX && b1->offsetY == b2->offsetY) {
    for (size_t i = 0; i < bb_all_dirs; ++i) {
      for (size_t j = 0; j < min(b1->size_bb_buffer[i], b2->size_bb_buffer[i]);
           ++j) {
        struct basic_block *bb1 = b1->bb_buffer[i][j];
        struct basic_block *bb2 = b2->bb_buffer[i][j];
        if (bb1 && !bb2) {
          if (!is_empty_block(bb1))
            return false;
        } else if (!bb1 && bb2) {
          if (!is_empty_block(bb2))
            return false;
        } else if (bb2) {
          int retval = memcmp(bb1->values, bb2->values, sizeof(bb1->values));
          if (retval != 0)
            return false;
        }
      }
    }
  } else {
    for (intmax_t i = b1bounds.lowerX; i <= b1bounds.upperX; ++i) {
      for (intmax_t j = b1bounds.lowerY; j <= b1bounds.upperY; ++j) {
        if (read_gol_board(i, j, b1) != read_gol_board(i, j, b2))
          return false;
      }
    }
  }
  return true;
}

void gol_copy_board(const struct gol_board *to_copy, struct gol_board *copy) {
  clean_board(copy);
  copy->board_bounds = get_game_bounds(to_copy);
  set_offset(to_copy->offsetX, to_copy->offsetY, copy);
  set_game_rules(get_game_rules(to_copy), copy);
  for (size_t i = 0; i < bb_all_dirs; ++i) {
    for (size_t j = to_copy->size_bb_buffer[i] - 1;
         j < to_copy->size_bb_buffer[i]; --j) {
      struct basic_block *bb_to_copy = to_copy->bb_buffer[i][j];
      if (bb_to_copy != NULL && !is_empty_block(bb_to_copy)) {
        struct basic_block *bb_copy = get_new_empty_bb(copy);
        memcpy(bb_copy->values, bb_to_copy->values, sizeof(bb_copy->values));
        realloc_bb_buffer(j + 1, &copy->size_bb_buffer[i], &copy->bb_buffer[i]);
        copy->bb_buffer[i][j] = bb_copy;
      }
    }
  }
}

void gol_swap_board(struct gol_board *swap1, struct gol_board *swap2) {
  for (enum bb_direction i = bb_ne; i < bb_all_dirs; ++i) {
    size_t size_bb_tmp;
    struct basic_block **tmp;
    tmp = swap1->bb_buffer[i];
    swap1->bb_buffer[i] = swap2->bb_buffer[i];
    swap2->bb_buffer[i] = tmp;
    size_bb_tmp = swap1->size_bb_buffer[i];
    swap1->size_bb_buffer[i] = swap2->size_bb_buffer[i];
    swap2->size_bb_buffer[i] = size_bb_tmp;
  }
  struct gol_board_bounds tmp_bounds = get_game_bounds(swap1);
  swap1->board_bounds = get_game_bounds(swap2);
  swap2->board_bounds = tmp_bounds;
  intmax_t tmp_OffsetX[2], tmpOffsetY[2];
  get_offset(swap1, &tmp_OffsetX[0], &tmpOffsetY[0]);
  get_offset(swap2, &tmp_OffsetX[1], &tmpOffsetY[1]);
  set_offset(tmp_OffsetX[1], tmpOffsetY[1], swap1);
  set_offset(tmp_OffsetX[0], tmpOffsetY[0], swap2);
}

enum gol_rules get_game_rules(const struct gol_board *b) { return b->rule; }

void clone_metadata(const struct gol_game *b1, struct gol_game *b2) {
  set_author(b1->authorName, b2);
  for (size_t i = 0; i < b1->num_comments; ++i) {
    add_comment(b1->comments[i], b2);
  }
  set_pattern_name(b1->patternName, b2);
}

void get_offset(const struct gol_board *board, intmax_t *offsetX,
                intmax_t *offsetY) {
  *offsetX = board->offsetX;
  *offsetY = board->offsetY;
}
