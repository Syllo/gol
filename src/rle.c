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
#include <stdio.h>

#include "board.h"
#include "mpc.h"
#include "rle.h"

static mpc_val_t *returnLife(mpc_val_t *val) {
  free(val);
  enum gol_rules *rule = malloc(sizeof(*rule));
  *rule = lifeRule;
  return rule;
}

static mpc_val_t *returnHighLife(mpc_val_t *val) {
  free(val);
  enum gol_rules *rule = malloc(sizeof(*rule));
  *rule = highLifeRule;
  return rule;
}

static mpc_val_t *toIntMax(mpc_val_t *val) {
  intmax_t *value = malloc(sizeof(*value));
  sscanf(val, "%" SCNdMAX, value);
  free(val);
  return value;
}

enum preHeaderType {
  preHeaderComment,
  preHeaderPatternName,
  preHeaderCreatorName,
  preHeaderGameRules,
  preHeaderCoordinateOffset,
};

struct preHeader {
  enum preHeaderType type;
  union {
    char *string;
    struct {
      intmax_t offsetX;
      intmax_t offsetY;
    };
    enum gol_rules rule;
  };
};

static mpc_val_t *fold_preHeader(int n, mpc_val_t **prehead) {
  struct preHeader **allPreHeaders =
      malloc(((size_t)n + 1) * sizeof(*allPreHeaders));
  memcpy(allPreHeaders, prehead, (size_t)n * sizeof(*allPreHeaders));
  allPreHeaders[n] = NULL;
  return allPreHeaders;
}

static mpc_val_t *phComment(mpc_val_t *val) {
  struct preHeader *ph = malloc(sizeof(*ph));
  ph->type = preHeaderComment;
  ph->string = val;
  return ph;
}

static mpc_val_t *phPatternName(mpc_val_t *val) {
  struct preHeader *ph = malloc(sizeof(*ph));
  ph->type = preHeaderPatternName;
  ph->string = val;
  return ph;
}

static mpc_val_t *phCreatorName(mpc_val_t *val) {
  struct preHeader *ph = malloc(sizeof(*ph));
  ph->type = preHeaderCreatorName;
  ph->string = val;
  return ph;
}

static mpc_val_t *phGameRules(mpc_val_t *val) {
  struct preHeader *ph = malloc(sizeof(*ph));
  ph->type = preHeaderGameRules;
  enum gol_rules *rule = (enum gol_rules *)val;
  ph->rule = *rule;
  free(rule);
  return ph;
}

static mpc_val_t *phCoordinateOffset(int n, mpc_val_t **val) {
  (void)n;
  struct preHeader *ph = malloc(sizeof(*ph));
  ph->type = preHeaderCoordinateOffset;
  intmax_t **vv = (intmax_t **)val;
  ph->offsetX = *(vv[1]);
  ph->offsetY = *(vv[2]);
  free(val[0]);
  free(val[1]);
  free(val[2]);
  return ph;
}

struct headerLine {
  intmax_t sizeX;
  intmax_t sizeY;
  enum gol_rules ruleSet;
};

static mpc_val_t *headerFold(int n, mpc_val_t **val) {
  struct headerLine *hl = malloc(sizeof(*hl));
  intmax_t **vv = (intmax_t **)val;
  enum gol_rules *rl = (enum gol_rules *)val[7];
  if (rl != NULL) {
    hl->ruleSet = *rl;
    free(rl);
  } else {
    hl->ruleSet = unknownRule;
  }
  hl->sizeX = *vv[2];
  hl->sizeY = *vv[6];
  for (int i = 0; i < n; ++i) {
    if (i != 7)
      free(val[i]);
  }
  return hl;
}

enum itemType {
  itemDead,
  itemAlive,
  itemLineJump,
};

struct Item {
  enum itemType itemType;
  intmax_t num;
};

static mpc_val_t *itemFold(int n, mpc_val_t **val) {
  struct Item **allItems = malloc(((size_t)n + 1) * sizeof(*allItems));
  memcpy(allItems, val, (size_t)n * sizeof(*allItems));
  allItems[n] = NULL;
  return allItems;
}

static mpc_val_t *newItem(int n, mpc_val_t **val) {
  (void)n;
  struct Item *item = malloc(sizeof(*item));
  char *type = (char *)val[1];
  switch (*type) {
  case 'b':
    item->itemType = itemDead;
    break;
  case 'o':
    item->itemType = itemAlive;
    break;
  case '$':
    item->itemType = itemLineJump;
    break;
  default:
    item->itemType = itemAlive;
    break;
  }
  free(val[1]);
  if (val[0] != NULL) {
    intmax_t *num_items = (intmax_t *)val[0];
    item->num = *num_items;
    free(val[0]);
  } else {
    item->num = INTMAX_C(1);
  }
  return item;
}

static mpc_val_t *foldRleFile(int n, mpc_val_t **val) {
  (void)n;
  struct gol_game *game = (struct gol_game *)val[0];
  struct preHeader **ph = (struct preHeader **)val[1];
  struct headerLine *hl = (struct headerLine *)val[2];
  struct Item **items = (struct Item **)val[3];
  struct preHeader *tmpPH = ph[0];
  for (size_t num = 0; tmpPH != NULL; tmpPH = ph[++num]) {
    switch (tmpPH->type) {
    case preHeaderComment:
      add_comment(tmpPH->string, game);
      // fprintf(stderr, "Ph comment %s\n", tmpPH->string);
      free(tmpPH->string);
      break;
    case preHeaderGameRules:
      set_game_rules(tmpPH->rule, game->board);
      /*fprintf(stderr, "Ph rules %s\n", tmpPH->string);*/
      break;
    case preHeaderCreatorName:
      set_author(tmpPH->string, game);
      /*fprintf(stderr, "Ph creator %s\n", tmpPH->string);*/
      free(tmpPH->string);
      break;
    case preHeaderPatternName:
      set_pattern_name(tmpPH->string, game);
      /*fprintf(stderr, "Ph pattennName %s\n", tmpPH->string);*/
      free(tmpPH->string);
      break;
    case preHeaderCoordinateOffset:
      set_offset(tmpPH->offsetX, tmpPH->offsetY, game->board);
      /*fprintf(stderr, "Ph offset %" PRIdMAX " %" PRIdMAX "\n",
       * tmpPH->offsetX,*/
      /*tmpPH->offsetY);*/
      break;
    }
    free(tmpPH);
  }
  /*fprintf(stderr, "Header x = %" PRIdMAX ", y = %" PRIdMAX ", ruleset =
   * %d\n",*/
  /*hl->sizeX, hl->sizeY, hl->ruleSet);*/
  if (hl->ruleSet != unknownRule)
    set_game_rules(hl->ruleSet, game->board);
  struct Item *tmpItem = items[0];
  intmax_t posX = 0, posY = 0;
  for (size_t num = 0; tmpItem != NULL; tmpItem = items[++num]) {
    switch (tmpItem->itemType) {
    case itemDead:
      posX += tmpItem->num;
      /*fprintf(stderr, " %" PRIdMAX " Dead", tmpItem->num);*/
      break;
    case itemAlive:
      for (intmax_t i = 0; i < tmpItem->num; ++i, ++posX)
        write_gol_board(posX, posY, true, game->board);
      /*fprintf(stderr, " %" PRIdMAX " Alive", tmpItem->num);*/
      break;
    case itemLineJump:
      posY += tmpItem->num;
      posX = 0;
      /*fprintf(stderr, " %" PRIdMAX " EndL\n", tmpItem->num);*/
      break;
    }
    free(tmpItem);
  }
  free(hl);
  free(ph);
  free(items);
  return game;
}

static void freeAllPreHeader(void **all) {
  struct preHeader *tmpPH = all[0];
  for (size_t num = 0; tmpPH != NULL; tmpPH = all[++num]) {
    switch (tmpPH->type) {
    case preHeaderComment:
    case preHeaderCreatorName:
    case preHeaderPatternName:
      free(tmpPH->string);
      break;
    case preHeaderGameRules:
    case preHeaderCoordinateOffset:
      break;
    }
    free(tmpPH);
  }
  free(all);
}
static void freeAllItems(void **all) {
  void *val = all[0];
  for (size_t num = 0; val != NULL; val = all[++num]) {
    free(val);
  }
  free(all);
}

bool parse_rle_file(const char *rle_file, struct gol_game **b) {

  *b = calloc(1, sizeof(**b));
  (*b)->board = new_board();

  mpc_parser_t *Life = mpc_new("life");
  mpc_parser_t *HighLife = mpc_new("highLife");
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *PositiveNumber = mpc_new("positiveNumber");
  mpc_parser_t *StringLine = mpc_new("stringLine");
  mpc_parser_t *RuleSet = mpc_new("ruleSet");
  mpc_parser_t *HeaderLine = mpc_new("headerLine");
  mpc_parser_t *Comment = mpc_new("comment");
  mpc_parser_t *PatternName = mpc_new("patternName");
  mpc_parser_t *CreatorName = mpc_new("creatorName");
  mpc_parser_t *GameRules = mpc_new("gameRules");
  mpc_parser_t *CoordinateOffset = mpc_new("coordinateOffset");
  mpc_parser_t *Item = mpc_new("item");
  mpc_parser_t *CellGrid = mpc_new("cellGrid");
  mpc_parser_t *RleFile = mpc_new("rleFile");
  mpc_parser_t *PreHeader = mpc_new("preHeader");

  mpc_define(
      Item,
      mpc_stripl(mpc_expect(
          mpc_and(2, newItem, mpc_maybe(PositiveNumber),
                  mpc_or(2, mpc_re("[a-zA-Z]"), mpc_char('$')), free),
          "an item: <positiveInteger>(b|o|$), where <positiveInteger> can be "
          "ommited if equal to one,")));

  mpc_define(
      HeaderLine,
      mpc_predictive(mpc_and(
          8, headerFold, mpc_stripl(mpc_char('x')), mpc_stripl(mpc_char('=')),
          mpc_stripl(PositiveNumber), mpc_stripl(mpc_char(',')),
          mpc_stripl(mpc_char('y')), mpc_stripl(mpc_char('=')),
          mpc_stripl(PositiveNumber),
          mpc_maybe(mpc_and(2, mpcf_snd,
                            mpc_and(3, mpcf_freefold, mpc_stripl(mpc_char(',')),
                                    mpc_stripl(mpc_string("rule")),
                                    mpc_stripl(mpc_char('=')), free, free),
                            mpc_stripl(RuleSet), mpcf_dtor_null)),
          free, free, free, free, free, free, free)));

  mpc_define(Life, mpc_apply(mpc_or(3, mpc_string("B3/S23"),
                                    mpc_string("b3/s23"), mpc_string("23/3")),
                             returnLife));

  mpc_define(HighLife,
             mpc_apply(mpc_or(3, mpc_string("B36/S23"), mpc_string("b36/s23"),
                              mpc_string("23/36")),
                       returnHighLife));

  mpc_define(RuleSet, mpc_or(2, Life, HighLife));

  mpc_define(Number,
             mpc_expect(mpc_apply(mpc_re("-?[0-9]+"), toIntMax), "an integer"));
  mpc_define(PositiveNumber,
             mpc_expect(mpc_apply(mpc_re("[1-9][0-9]*"), toIntMax),
                        "a positive and greater than zero integer"));

  mpc_define(StringLine, mpc_re("[^\\n]*"));

  mpc_define(PreHeader,
             mpc_many(fold_preHeader,
                      mpc_and(2, mpcf_snd_free, mpc_stripl(mpc_char('#')),
                              mpc_predictive(mpc_expect(
                                  mpc_or(5, Comment, PatternName, CreatorName,
                                         GameRules, CoordinateOffset),
                                  "'#' followed by one of the type character "
                                  "(C,c,N,r,R,P,O)")),
                              free)));

  mpc_define(Comment,
             mpc_and(2, mpcf_snd_free, mpc_or(2, mpc_char('c'), mpc_char('C')),
                     mpc_apply(mpc_stripl(StringLine), phComment), free));

  mpc_define(PatternName,
             mpc_and(2, mpcf_snd_free, mpc_char('N'),
                     mpc_apply(mpc_stripl(StringLine), phPatternName), free));

  mpc_define(CreatorName,
             mpc_and(2, mpcf_snd_free, mpc_char('O'),
                     mpc_apply(mpc_stripl(StringLine), phCreatorName), free));

  mpc_define(GameRules,
             mpc_and(2, mpcf_snd_free, mpc_char('r'),
                     mpc_apply(mpc_stripl(StringLine), phGameRules), free));

  mpc_define(CoordinateOffset,
             mpc_and(3, phCoordinateOffset,
                     mpc_or(2, mpc_char('P'), mpc_char('R')),
                     mpc_stripl(Number), mpc_stripl(Number), free, free));

  mpc_define(CellGrid, mpc_and(2, mpcf_fst_free, mpc_many(itemFold, Item),
                               mpc_expect(mpc_stripl(mpc_char('!')),
                                          "\"End of pattern: '!'\""),
                               freeAllItems));

  mpc_define(RleFile,
             mpc_and(4, foldRleFile, mpc_lift_val(*b), PreHeader, HeaderLine,
                     CellGrid, mpcf_dtor_null, freeAllPreHeader, free));

  mpc_optimise(Number);
  mpc_optimise(PositiveNumber);
  mpc_optimise(StringLine);
  mpc_optimise(Life);
  mpc_optimise(HighLife);
  mpc_optimise(RuleSet);
  mpc_optimise(Comment);
  mpc_optimise(PatternName);
  mpc_optimise(CreatorName);
  mpc_optimise(GameRules);
  mpc_optimise(CoordinateOffset);
  mpc_optimise(PreHeader);
  mpc_optimise(HeaderLine);
  mpc_optimise(Item);
  mpc_optimise(CellGrid);
  mpc_optimise(RleFile);

  mpc_result_t r;
  int parse_success = mpc_parse_contents(rle_file, RleFile, &r);
  if (!parse_success) {
    fprintf(stderr, "Error while parsing input rle file:\n");
    mpc_err_print_to(r.error, stderr);
    mpc_err_delete(r.error);
    free_game(*b);
  }

  mpc_cleanup(16, Number, StringLine, Life, HighLife, RuleSet, Comment,
              PatternName, CreatorName, GameRules, CoordinateOffset, PreHeader,
              HeaderLine, Item, CellGrid, RleFile, PositiveNumber);
  return parse_success;
}

static void print_cell_state(FILE *output_file, bool previous_was_alive,
                             size_t previous_cells_in_state,
                             int *num_written_in_line) {
  if (previous_cells_in_state) {
    char p = previous_was_alive ? 'o' : 'b';
    int num_to_write;
    if (previous_cells_in_state > 1) {
      num_to_write = snprintf(NULL, 0, "%zu%c", previous_cells_in_state, p);
    } else {
      num_to_write = 1;
    }
    if (*num_written_in_line + num_to_write > 69) {
      fprintf(output_file, "\n");
      *num_written_in_line = 0;
    }
    *num_written_in_line +=
        previous_cells_in_state > 1
            ? fprintf(output_file, "%zu%c", previous_cells_in_state, p)
            : fprintf(output_file, "%c", p);
  }
}

void dump_rle(FILE *output_file, struct gol_game *b) {
  const struct gol_board *board = b->board;
  // Pre-Header
  if (b->authorName)
    fprintf(output_file, "#O %s\n", b->authorName);
  if (b->patternName)
    fprintf(output_file, "#N %s\n", b->patternName);
  intmax_t offsetX, offsetY;
  get_offset(board, &offsetX, &offsetY);
  if (offsetX != 0 || offsetY != 0)
    fprintf(output_file, "#R %" PRIdMAX " %" PRIdMAX "\n", offsetX, offsetY);
  for (size_t i = 0; i < b->num_comments; ++i) {
    fprintf(output_file, "#C %s\n", b->comments[i]);
  }
  // Header
  struct gol_board_bounds bounds = get_game_bounds(board);
  fprintf(output_file, "x = %" PRIdMAX ", y = %" PRIdMAX ", rule = %s\n",
          bounds.upperX - bounds.lowerX + 1, bounds.upperY - bounds.lowerY + 1,
          gol_rule_string[get_game_rules(b->board)]);

  int num_written_in_line = 0;
  size_t consecutive_empty = 0;
  for (intmax_t j = bounds.lowerY; j <= bounds.upperY; ++j) {
    bool previous_was_alive = false;
    size_t previous_cells_in_state = 0;
    for (intmax_t i = bounds.lowerX; i <= bounds.upperX; ++i) {
      bool cell_state = read_gol_board(i, j, board);
      if (previous_was_alive == cell_state) {
        previous_cells_in_state++;
      } else {
        if (consecutive_empty) {
          int num_to_write = consecutive_empty > 1
                                 ? snprintf(NULL, 0, "%zu$", consecutive_empty)
                                 : 1;
          if (num_written_in_line + num_to_write > 69) {
            fprintf(output_file, "\n");
            num_written_in_line = 0;
          }
          num_written_in_line +=
              consecutive_empty > 1
                  ? fprintf(output_file, "%zu$", consecutive_empty)
                  : fprintf(output_file, "$");
          consecutive_empty = 0;
        }
        print_cell_state(output_file, previous_was_alive,
                         previous_cells_in_state, &num_written_in_line);
        previous_was_alive = !previous_was_alive;
        previous_cells_in_state = 1;
      }
    }
    if (previous_was_alive)
      print_cell_state(output_file, previous_was_alive, previous_cells_in_state,
                       &num_written_in_line);
    consecutive_empty++;
  }
  if (num_written_in_line > 68)
    fprintf(output_file, "\n");
  else
    fprintf(output_file, "!\n");
}
