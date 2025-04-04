/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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

#include "dbSdcNetwork.hh"

#include <string>

#include "sta/ParseBus.hh"
#include "sta/PatternMatch.hh"

namespace sta {

static string escapeDividers(const char* token, const Network* network);
static string escapeBrackets(const char* token, const Network* network);

dbSdcNetwork::dbSdcNetwork(Network* network) : SdcNetwork(network)
{
}

// Override SdcNetwork to NetworkNameAdapter.
Instance* dbSdcNetwork::findInstance(const char* path_name) const
{
  Instance* inst = network_->findInstance(path_name);
  if (inst == nullptr) {
    inst = network_->findInstance(escapeDividers(path_name, this).c_str());
  }
  if (inst == nullptr) {
    inst = network_->findInstance(escapeBrackets(path_name, this).c_str());
  }
  return inst;
}

InstanceSeq dbSdcNetwork::findInstancesMatching(
    const Instance*,
    const PatternMatch* pattern) const
{
  InstanceSeq insts;
  if (pattern->hasWildcards()) {
    findInstancesMatching1(pattern, insts);
  } else {
    Instance* inst = findInstance(pattern->pattern());
    if (inst) {
      insts.push_back(inst);
    } else {
      // Look for a match with path dividers escaped.
      std::string escaped
          = escapeChars(pattern->pattern(), divider_, '\0', escape_);
      inst = findInstance(escaped.c_str());
      if (inst) {
        insts.push_back(inst);
      } else {
        // Malo
        findInstancesMatching1(pattern, insts);
      }
    }
  }

  return insts;
}

void dbSdcNetwork::findInstancesMatching1(const PatternMatch* pattern,
                                          InstanceSeq& insts) const
{
  std::unique_ptr<InstanceChildIterator> child_iter{
      childIterator(topInstance())};
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    if (pattern->match(staToSdc(name(child)))) {
      insts.push_back(child);
    }
  }
}

NetSeq dbSdcNetwork::findNetsMatching(const Instance*,
                                      const PatternMatch* pattern) const
{
  NetSeq nets;
  if (pattern->hasWildcards()) {
    findNetsMatching1(pattern, nets);
  } else {
    Net* net = findNet(pattern->pattern());
    if (net) {
      nets.push_back(net);
    } else {
      // Look for a match with path dividers escaped.
      std::string escaped
          = escapeChars(pattern->pattern(), divider_, '\0', escape_);
      net = findNet(escaped.c_str());
      if (net) {
        nets.push_back(net);
      } else {
        findNetsMatching1(pattern, nets);
      }
    }
  }
  return nets;
}

void dbSdcNetwork::findNetsMatching1(const PatternMatch* pattern,
                                     NetSeq& nets) const
{
  std::unique_ptr<NetIterator> net_iter{netIterator(topInstance())};
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    if (pattern->match(staToSdc(name(net)))) {
      nets.push_back(net);
    }
  }
}

PinSeq dbSdcNetwork::findPinsMatching(const Instance* instance,
                                      const PatternMatch* pattern) const
{
  PinSeq pins;
  if (stringEq(pattern->pattern(), "*")) {
    // Pattern of '*' matches all child instance pins.
    std::unique_ptr<InstanceChildIterator> child_iter{childIterator(instance)};
    while (child_iter->hasNext()) {
      Instance* child = child_iter->next();
      std::unique_ptr<InstancePinIterator> pin_iter{pinIterator(child)};
      while (pin_iter->hasNext()) {
        Pin* pin = pin_iter->next();
        pins.push_back(pin);
      }
    }
  } else {
    char *inst_path, *port_name;
    pathNameLast(pattern->pattern(), inst_path, port_name);
    if (port_name) {
      PatternMatch inst_pattern(inst_path, pattern);
      InstanceSeq insts = findInstancesMatching(nullptr, &inst_pattern);
      PatternMatch port_pattern(port_name, pattern);
      for (auto inst : insts) {
        findMatchingPins(inst, &port_pattern, pins);
      }
    }
    stringDelete(inst_path);
    stringDelete(port_name);
  }

  return pins;
}

void dbSdcNetwork::findMatchingPins(const Instance* instance,
                                    const PatternMatch* port_pattern,
                                    PinSeq& pins) const
{
  if (instance != network_->topInstance()) {
    Cell* cell = network_->cell(instance);
    std::unique_ptr<CellPortIterator> port_iter{network_->portIterator(cell)};
    while (port_iter->hasNext()) {
      Port* port = port_iter->next();
      const char* port_name = network_->name(port);
      if (network_->hasMembers(port)) {
        bool bus_matches
            = port_pattern->match(port_name)
              || port_pattern->match(escapeDividers(port_name, network_));
        std::unique_ptr<PortMemberIterator> member_iter{
            network_->memberIterator(port)};
        while (member_iter->hasNext()) {
          Port* member_port = member_iter->next();
          Pin* pin = network_->findPin(instance, member_port);
          if (pin) {
            if (bus_matches) {
              pins.push_back(pin);
            } else {
              const char* member_name = network_->name(member_port);
              if (port_pattern->match(member_name)
                  || port_pattern->match(
                      escapeDividers(member_name, network_))) {
                pins.push_back(pin);
              }
            }
          }
        }
      } else if (port_pattern->match(port_name)
                 || port_pattern->match(escapeDividers(port_name, network_))) {
        Pin* pin = network_->findPin(instance, port);
        if (pin) {
          pins.push_back(pin);
        }
      }
    }
  }
}

Pin* dbSdcNetwork::findPin(const char* path_name) const
{
  char *inst_path, *port_name;
  pathNameLast(path_name, inst_path, port_name);
  Pin* pin = nullptr;
  if (inst_path) {
    Instance* inst = findInstance(inst_path);
    if (inst) {
      pin = findPin(inst, port_name);
    } else {
      pin = nullptr;
    }
  } else {
    pin = findPin(topInstance(), path_name);
  }
  stringDelete(inst_path);
  stringDelete(port_name);
  return pin;
}

static string escapeDividers(const char* token, const Network* network)
{
  return escapeChars(
      token, network->pathDivider(), '\0', network->pathEscape());
}

static string escapeBrackets(const char* token, const Network* network)
{
  return escapeChars(token, '[', ']', network->pathEscape());
}

}  // namespace sta
