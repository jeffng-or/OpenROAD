/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utl/Logger.h"

namespace ord {
class OpenRoad;
}

namespace odb {
class dbDatabase;
class dbNet;
}  // namespace odb

namespace gui {
class Gui;
}

namespace stt {

using utl::Logger;

namespace flt {
class Flute;
}

struct Branch
{
  int x, y;  // starting point of the branch
  int n;     // index of neighbor
};

struct Tree
{
  int deg;                     // degree
  int length;                  // total wirelength
  std::vector<Branch> branch;  // array of tree branches

  void printTree(utl::Logger* logger) const;
  int branchCount() const { return branch.size(); }
};

class SteinerTreeBuilder
{
 public:
  SteinerTreeBuilder();
  ~SteinerTreeBuilder();

  void init(odb::dbDatabase* db, Logger* logger);

  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index,
                       float alpha);
  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index);
  Tree makeSteinerTree(odb::dbNet* net,
                       const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index);
  // API only for FastRoute, that requires the use of flutes in its
  // internal flute implementation
  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       const std::vector<int>& s,
                       int acc);

  bool checkTree(const Tree& tree) const;
  float getAlpha() const { return alpha_; }
  void setAlpha(float alpha);
  float getAlpha(const odb::dbNet* net) const;
  void setNetAlpha(const odb::dbNet* net, float alpha);
  void setMinFanoutAlpha(int min_fanout, float alpha);
  void setMinHPWLAlpha(int min_hpwl, float alpha);

  Tree flute(const std::vector<int>& x, const std::vector<int>& y, int acc);
  int wirelength(Tree t);
  void plottree(Tree t);
  Tree flutes(const std::vector<int>& xs,
              const std::vector<int>& ys,
              const std::vector<int>& s,
              int acc);

 private:
  int computeHPWL(odb::dbNet* net);

  const int flute_accuracy = 3;
  float alpha_;
  std::map<const odb::dbNet*, float> net_alpha_map_;
  std::pair<int, float> min_fanout_alpha_;
  std::pair<int, float> min_hpwl_alpha_;

  Logger* logger_;
  odb::dbDatabase* db_;
  std::unique_ptr<flt::Flute> flute_;
};

// Used by regressions.
void reportSteinerTree(const Tree& tree,
                       int drvr_x,
                       int drvr_y,
                       Logger* logger);
void reportSteinerTree(const stt::Tree& tree, Logger* logger);

}  // namespace stt
