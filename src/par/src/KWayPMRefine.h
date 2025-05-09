// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "KWayFMRefine.h"
#include "Refiner.h"
#include "Utilities.h"

namespace par {

class KWayPMRefine;
using KWayPMRefinerPtr = std::shared_ptr<KWayPMRefine>;

// ------------------------------------------------------------------------------
// K-way pair-wise FM refinement
// ------------------------------------------------------------------------------
// The motivation is that FM can achieve better performance for 2-way
// partitioning than k-way partitioning. So we decompose the k-way partitioning
// into multiple 2-way partitioning through maximum matching. In the first
// iteration, the maximum matching is based on connectivity between each blocks.
// In the remaing iterations, the maximum matching is based on the delta gain
// for each block. Based on the paper of pair-wise PM, we can not keep using
// connectivity based maximum matching.  Otherwise, we may easily got stuck in
// local minimum. We use multiple multiple member functions of KWayFMRefine
// especially the functions related to gain buckets
class KWayPMRefine : public KWayFMRefine
{
 public:
  using KWayFMRefine::KWayFMRefine;

 private:
  // In each pass, we only move the boundary vertices
  // here we pass block_balance and net_degrees as reference
  // because we only move a few vertices during each pass
  // i.e., block_balance and net_degs will not change too much
  // so we precompute the block_balance and net_degs
  // the return value is the gain improvement
  float Pass(const HGraphPtr& hgraph,
             const Matrix<float>& upper_block_balance,
             const Matrix<float>& lower_block_balance,
             Matrix<float>& block_balance,        // the current block balance
             Matrix<int>& net_degs,               // the current net degree
             std::vector<float>& cur_paths_cost,  // the current path cost
             Partitions& solution,
             std::vector<bool>& visited_vertices_flag) override;

  // The function to calculate the matching_scores
  void CalculateMaximumMatch(
      std::vector<std::pair<int, int>>& maximum_matches,
      const std::map<std::pair<int, int>, float>& matching_scores) const;

  // Perform 2-way FM between blocks in partition pair
  float PerformPairFM(
      const HGraphPtr& hgraph,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance,
      Matrix<float>& block_balance,    // the current block balance
      Matrix<int>& net_degs,           // the current net degree
      std::vector<float>& paths_cost,  // the current path cost
      Partitions& solution,
      GainBuckets& buckets,
      std::vector<bool>& visited_vertices_flag,
      const std::pair<int, int>& partition_pair) const;

  // gain bucket related functions
  // Initialize the gain buckets in parallel
  void InitializeGainBucketsPM(GainBuckets& buckets,
                               const HGraphPtr& hgraph,
                               const std::vector<int>& boundary_vertices,
                               const Matrix<int>& net_degs,
                               const std::vector<float>& cur_paths_cost,
                               const Partitions& solution,
                               const std::pair<int, int>& partition_pair) const;

  // variables
  // the connectivity between different blocks.
  // (block_a, block_b, score) where block_a < block_b
  std::map<std::pair<int, int>, float> pre_matching_connectivity_;
};

}  // namespace par
