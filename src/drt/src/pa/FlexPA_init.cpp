// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "FlexPA.h"
#include "db/infra/frTime.h"
#include "gc/FlexGC.h"

namespace drt {

void FlexPA::initViaRawPriority()
{
  for (int layer_num = design_->getTech()->getBottomLayerNum();
       layer_num <= design_->getTech()->getTopLayerNum();
       ++layer_num) {
    if (design_->getTech()->getLayer(layer_num)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    for (const auto& via_def :
         design_->getTech()->getLayer(layer_num)->getViaDefs()) {
      const int cutNum = int(via_def->getCutFigs().size());
      const ViaRawPriorityTuple priority = getViaRawPriority(via_def);
      layer_num_to_via_defs_[layer_num][cutNum][priority] = via_def;
    }
  }
}

ViaRawPriorityTuple FlexPA::getViaRawPriority(const frViaDef* via_def)
{
  const bool is_not_default_via = !(via_def->getDefault());
  gtl::polygon_90_set_data<frCoord> via_layer_ps1;

  for (auto& fig : via_def->getLayer1Figs()) {
    const Rect bbox = fig->getBBox();
    gtl::rectangle_data<frCoord> bbox_rect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using boost::polygon::operators::operator+=;
    via_layer_ps1 += bbox_rect;
  }
  gtl::rectangle_data<frCoord> layer1_rect;
  gtl::extents(layer1_rect, via_layer_ps1);
  const bool is_layer1_horz = (gtl::xh(layer1_rect) - gtl::xl(layer1_rect))
                              > (gtl::yh(layer1_rect) - gtl::yl(layer1_rect));
  const frCoord layer1_width
      = std::min((gtl::xh(layer1_rect) - gtl::xl(layer1_rect)),
                 (gtl::yh(layer1_rect) - gtl::yl(layer1_rect)));

  const auto layer1_num = via_def->getLayer1Num();
  const auto dir1 = getDesign()->getTech()->getLayer(layer1_num)->getDir();

  const bool is_not_lower_align
      = (is_layer1_horz && (dir1 == dbTechLayerDir::VERTICAL))
        || (!is_layer1_horz && (dir1 == dbTechLayerDir::HORIZONTAL));

  gtl::polygon_90_set_data<frCoord> via_layer_PS2;
  for (auto& fig : via_def->getLayer2Figs()) {
    const Rect bbox = fig->getBBox();
    const gtl::rectangle_data<frCoord> bbox_rect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using boost::polygon::operators::operator+=;
    via_layer_PS2 += bbox_rect;
  }
  gtl::rectangle_data<frCoord> layer2_rect;
  gtl::extents(layer2_rect, via_layer_PS2);
  const bool is_layer2_horz = (gtl::xh(layer2_rect) - gtl::xl(layer2_rect))
                              > (gtl::yh(layer2_rect) - gtl::yl(layer2_rect));
  const frCoord layer2_width
      = std::min((gtl::xh(layer2_rect) - gtl::xl(layer2_rect)),
                 (gtl::yh(layer2_rect) - gtl::yl(layer2_rect)));

  const auto layer2_num = via_def->getLayer2Num();
  const auto dir2 = getDesign()->getTech()->getLayer(layer2_num)->getDir();

  const bool is_not_upper_align
      = (is_layer2_horz && (dir2 == dbTechLayerDir::VERTICAL))
        || (!is_layer2_horz && (dir2 == dbTechLayerDir::HORIZONTAL));

  const frCoord layer1_area = gtl::area(via_layer_ps1);
  const frCoord layer2_area = gtl::area(via_layer_PS2);

  return std::make_tuple(is_not_default_via,
                         layer1_width,
                         layer2_width,
                         is_not_upper_align,
                         layer2_area,
                         layer1_area,
                         is_not_lower_align);
}

void FlexPA::initTrackCoords()
{
  const int num_layers = getDesign()->getTech()->getLayers().size();
  const frCoord manu_grid = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  track_coords_.clear();
  track_coords_.resize(num_layers);
  for (auto& track_pattern : design_->getTopBlock()->getTrackPatterns()) {
    const auto layer_num = track_pattern->getLayerNum();
    const auto is_vert_layer
        = (design_->getTech()->getLayer(layer_num)->getDir()
           == dbTechLayerDir::VERTICAL);
    const auto is_vert_track
        = track_pattern->isHorizontal();  // true = vertical track
    if ((!is_vert_layer && !is_vert_track)
        || (is_vert_layer && is_vert_track)) {
      frCoord curr_coord = track_pattern->getStartCoord();
      for (int i = 0; i < (int) track_pattern->getNumTracks(); i++) {
        track_coords_[layer_num][curr_coord] = frAccessPointEnum::OnGrid;
        curr_coord += track_pattern->getTrackSpacing();
      }
    }
  }

  // half coords
  std::vector<std::vector<frCoord>> half_track_coords(num_layers);
  for (int i = 0; i < num_layers; i++) {
    frCoord prev_full_coord = std::numeric_limits<frCoord>::max();

    for (auto& [curr_full_coord, cost] : track_coords_[i]) {
      if (curr_full_coord > prev_full_coord) {
        const frCoord curr_half_grid
            = (curr_full_coord + prev_full_coord) / 2 / manu_grid * manu_grid;
        if (curr_half_grid != curr_full_coord
            && curr_half_grid != prev_full_coord) {
          half_track_coords[i].push_back(curr_half_grid);
        }
      }
      prev_full_coord = curr_full_coord;
    }
    for (auto half_coord : half_track_coords[i]) {
      track_coords_[i][half_coord] = frAccessPointEnum::HalfGrid;
    }
  }
}

void FlexPA::initAllSkipInstTerm()
{
  const auto& unique = unique_insts_.getUnique();
#pragma omp parallel for schedule(dynamic)
  for (frInst* unique_inst : unique) {
    initSkipInstTerm(unique_inst);
  }
}

void FlexPA::initSkipInstTerm(frInst* unique_inst)
{
  for (auto& inst_term : unique_inst->getInstTerms()) {
    frMTerm* term = inst_term->getTerm();
    const UniqueInsts::InstSet* inst_class
        = unique_insts_.getClass(unique_inst);

#pragma omp critical
    skip_unique_inst_term_[{inst_class, term}] = false;

    // We have to be careful that the skip conditions are true not only of
    // the unique instance but also all the equivalent instances.
    bool skip = isSkipInstTermLocal(inst_term.get());
    if (skip) {
      for (frInst* inst : *inst_class) {
        frInstTerm* it = inst->getInstTerm(inst_term->getIndexInOwner());
        skip = isSkipInstTermLocal(it);
        if (!skip) {
          break;
        }
      }
    }
#pragma omp critical
    skip_unique_inst_term_.at({inst_class, term}) = skip;
  }
}

}  // namespace drt
