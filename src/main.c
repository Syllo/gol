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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "board.h"
#include "life.h"
#include "rle.h"
#include "time_measurement.h"

static struct option opt_options[] = {
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'v'},
    {"output", required_argument, 0, 'o'},
    {"compare-rle", required_argument, 0, 'c'},
    {"generation", required_argument, 0, 'g'},
    {"force-life", no_argument, 0, 'l'},
    {"force-highlife", no_argument, 0, 'L'},
    {"ascii-output", no_argument, 0, 'a'},
    {0, 0, 0, 0}};

static const char options[] = ":ho:c:g:lLav";

static const char help_string[] =
    "Options:"
    "\n  -o --output          : Output file"
    "\n  -c --compare-rle     : Compare the result to this file"
    "\n  -g --generation      : Select end generation (default 0)"
    "\n  -l --force-life      : Select Life rule"
    "\n  -L --force-highlife  : Select HighLife rule"
    "\n  -a --ascii-output    : Output grid as ASCII"
    "\n  -v --verbose         : Print solver avancement information"
    "\n  -h --help            : Print this help";

int main(int argc, char **argv) {
  size_t goto_generation = 0;
  char *output_file_name = NULL;
  char *rle_to_compare = NULL;
  bool force_life = false;
  bool force_highlife = false;
  bool output_ascii = false;
  bool verbose = false;

  while (true) {
    int sscanf_return;
    int optchar = getopt_long(argc, argv, options, opt_options, NULL);
    if (optchar == -1)
      break;
    switch (optchar) {
    case 'o':
      output_file_name = optarg;
      break;
    case 'c':
      rle_to_compare = optarg;
      break;
    case 'g':
      sscanf_return = sscanf(optarg, "%zu", &goto_generation);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(
            stderr,
            "Please input a positive generation number instead of \"-%c %s\"\n",
            optchar, optarg);
        goto_generation = 0;
      }
      break;
    case 'l':
      force_life = true;
      force_highlife = false;
      break;
    case 'L':
      force_life = false;
      force_highlife = true;
      break;
    case 'a':
      output_ascii = true;
      break;
    case 'v':
      verbose = true;
      break;
    case 'h':
      printf("Usage: %s <options> start_generation.rle\n%s\n", argv[0],
             help_string);
      return EXIT_SUCCESS;
      break;
    case ':':
      fprintf(stderr, "Option %c requires an argument\n", optopt);
      exit(EXIT_FAILURE);
      break;
    default:
      fprintf(stderr, "Unrecognized option %c\n", optopt);
      exit(EXIT_FAILURE);
      break;
    }
  }

  if (optind != argc - 1) {
    fprintf(stderr, "Usage: %s <options> start_generation.rle\n%s\n", argv[0],
            help_string);
    exit(EXIT_FAILURE);
  }
  char *input_file_name = argv[optind];
  struct gol_game *game = NULL;
  bool has_parsed = parse_rle_file(input_file_name, &game);
  if (!has_parsed)
    exit(EXIT_FAILURE);
  struct gol_game *comparison_board = NULL;
  if (rle_to_compare) {
    has_parsed = parse_rle_file(rle_to_compare, &comparison_board);
    if (!has_parsed)
      exit(EXIT_FAILURE);
  }
  FILE *output_file = NULL;
  if (output_file_name) {
    if (output_file_name[0] != '\0' && output_file_name[0] == '-' &&
        output_file_name[1] == '\0') {
      output_file = stdout;
    } else {
      output_file = fopen(output_file_name, "w");
      if (output_file == NULL) {
        perror("Error while opening the output file: ");
        exit(EXIT_FAILURE);
      }
    }
  }
  if (output_ascii && !output_file)
    output_file = stdout;
  if (force_life)
    set_game_rules(lifeRule, game->board);
  if (force_highlife)
    set_game_rules(highLifeRule, game->board);

  time_measure startTime, endTime;
  get_current_time(&startTime);
  evolve_to_generation_n(goto_generation, game->board, verbose);
  get_current_time(&endTime);
  fprintf(stdout, "Kernel time %.4fs\n",
          measuring_difftime(startTime, endTime));
  if (output_file) {
    if (output_ascii)
      dump_ASCII(output_file, game);
    else
      dump_rle(output_file, game);
  }

  bool same_board = true;
  if (rle_to_compare)
    same_board = gol_same_board(game->board, comparison_board->board);

  free_game(game);
  free_game(comparison_board);
  if (output_file)
    fclose(output_file);
  return !same_board;
}
