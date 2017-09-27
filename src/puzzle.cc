/*
 * Copyright (c) 2004-2006  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of Somato.
 *
 * Somato is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Somato is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Somato.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "puzzle.h"

#include <glib.h>
#include <algorithm>
#include <chrono>
#include <functional>
#include <numeric>

namespace
{

using Somato::Cube;
using Somato::Solution;

typedef std::vector<Cube> PieceStore;
typedef std::array<PieceStore, Somato::CUBE_PIECE_COUNT> ColumnStore;

class PuzzleSolver
{
public:
  PuzzleSolver() = default;
  std::vector<Solution> execute();

  PuzzleSolver(const PuzzleSolver&) = delete;
  PuzzleSolver& operator=(const PuzzleSolver&) = delete;

private:
  ColumnStore           columns_;
  std::vector<Solution> solutions_;
  Solution              state_;

  void recurse(int col, Cube cube);
};

/*
 * Cube pieces rearranged for maximum efficiency.  It is about 15 times
 * faster than with the original order from the project description.
 * The cube piece at index 0 should be suitable for use as the anchor.
 */
const std::array<Cube, Somato::CUBE_PIECE_COUNT> cube_piece_data
{{
  { // Piece #6
    1,1,0, 0,0,0, 0,0,0,
    0,1,0, 0,1,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #7
    1,1,0, 0,1,0, 0,0,0,
    0,1,0, 0,0,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #5
    1,1,0, 1,0,0, 0,0,0,
    0,1,0, 0,0,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #4
    1,0,0, 1,0,0, 0,0,0,
    0,0,0, 1,0,0, 1,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #3
    1,0,0, 1,0,0, 1,0,0,
    0,0,0, 1,0,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #2
    1,0,0, 1,0,0, 1,0,0,
    1,0,0, 0,0,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  },
  { // Piece #1
    1,0,0, 1,0,0, 0,0,0,
    1,0,0, 0,0,0, 0,0,0,
    0,0,0, 0,0,0, 0,0,0
  }
}};

/*
 * Rotate the cube.  This takes care of all orientations possible.
 */
void compute_rotations(Cube cube, PieceStore& store)
{
  for (unsigned int i = 0;; ++i)
  {
    Cube temp = cube;

    // Add the 4 possible orientations of each cube side.
    store.push_back(temp);
    store.push_back(temp.rotate(Cube::AXIS_Z));
    store.push_back(temp.rotate(Cube::AXIS_Z));
    store.push_back(temp.rotate(Cube::AXIS_Z));

    if (i == 5)
      break;

    // Due to the zigzagging performed here, only 5 rotations are
    // necessary to move each of the 6 cube sides in turn to the front.
    cube.rotate(Cube::AXIS_X + i % 2);
  }
}

/*
 * Push the Soma block around; into every position respectively rotation
 * imaginable.  Note that the block is assumed to be positioned initially
 * in the (0, 0, 0) corner of the cube.
 */
void shuffle_cube_piece(Cube cube, PieceStore& store)
{
  // Make sure the piece is positioned where we expect it to be.
  g_return_if_fail(cube.get(0, 0, 0));

  for (Cube z = cube; z != Cube{}; z.shift(Cube::AXIS_Z))
    for (Cube y = z; y != Cube{}; y.shift(Cube::AXIS_Y))
      for (Cube x = y; x != Cube{}; x.shift(Cube::AXIS_X))
      {
        compute_rotations(x, store);
      }
}

/*
 * Replace store by a new set of piece placements that contains only those
 * items from the source which cannot be reproduced by rotating any other
 * item.  This is not a universally applicable utility function; the input
 * is assumed to have come straight out of shuffle_cube_piece().
 */
void filter_rotations(PieceStore& store)
{
  g_return_if_fail(store.size() % 24 == 0);

  auto pdest = store.begin();

  for (auto p = store.cbegin(); p != store.cend(); p += 24)
  {
    *pdest++ = *std::min_element(p, p + 24, Cube::SortPredicate{});
  }

  store.erase(pdest, store.end());
}

bool find_piece_translation(Cube original, Cube piece, Math::Matrix4& transform)
{
  int z = 0;

  for (Cube piece_z = piece; piece_z != Cube{}; piece_z.shift_rev(Cube::AXIS_Z))
  {
    int y = 0;

    for (Cube piece_y = piece_z; piece_y != Cube{}; piece_y.shift_rev(Cube::AXIS_Y))
    {
      int x = 0;

      for (Cube piece_x = piece_y; piece_x != Cube{}; piece_x.shift_rev(Cube::AXIS_X))
      {
        if (piece_x == original)
        {
          transform.translate(Math::Vector4(x, y, z, 1.f));
          return true;
        }
        ++x;
      }
      ++y;
    }
    --z;
  }
  return false;
}

std::vector<Somato::Solution> PuzzleSolver::execute()
{
  solutions_.reserve(480);

  for (size_t i = 0; i < Somato::CUBE_PIECE_COUNT; ++i)
  {
    PieceStore& store = columns_[i];

    store.reserve(256);
    shuffle_cube_piece(cube_piece_data[i], store);

    if (i == 0)
      filter_rotations(store);

    std::sort(store.begin(), store.end(), Cube::SortPredicate{});
    store.erase(std::unique(store.begin(), store.end()), store.end());
  }

  const Cube common = std::accumulate(columns_[0].begin(), columns_[0].end(),
                                      ~Cube{}, std::bit_and<Cube>{});
  if (common != Cube{})
    for (size_t i = 1; i < Somato::CUBE_PIECE_COUNT; ++i)
    {
      columns_[i].erase(std::remove_if(columns_[i].begin(), columns_[i].end(),
                                       [common](Cube c) { return ((c & common) != Cube{}); }),
                        columns_[i].end());
    }

  // Add zero-termination.
  for (auto& column : columns_)
    column.push_back({});

  recurse(0, {});

  return std::move(solutions_);
}

void PuzzleSolver::recurse(int col, Cube cube)
{
  auto row = columns_[col].cbegin();

  for (;;)
  {
    const Cube piece = *row++;

    if ((piece & cube) == Cube{})
    {
      if (piece == Cube{})
        break;

      state_[col] = piece;

      if (col < Somato::CUBE_PIECE_COUNT - 1)
        recurse(col + 1, cube | piece);
      else
        solutions_.push_back(state_);
    }
  }
}

} // anonymous namespace

namespace Somato
{

PuzzleThread::PuzzleThread()
{}

PuzzleThread::~PuzzleThread()
{
  wait_finish();
}

std::vector<Solution> PuzzleThread::acquire_results()
{
  rethrow_any_error();
  return std::move(solutions_);
}

void PuzzleThread::execute()
{
  const auto start = std::chrono::steady_clock::now();
  {
    PuzzleSolver solver;
    solutions_ = solver.execute();
  }
  const auto stop = std::chrono::steady_clock::now();
  const std::chrono::duration<double, std::milli> elapsed = stop - start;

  g_info("Puzzle solve time: %0.1f ms", elapsed.count());
}

Math::Matrix4 find_puzzle_piece_orientation(int piece_idx, Cube piece)
{
  static const Math::Matrix4 rotate90[3] =
  {
    {{1, 0,  0}, { 0, 0, -1}, {0, 1, 0}}, // 90 deg around x
    {{0, 0, -1}, { 0, 1,  0}, {1, 0, 0}}, // 90 deg around y
    {{0, 1,  0}, {-1, 0,  0}, {0, 0, 1}}  // 90 deg around z
  };
  Math::Matrix4 transform;

  g_return_val_if_fail(piece_idx >= 0 && piece_idx < CUBE_PIECE_COUNT, transform);

  const Cube original = cube_piece_data[piece_idx];

  for (size_t i = 0; i < 6; ++i)
  {
    // Add the 4 possible orientations of each cube side.
    for (int k = 0; k < 4; ++k)
    {
      if (find_piece_translation(original, piece, transform))
        return transform;

      piece.rotate(Cube::AXIS_Z);
      transform *= rotate90[Cube::AXIS_Z];
    }

    // Due to the zigzagging performed here, only 5 rotations are
    // necessary to move each of the 6 cube sides in turn to the front.
    piece.rotate(i % 2);
    transform *= rotate90[i % 2];
  }

  g_warn_if_reached();
  return transform;
}

} // namespace Somato
