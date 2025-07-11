// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbWireGraph.h"

#include <map>
#include <string>
#include <vector>

#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"

namespace odb {

static void addObject(dbWireEncoder& encoder, dbObject* obj)
{
  dbObjectType type = obj->getObjectType();

  if (type == dbITermObj) {
    encoder.addITerm((dbITerm*) obj);
  } else if (type == dbBTermObj) {
    encoder.addBTerm((dbBTerm*) obj);
  }
}

dbWireGraph::dbWireGraph()
{
}

dbWireGraph::~dbWireGraph()
{
}

void dbWireGraph::clear()
{
  node_iterator itr;

  for (itr = begin_nodes(); itr != end_nodes();) {
    itr = deleteNode(itr);
  }

  _junction_map.clear();
}

dbWireGraph::Node* dbWireGraph::createNode(int x, int y, dbTechLayer* l)
{
  Node* n = new Node(x, y, l);
  assert(n);
  _nodes.push_back(n);
  return n;
}

dbWireGraph::Via* dbWireGraph::createVia(Node* src,
                                         Node* tgt,
                                         dbVia* via,
                                         dbWireType::Value type,
                                         dbTechLayerRule* rule)
{
  assert(tgt->_in_edge == nullptr);
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");

  Via* v = new Via(via, type, rule);
  v->_src = src;
  v->_tgt = tgt;
  tgt->_in_edge = v;
  src->_out_edges.push_back(v);
  _edges.push_back(v);
  return v;
}

dbWireGraph::TechVia* dbWireGraph::createTechVia(Node* src,
                                                 Node* tgt,
                                                 dbTechVia* via,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule* rule)
{
  assert(tgt->_in_edge == nullptr);
  assert((src->_x == tgt->_x) && (src->_y == tgt->_y)
         && "via coordinates are skewed");

  TechVia* v = new TechVia(via, type, rule);
  v->_src = src;
  v->_tgt = tgt;
  tgt->_in_edge = v;
  src->_out_edges.push_back(v);
  _edges.push_back(v);
  return v;
}

dbWireGraph::Segment* dbWireGraph::createSegment(Node* src,
                                                 Node* tgt,
                                                 EndStyle src_style,
                                                 EndStyle tgt_style,
                                                 dbWireType::Value type,
                                                 dbTechLayerRule* rule)
{
  assert(tgt->_in_edge == nullptr);
  assert(src->_layer == tgt->_layer);
  assert((src->_x == tgt->_x || src->_y == tgt->_y) && "non-orthognal segment");

  Segment* s = new Segment(src_style, tgt_style, type, rule);

  s->_src = src;
  s->_tgt = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

dbWireGraph::Short* dbWireGraph::createShort(Node* src,
                                             Node* tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule* rule)
{
  assert(tgt->_in_edge == nullptr);

  Short* s = new Short(type, rule);
  s->_src = src;
  s->_tgt = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

dbWireGraph::VWire* dbWireGraph::createVWire(Node* src,
                                             Node* tgt,
                                             dbWireType::Value type,
                                             dbTechLayerRule* rule)
{
  assert(tgt->_in_edge == nullptr);

  VWire* s = new VWire(type, rule);
  s->_src = src;
  s->_tgt = tgt;
  tgt->_in_edge = s;
  src->_out_edges.push_back(s);
  _edges.push_back(s);
  return s;
}

void dbWireGraph::deleteNode(Node* n)
{
  Node::edge_iterator eitr;

  for (eitr = n->begin(); eitr != n->end();) {
    Edge* e = *eitr;
    eitr = n->_out_edges.remove(e);
    e->_tgt->_in_edge = nullptr;
    _edges.remove(e);
    delete e;
  }

  if (n->_in_edge) {
    n->_in_edge->_src->_out_edges.remove(n->_in_edge);
    _edges.remove(n->_in_edge);
  }

  _nodes.remove(n);
  delete n;
}

dbWireGraph::node_iterator dbWireGraph::deleteNode(node_iterator itr)
{
  Node* n = *itr;
  node_iterator next = ++itr;
  deleteNode(n);
  return next;
}

void dbWireGraph::deleteEdge(Edge* e)
{
  e->_src->_out_edges.remove(e);
  e->_tgt->_in_edge = nullptr;
  _edges.remove(e);
  delete e;
}

dbWireGraph::edge_iterator dbWireGraph::deleteEdge(edge_iterator itr)
{
  Edge* e = *itr;
  edge_iterator next = ++itr;
  deleteEdge(e);
  return next;
}

dbWireGraph::Edge* dbWireGraph::getEdge(uint shape_id)
{
  assert(shape_id < _junction_map.size());
  Node* n = _junction_map[shape_id];
  assert(n->_in_edge);
  return n->_in_edge;
}

void dbWireGraph::decode(dbWire* wire)
{
  clear();

  _dbWire* w = (_dbWire*) wire;
  _junction_map.resize(w->_opcodes.size(), nullptr);

  dbTechLayer* cur_layer = nullptr;
  dbTechLayerRule* cur_rule = nullptr;
  dbWireType::Value cur_type = dbWireType::NONE;
  Node* prev = nullptr;
  EndStyle prev_style;
  int jct_id = -1;
  int short_id = -1;
  int vwire_id = -1;

  dbWireDecoder decoder;
  decoder.begin(wire);
  bool done = false;

  while (!done) {
    switch (decoder.next()) {
      case dbWireDecoder::PATH: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::JUNCTION: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        jct_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::SHORT: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        short_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::VWIRE: {
        cur_layer = decoder.getLayer();
        cur_type = decoder.getWireType();
        vwire_id = decoder.getJunctionValue();
        prev = nullptr;
        prev_style.setExtended();
        break;
      }

      case dbWireDecoder::POINT: {
        int x, y;
        decoder.getPoint(x, y);

        // The decoder always outputs the point of the junction.
        if (jct_id != -1) {
          prev_style.setExtended();
          prev = _junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == nullptr) {
          prev_style.setExtended();
          prev = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = _junction_map[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = _junction_map[vwire_id];
            createVWire(s, prev, cur_type, cur_rule);
            vwire_id = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->xy(prev_x, prev_y);

          // And check for the colinear cancelation of a extension.
          if ((x == prev_x) && (y == prev_y)) {
            if (prev_style.getType() == EndStyle::VARIABLE) {
              prev_style.setExtended();
              _junction_map[decoder.getJunctionId()] = prev;
              break;
            }
          }

          Node* cur = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = cur;
          createSegment(prev, cur, prev_style, EndStyle(), cur_type, cur_rule);
          prev = cur;
          prev_style.setExtended();
        }

        break;
      }

      case dbWireDecoder::POINT_EXT: {
        int x, y, ext;
        decoder.getPoint(x, y, ext);

        // The decoder always outputs the point of the junction.
        if (jct_id != -1) {
          prev_style.setVariable(ext);
          prev = _junction_map[jct_id];
          jct_id = -1;
        }

        else if (prev == nullptr) {
          prev_style.setVariable(ext);
          prev = createNode(x, y, cur_layer);
          _junction_map[decoder.getJunctionId()] = prev;

          if (short_id != -1) {
            Node* s = _junction_map[short_id];
            createShort(s, prev, cur_type, cur_rule);
            short_id = -1;
          }

          if (vwire_id != -1) {
            Node* s = _junction_map[vwire_id];
            createVWire(s, prev, cur_type, cur_rule);
            vwire_id = -1;
          }
        }

        else {
          int prev_x, prev_y;
          prev->xy(prev_x, prev_y);

          // A colinear point with an extenstion begins a new path segment
          if ((x == prev_x) && (y == prev_y)) {
            prev_style.setVariable(ext);
            _junction_map[decoder.getJunctionId()] = prev;
          } else {
            EndStyle cur_style;
            cur_style.setVariable(ext);
            Node* cur = createNode(x, y, cur_layer);
            _junction_map[decoder.getJunctionId()] = cur;
            createSegment(prev, cur, prev_style, cur_style, cur_type, cur_rule);
            prev = cur;
            prev_style = cur_style;
          }
        }
        break;
      }

      case dbWireDecoder::VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur = createNode(x, y, cur_layer);
        _junction_map[decoder.getJunctionId()] = cur;
        createVia(prev, cur, decoder.getVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        cur_layer = decoder.getLayer();
        int x, y;
        prev->xy(x, y);
        Node* cur = createNode(x, y, cur_layer);
        _junction_map[decoder.getJunctionId()] = cur;
        createTechVia(prev, cur, decoder.getTechVia(), cur_type, cur_rule);
        prev = cur;
        break;
      }

      case dbWireDecoder::ITERM: {
        if (prev->_object) {
          assert(prev->_object == (dbObject*) decoder.getITerm());
        }
        prev->_object = (dbObject*) decoder.getITerm();
        break;
      }

      case dbWireDecoder::BTERM: {
        if (prev->_object) {
          assert(prev->_object == (dbObject*) decoder.getBTerm());
        }
        prev->_object = (dbObject*) decoder.getBTerm();
        break;
      }

      case dbWireDecoder::RECT:
        // ignored
        break;

      case dbWireDecoder::RULE: {
        cur_rule = decoder.getRule();
        break;
      }

      case dbWireDecoder::END_DECODE: {
        done = true;
        break;
      }
    }
  }
}

void dbWireGraph::encode(dbWire* wire)
{
  dbWireEncoder encoder;

  encoder.begin(wire);

  node_iterator itr;
  std::vector<Edge*> path;

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    Node* n = *itr;
    n->_jct_id = -1;
  }

  for (itr = _nodes.begin(); itr != _nodes.end(); ++itr) {
    Node* n = *itr;

    if (n->_in_edge == nullptr) {
      path.clear();
      encodePath(encoder, path, n, dbWireType::NONE, nullptr);
    }
  }

  encoder.end();
}

class EdgeCmp
{
 public:
  int operator()(dbWireGraph::Edge& lhs, dbWireGraph::Edge& rhs)
  {
    return lhs.type() < rhs.type();
  }
};

void dbWireGraph::encodePath(dbWireEncoder& encoder,
                             std::vector<Edge*>& path,
                             Node* src,
                             dbWireType::Value cur_type,
                             dbTechLayerRule* cur_rule)
{
  if (src->_out_edges.empty()) {
    encodePath(encoder, path);
    return;
  }

  // src->_out_edges.sort( EdgeCmp() );
  Node::edge_iterator itr;
  bool has_shorts_or_vwires = false;

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT)
        || (e->type() == Edge::VWIRE))  // do shorts/vwires on second pass
    {
      has_shorts_or_vwires = true;
      continue;
    }

    bool type_or_rule_changed = false;

    if (e->_non_default_rule != cur_rule) {
      cur_rule = e->_non_default_rule;
      type_or_rule_changed = true;
    }

    if (e->_wire_type != cur_type) {
      cur_type = e->_wire_type;
      type_or_rule_changed = true;
    }

    if (type_or_rule_changed && (!path.empty())) {
      encodePath(encoder, path);
    }

    path.push_back(e);
    encodePath(encoder, path, e->_tgt, cur_type, cur_rule);
  }

  // Handle the case where there was only a short branch(es) from the src node:
  if (!path.empty()) {
    encodePath(encoder, path);
  }

  if (!has_shorts_or_vwires) {
    return;
  }

  for (itr = src->begin(); itr != src->end(); ++itr) {
    Edge* e = *itr;

    if ((e->type() == Edge::SHORT) || (e->type() == Edge::VWIRE)) {
      std::vector<Edge*> new_path;
      new_path.push_back(e);
      encodePath(
          encoder, new_path, e->_tgt, e->_wire_type, e->_non_default_rule);
    }
  }
}

void dbWireGraph::encodePath(dbWireEncoder& encoder, std::vector<Edge*>& path)
{
  std::vector<Edge*>::iterator itr = path.begin();

  if (itr == path.end()) {
    return;
  }

  Edge* e = *itr;
  EndStyle prev_style;

  bool is_short_or_vwire_path = false;

  switch (e->_type) {
    case Edge::SEGMENT: {
      Segment* s = (Segment*) e;

      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == nullptr) {
          encoder.newPath(e->_src->_layer, e->_wire_type);
        } else {
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);
        }

        if (s->_src_style.getType() == EndStyle::EXTENDED) {
          e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
        } else {
          e->_src->_jct_id = encoder.addPoint(
              e->_src->_x, e->_src->_y, s->_src_style.getExt());
        }

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      } else {
        if (e->_non_default_rule == nullptr) {
          if (s->_src_style.getType() == EndStyle::EXTENDED) {
            encoder.newPath(e->_src->_jct_id, e->_wire_type);
          } else {
            encoder.newPathExt(
                e->_src->_jct_id, s->_src_style.getExt(), e->_wire_type);
          }
        } else {
          if (s->_src_style.getType() == EndStyle::EXTENDED) {
            encoder.newPath(
                e->_src->_jct_id, e->_wire_type, e->_non_default_rule);
          } else {
            encoder.newPathExt(e->_src->_jct_id,
                               s->_src_style.getExt(),
                               e->_wire_type,
                               e->_non_default_rule);
          }
        }

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      }

      if (s->_tgt_style.getType() == EndStyle::EXTENDED) {
        e->_tgt->_jct_id = encoder.addPoint(e->_tgt->_x, e->_tgt->_y);
      } else {
        e->_tgt->_jct_id = encoder.addPoint(
            e->_tgt->_x, e->_tgt->_y, s->_tgt_style.getExt());
      }

      if (e->_tgt->_object != nullptr) {
        addObject(encoder, e->_tgt->_object);
      }

      prev_style = s->_tgt_style;
      break;
    }

    case Edge::TECH_VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == nullptr) {
          encoder.newPath(e->_src->_layer, e->_wire_type);
        } else {
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);
        }

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      } else {
        if (e->_non_default_rule == nullptr) {
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        } else {
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);
        }

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      }

      TechVia* v = (TechVia*) e;
      e->_tgt->_jct_id = encoder.addTechVia(v->_via);

      if (e->_tgt->_object != nullptr) {
        addObject(encoder, e->_tgt->_object);
      }
      break;
    }

    case Edge::VIA: {
      if (e->_src->_jct_id == -1) {
        if (e->_non_default_rule == nullptr) {
          encoder.newPath(e->_src->_layer, e->_wire_type);
        } else {
          encoder.newPath(e->_src->_layer, e->_wire_type, e->_non_default_rule);
        }

        e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      } else {
        if (e->_non_default_rule == nullptr) {
          encoder.newPath(e->_src->_jct_id, e->_wire_type);
        } else {
          encoder.newPath(
              e->_src->_jct_id, e->_wire_type, e->_non_default_rule);
        }

        if (e->_src->_object != nullptr) {
          addObject(encoder, e->_src->_object);
        }
      }

      Via* v = (Via*) e;
      e->_tgt->_jct_id = encoder.addVia(v->_via);

      if (e->_tgt->_object != nullptr) {
        addObject(encoder, e->_tgt->_object);
      }
      break;
    }

    case Edge::SHORT: {
      assert(e->_src->_jct_id != -1);

      if (e->_non_default_rule == nullptr) {
        encoder.newPathShort(e->_src->_jct_id, e->_src->_layer, e->_wire_type);
      } else {
        encoder.newPathShort(e->_src->_jct_id,
                             e->_src->_layer,
                             e->_wire_type,
                             e->_non_default_rule);
      }

      is_short_or_vwire_path = true;
      break;
    }

    case Edge::VWIRE: {
      assert(e->_src->_jct_id != -1);

      if (e->_non_default_rule == nullptr) {
        encoder.newPathVirtualWire(
            e->_src->_jct_id, e->_src->_layer, e->_wire_type);
      } else {
        encoder.newPathVirtualWire(e->_src->_jct_id,
                                   e->_src->_layer,
                                   e->_wire_type,
                                   e->_non_default_rule);
      }

      is_short_or_vwire_path = true;
      break;
    }
  }

  for (++itr; itr != path.end(); ++itr) {
    e = *itr;

    switch (e->_type) {
      case Edge::SEGMENT: {
        Segment* s = (Segment*) e;

        if (is_short_or_vwire_path) {
          if (s->_src_style.getType() == EndStyle::EXTENDED) {
            e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
          } else {
            e->_src->_jct_id = encoder.addPoint(
                e->_src->_x, e->_src->_y, s->_src_style.getExt());
          }

          if (e->_src->_object != nullptr) {
            addObject(encoder, e->_src->_object);
          }
          is_short_or_vwire_path = false;
        }

        else if (prev_style.getType() != s->_src_style.getType()) {
          // reset: default ext
          if (prev_style.getType() == EndStyle::VARIABLE
              && s->_src_style.getType() == EndStyle::EXTENDED) {
            encoder.addPoint(e->_src->_x, e->_src->_y);

            // reset: variable ext
          } else if (prev_style.getType() == EndStyle::EXTENDED
                     && s->_src_style.getType() == EndStyle::VARIABLE) {
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style.getExt());

            // Reset: variable ext
          } else if (prev_style.getType() == EndStyle::VARIABLE
                     && s->_src_style.getType() == EndStyle::VARIABLE) {
            encoder.addPoint(e->_src->_x, e->_src->_y, s->_src_style.getExt());
          }
        }

        assert(e->_src->_jct_id != -1);

        if (s->_tgt_style.getType() == EndStyle::EXTENDED) {
          e->_tgt->_jct_id = encoder.addPoint(e->_tgt->_x, e->_tgt->_y);
        } else {
          e->_tgt->_jct_id = encoder.addPoint(
              e->_tgt->_x, e->_tgt->_y, s->_tgt_style.getExt());
        }

        if (e->_tgt->_object != nullptr) {
          addObject(encoder, e->_tgt->_object);
        }
        prev_style = s->_tgt_style;
        break;
      }

      case Edge::TECH_VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;

          if (e->_src->_object != nullptr) {
            addObject(encoder, e->_src->_object);
          }
        }

        assert(e->_src->_jct_id != -1);

        TechVia* v = (TechVia*) e;
        e->_tgt->_jct_id = encoder.addTechVia(v->_via);

        if (e->_tgt->_object != nullptr) {
          addObject(encoder, e->_tgt->_object);
        }
        break;
      }

      case Edge::VIA: {
        if (is_short_or_vwire_path) {
          e->_src->_jct_id = encoder.addPoint(e->_src->_x, e->_src->_y);
          is_short_or_vwire_path = false;

          if (e->_src->_object != nullptr) {
            addObject(encoder, e->_src->_object);
          }
        }

        assert(e->_src->_jct_id != -1);

        Via* v = (Via*) e;
        e->_tgt->_jct_id = encoder.addVia(v->_via);

        if (e->_tgt->_object != nullptr) {
          addObject(encoder, e->_tgt->_object);
        }
        break;
      }

      case Edge::SHORT:
      case Edge::VWIRE: {
        // there should never be a short edge in the middle of a path
        assert(0);
        break;
      }
    }
  }

  path.clear();
}

void dbWireGraph::dump(utl::Logger* logger)
{
  std::map<Node*, int> node2index;
  int index = 0;
  for (auto it = begin_nodes(); it != end_nodes(); ++it) {
    auto node = *it;
    node2index[node] = index;
    int x;
    int y;
    node->xy(x, y);
    std::string obj_name("-");
    if (dbObject* object = node->object()) {
      if (object->getObjectType() == dbITermObj) {
        obj_name = static_cast<dbITerm*>(object)->getName();
      } else if (object->getObjectType() == dbBTermObj) {
        obj_name = static_cast<dbBTerm*>(object)->getName();
      } else {
        obj_name = "<unknown>";
      }
    }
    logger->report("Node {:2}: ({}, {}) {} (obj {})",
                   index++,
                   x,
                   y,
                   node->layer()->getName(),
                   obj_name);
  }

  index = 0;
  for (auto it = begin_edges(); it != end_edges(); ++it) {
    auto edge = *it;
    std::string type;
    switch (edge->type()) {
      case Edge::SEGMENT:
        type = "SEGMENT";
        break;
      case Edge::TECH_VIA:
        type = "TECH_VIA";
        break;
      case Edge::VIA:
        type = "VIA";
        break;
      case Edge::SHORT:
        type = "SHORT";
        break;
      case Edge::VWIRE:
        type = "VWIRE";
        break;
    }
    logger->report("Edge {:2}: {:8} {:2} -> {:2}",
                   index++,
                   type,
                   node2index.at(edge->source()),
                   node2index.at(edge->target()));
  }
}

}  // namespace odb
