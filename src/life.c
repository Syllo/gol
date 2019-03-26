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

#include "board.h"
#include "life.h"

__attribute__((const)) static inline bool is_alive_life(bool previous_state,
                                                        size_t num_alive) {
  switch (num_alive) {
  case 3:
    return true;
  case 2:
    return previous_state;
  default:
    return false;
  }
}

__attribute__((const)) static inline bool is_alive_hilife(bool previous_state,
                                                          size_t num_alive) {
  switch (num_alive) {
  case 3:
  case 6:
    return true;
  case 2:
    return previous_state;
  default:
    return false;
  }
}

static void get_next_generation(const struct gol_board *previous,
                                struct gol_board *next,
                                bool (*new_state)(bool, size_t)) {
  struct gol_board_bounds previous_bounds = get_game_bounds(previous);
  for (intmax_t i = previous_bounds.lowerX - 1; i <= previous_bounds.upperX + 1;
       ++i) {
    for (intmax_t j = previous_bounds.lowerY - 1;
         j <= previous_bounds.upperY + 1; ++j) {
      bool val = read_gol_board(i, j, previous);
      size_t num_alive = val ? SIZE_MAX : 0;
      for (intmax_t k = i - 1; k <= i + 1; ++k) {
        for (intmax_t l = j - 1; l <= j + 1; ++l) {
          num_alive += read_gol_board(k, l, previous) ? 1 : 0;
        }
      }
      if (new_state(val, num_alive))
        write_gol_board(i, j, true, next);
    }
  }
}

static void get_next_generation_iterator(struct gol_board *previous,
                                         struct gol_board *next,
                                         bool (*new_state)(bool, size_t)) {
  struct gol_board_iterator *it = board_iterator_start(previous);
  while (!board_iterator_is_end(it)) {
    const struct gol_board_iterator_position pos = board_iterator_position(it);
    for (intmax_t k = pos.posX - 1; k <= pos.posX + 1; ++k) {
      for (intmax_t l = pos.posY - 1; l <= pos.posY + 1; ++l) {
        if (!read_gol_board(k, l, next)) {
          bool val = read_gol_board(k, l, previous);
          size_t num_alive = val ? SIZE_MAX : 0;
          for (intmax_t i = k - 1; i <= k + 1; ++i) {
            for (intmax_t j = l - 1; j <= l + 1; ++j) {
              num_alive += read_gol_board(i, j, previous) ? 1 : 0;
            }
          }
          if (new_state(val, num_alive))
            write_gol_board(k, l, true, next);
        }
      }
    }
    it = board_iterator_next(it);
  }
  board_iterator_free(it);
}

static inline void center_offset(struct gol_board_bounds *bounds,
                                 struct gol_board *board) {
  intmax_t offsetX = -(bounds->upperX - bounds->lowerX) / 2;
  intmax_t offsetY = -(bounds->upperY - bounds->lowerY) / 2;
  set_offset(offsetX, offsetY, board);
}

void evolve_to_generation_n(size_t generation,
                            struct gol_board *const start_gen, bool verbose,
                            bool iterator) {
  if (generation == 0)
    return;
  struct gol_board_bounds bounds;

  struct gol_board *next_gen = new_board();
  set_game_rules(get_game_rules(start_gen), next_gen);
  struct gol_board *current_gen = start_gen;

  enum gol_rules rule = get_game_rules(start_gen);
  float percentage;
  size_t verbose_step;
  if (generation >= 20) {
    verbose_step = generation / 20;
    percentage = 100. * verbose_step / (float)generation;
  } else {
    verbose_step = 1;
    percentage = 100. / (float)generation;
  }
  bool (*life_count)(bool,size_t) = NULL;
  switch (rule) {
    case highLifeRule:
      life_count = is_alive_hilife;
      break;
    case lifeRule:
    default:
      life_count = is_alive_life;
      break;
  }
  // Kernel
  for (size_t i = 0; i < generation; ++i) {
    if (verbose && i % verbose_step == 0) {
      printf("\rGeneration avancement %.0f%%", i / verbose_step * percentage);
      fflush(stdout);
    }
    clean_board(next_gen);
    bounds = get_game_bounds(current_gen);
    // Re-center the to spare memory
    center_offset(&bounds, next_gen);
    if (iterator)
      get_next_generation_iterator(current_gen, next_gen, life_count);
    else
      get_next_generation(current_gen, next_gen, life_count);
    struct gol_board *swap_b = current_gen;
    current_gen = next_gen;
    next_gen = swap_b;
  }

  if (verbose)
    printf("\rGeneration avancement 100%%\n");
  if (current_gen != start_gen) {
    gol_swap_board(next_gen, current_gen);
    free_board(current_gen);
  } else {
    free_board(next_gen);
  }
}
