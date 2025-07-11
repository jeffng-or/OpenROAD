// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "CtsObserver.h"
#include "Util.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "utl/Logger.h"

namespace stt {
class SteinerTreeBuilder;
}
namespace cts {

class CtsOptions : public odb::dbBlockCallBackObj
{
 public:
  enum class MasterType
  {
    DUMMY,
    TREE
  };
  using MasterCount = std::map<odb::dbMaster*, int>;

  CtsOptions(utl::Logger* logger, stt::SteinerTreeBuilder* sttBuildder)
      : logger_(logger), sttBuilder_(sttBuildder)
  {
  }

  void setClockNets(const std::string& clockNets) { clockNets_ = clockNets; }
  std::string getClockNets() const { return clockNets_; }
  void setRootBuffer(const std::string& buffer) { rootBuffer_ = buffer; }
  std::string getRootBuffer() const { return rootBuffer_; }
  void setBufferList(const std::vector<std::string>& buffers)
  {
    bufferList_ = buffers;
  }
  std::vector<std::string> getBufferList() const { return bufferList_; }
  void setDbUnits(int units) { dbUnits_ = units; }
  int getDbUnits() const { return dbUnits_; }
  void setWireSegmentUnit(unsigned wireSegmentUnit)
  {
    wireSegmentUnit_ = wireSegmentUnit;
  }
  unsigned getWireSegmentUnit() const { return wireSegmentUnit_; }
  void setPlotSolution(bool plot) { plotSolution_ = plot; }
  bool getPlotSolution() const { return plotSolution_; }

  void setObserver(std::unique_ptr<CtsObserver> observer)
  {
    observer_ = std::move(observer);
  }
  CtsObserver* getObserver() const { return observer_.get(); }

  void setSinkClustering(bool enable) { sinkClusteringEnable_ = enable; }
  bool getSinkClustering() const { return sinkClusteringEnable_; }
  void setSinkClusteringUseMaxCap(bool useMaxCap)
  {
    sinkClusteringUseMaxCap_ = useMaxCap;
  }
  bool getSinkClusteringUseMaxCap() const { return sinkClusteringUseMaxCap_; }
  void setNumMaxLeafSinks(unsigned numSinks) { numMaxLeafSinks_ = numSinks; }
  unsigned getNumMaxLeafSinks() const { return numMaxLeafSinks_; }
  void setMaxSlew(unsigned slew) { maxSlew_ = slew; }
  unsigned getMaxSlew() const { return maxSlew_; }
  void setMaxCharSlew(double slew) { maxCharSlew_ = slew; }
  double getMaxCharSlew() const { return maxCharSlew_; }
  void setMaxCharCap(double cap) { maxCharCap_ = cap; }
  double getMaxCharCap() const { return maxCharCap_; }
  void setCharWirelengthIterations(unsigned wirelengthIterations)
  {
    charWirelengthIterations_ = wirelengthIterations;
  }
  unsigned getCharWirelengthIterations() const
  {
    return charWirelengthIterations_;
  }
  void setCapSteps(int steps) { capSteps_ = steps; }
  int getCapSteps() const { return capSteps_; }
  void setSlewSteps(int steps) { slewSteps_ = steps; }
  int getSlewSteps() const { return slewSteps_; }
  void setClockTreeMaxDepth(unsigned depth) { clockTreeMaxDepth_ = depth; }
  unsigned getClockTreeMaxDepth() const { return clockTreeMaxDepth_; }
  void setEnableFakeLutEntries(bool enable) { enableFakeLutEntries_ = enable; }
  unsigned isFakeLutEntriesEnabled() const { return enableFakeLutEntries_; }
  void setForceBuffersOnLeafLevel(bool force)
  {
    forceBuffersOnLeafLevel_ = force;
  }
  bool forceBuffersOnLeafLevel() const { return forceBuffersOnLeafLevel_; }
  void setBufDistRatio(double ratio) { bufDistRatio_ = ratio; }
  double getBufDistRatio() { return bufDistRatio_; }
  void setClockNetsObjs(const std::vector<odb::dbNet*>& nets)
  {
    clockNetsObjs_ = nets;
  }
  std::vector<odb::dbNet*> getClockNetsObjs() const { return clockNetsObjs_; }
  void setMetricsFile(const std::string& metricFile)
  {
    metricFile_ = metricFile;
  }
  std::string getMetricsFile() const { return metricFile_; }
  void setNumClockRoots(unsigned roots) { clockRoots_ = roots; }
  int getNumClockRoots() const { return clockRoots_; }
  void setNumClockSubnets(int nets) { clockSubnets_ = nets; }
  int getNumClockSubnets() const { return clockSubnets_; }
  void setNumBuffersInserted(int buffers) { buffersInserted_ = buffers; }
  int getNumBuffersInserted() const { return buffersInserted_; }
  void setNumSinks(int sinks) { sinks_ = sinks; }
  int getNumSinks() const { return sinks_; }
  void setTreeBuffer(const std::string& buffer) { treeBuffer_ = buffer; }
  std::string getTreeBuffer() const { return treeBuffer_; }
  unsigned getClusteringPower() const { return clusteringPower_; }
  void setClusteringPower(unsigned power) { clusteringPower_ = power; }
  double getClusteringCapacity() const { return clusteringCapacity_; }
  void setClusteringCapacity(double capacity)
  {
    clusteringCapacity_ = capacity;
  }

  void setMaxFanout(unsigned maxFanout) { maxFanout_ = maxFanout; }
  unsigned getMaxFanout() const { return maxFanout_; }

  // BufferDistance is in DBU
  int32_t getBufferDistance() const
  {
    if (bufDistance_) {
      return *bufDistance_;
    }

    if (dbUnits_ == -1) {
      logger_->error(
          utl::CTS, 542, "Must provide a dbUnit conversion though setDbUnits.");
    }

    return 100 /*um*/ * dbUnits_;
  }
  void setBufferDistance(int32_t distance_dbu) { bufDistance_ = distance_dbu; }

  // VertexBufferDistance is in DBU
  int32_t getVertexBufferDistance() const
  {
    if (vertexBufDistance_) {
      return *vertexBufDistance_;
    }

    if (dbUnits_ == -1) {
      logger_->error(
          utl::CTS, 543, "Must provide a dbUnit conversion though setDbUnits.");
    }

    return 240 /*um*/ * dbUnits_;
  }
  void setVertexBufferDistance(int32_t distance_dbu)
  {
    vertexBufDistance_ = distance_dbu;
  }
  bool isVertexBuffersEnabled() const { return vertexBuffersEnable_; }
  void setVertexBuffersEnabled(bool enable) { vertexBuffersEnable_ = enable; }
  bool isSimpleSegmentEnabled() const { return simpleSegmentsEnable_; }
  void setSimpleSegmentsEnabled(bool enable) { simpleSegmentsEnable_ = enable; }
  double getMaxDiameter() const { return maxDiameter_; }
  void setMaxDiameter(double distance)
  {
    maxDiameter_ = distance;
    sinkClusteringUseMaxCap_ = false;
    maxDiameterSet_ = true;
  }
  bool isMaxDiameterSet() const { return maxDiameterSet_; }
  unsigned getSinkClusteringSize() const { return sinkClustersSize_; }
  void setSinkClusteringSize(unsigned size)
  {
    sinkClustersSize_ = size;
    sinkClusteringUseMaxCap_ = false;
    sinkClustersSizeSet_ = true;
  }
  bool isSinkClusteringSizeSet() const { return sinkClustersSizeSet_; }
  unsigned getSinkClusteringLevels() const { return sinkClusteringLevels_; }
  void setSinkClusteringLevels(unsigned levels)
  {
    sinkClusteringLevels_ = levels;
  }

  double getMacroMaxDiameter() const { return macroMaxDiameter_; }
  void setMacroMaxDiameter(double distance)
  {
    macroMaxDiameter_ = distance;
    macroMaxDiameterSet_ = true;
  }
  bool isMacroMaxDiameterSet() const { return macroMaxDiameterSet_; }
  unsigned getMacroSinkClusteringSize() const { return macroSinkClustersSize_; }
  void setMacroClusteringSize(unsigned size)
  {
    macroSinkClustersSize_ = size;
    macroSinkClustersSizeSet_ = true;
  }
  bool isMacroSinkClusteringSizeSet() const
  {
    return macroSinkClustersSizeSet_;
  }
  unsigned getNumStaticLayers() const { return numStaticLayers_; }
  void setBalanceLevels(bool balance) { balanceLevels_ = balance; }
  bool getBalanceLevels() const { return balanceLevels_; }
  void setNumStaticLayers(unsigned num) { numStaticLayers_ = num; }
  void setSinkBuffer(const std::string& buffer) { sinkBuffer_ = buffer; }
  void setSinkBufferInputCap(double cap) { sinkBufferInputCap_ = cap; }
  double getSinkBufferInputCap() const { return sinkBufferInputCap_; }
  std::string getSinkBuffer() const { return sinkBuffer_; }
  utl::Logger* getLogger() const { return logger_; }
  stt::SteinerTreeBuilder* getSttBuilder() const { return sttBuilder_; }
  void setObstructionAware(bool obs) { obsAware_ = obs; }
  bool getObstructionAware() const { return obsAware_; }
  void setApplyNDR(bool ndr) { applyNDR_ = ndr; }
  bool applyNDR() const { return applyNDR_; }
  void enableInsertionDelay(bool insDelay) { insertionDelay_ = insDelay; }
  bool insertionDelayEnabled() const { return insertionDelay_; }
  void setBufferListInferred(bool inferred) { bufferListInferred_ = inferred; }
  bool isBufferListInferred() const { return bufferListInferred_; }
  void setSinkBufferInferred(bool inferred) { sinkBufferInferred_ = inferred; }
  bool isSinkBufferInferred() const { return sinkBufferInferred_; }
  void setRootBufferInferred(bool inferred) { rootBufferInferred_ = inferred; }
  bool isRootBufferInferred() const { return rootBufferInferred_; }
  void setSinkBufferMaxCapDerate(float derate)
  {
    sinkBufferMaxCapDerate_ = derate;
    sinkBufferMaxCapDerateSet_ = true;
  }
  float getSinkBufferMaxCapDerate() const { return sinkBufferMaxCapDerate_; }
  bool isSinkBufferMaxCapDerateSet() const
  {
    return sinkBufferMaxCapDerateSet_;
  }
  void setDelayBufferDerate(float derate) { delayBufferDerate_ = derate; }
  float getDelayBufferDerate() const { return delayBufferDerate_; }
  void enableDummyLoad(bool dummyLoad) { dummyLoad_ = dummyLoad; }
  bool dummyLoadEnabled() const { return dummyLoad_; }
  std::string getDummyLoadPrefix() const { return dummyload_prefix_; }
  void setCtsLibrary(const char* name) { ctsLibrary_ = name; }
  const char* getCtsLibrary() { return ctsLibrary_.c_str(); }
  bool isCtsLibrarySet() { return !ctsLibrary_.empty(); }

  void recordBuffer(odb::dbMaster* master, MasterType type);
  const MasterCount& getBufferCount() const { return buffer_count_; }
  const MasterCount& getDummyCount() const { return dummy_count_; }

  MasterType getType(odb::dbInst* inst) const;

  // Callbacks
  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstCreate(odb::dbInst* inst, odb::dbRegion* region) override;

  void setRepairClockNets(bool value) { repairClockNets_ = value; }
  bool getRepairClockNets() { return repairClockNets_; }

 private:
  std::string clockNets_;
  std::string rootBuffer_;
  std::string sinkBuffer_;
  std::string treeBuffer_;
  std::string metricFile_;
  int dbUnits_ = -1;
  unsigned wireSegmentUnit_ = 0;
  bool plotSolution_ = false;
  bool sinkClusteringEnable_ = true;
  bool sinkClusteringUseMaxCap_ = true;
  bool simpleSegmentsEnable_ = false;
  bool vertexBuffersEnable_ = false;
  std::unique_ptr<CtsObserver> observer_;
  std::optional<int> vertexBufDistance_;
  std::optional<int> bufDistance_;
  double clusteringCapacity_ = 0.6;
  unsigned clusteringPower_ = 4;
  unsigned numMaxLeafSinks_ = 15;
  unsigned maxFanout_ = 0;
  unsigned maxSlew_ = 4;
  double maxCharSlew_ = 0;
  double maxCharCap_ = 0;
  double sinkBufferInputCap_ = 0;
  int capSteps_ = 20;
  int slewSteps_ = 7;
  unsigned charWirelengthIterations_ = 4;
  unsigned clockTreeMaxDepth_ = 100;
  bool enableFakeLutEntries_ = true;
  bool forceBuffersOnLeafLevel_ = true;
  double bufDistRatio_ = 0.1;
  int clockRoots_ = 0;
  int clockSubnets_ = 0;
  int buffersInserted_ = 0;
  int sinks_ = 0;
  double maxDiameter_ = 50;
  bool maxDiameterSet_ = false;
  unsigned sinkClustersSize_ = 20;
  bool sinkClustersSizeSet_ = false;
  double macroMaxDiameter_ = 50;
  bool macroMaxDiameterSet_ = false;
  unsigned macroSinkClustersSize_ = 4;
  bool macroSinkClustersSizeSet_ = true;
  bool balanceLevels_ = false;
  unsigned sinkClusteringLevels_ = 0;
  unsigned numStaticLayers_ = 0;
  std::vector<std::string> bufferList_;
  std::vector<odb::dbNet*> clockNetsObjs_;
  utl::Logger* logger_ = nullptr;
  stt::SteinerTreeBuilder* sttBuilder_ = nullptr;
  bool obsAware_ = true;
  bool applyNDR_ = false;
  bool insertionDelay_ = true;
  bool bufferListInferred_ = false;
  bool sinkBufferInferred_ = false;
  bool rootBufferInferred_ = false;
  bool sinkBufferMaxCapDerateSet_ = false;
  float sinkBufferMaxCapDerateDefault_ = 0.01;
  float sinkBufferMaxCapDerate_ = sinkBufferMaxCapDerateDefault_;
  bool dummyLoad_ = true;
  float delayBufferDerate_ = 1.0;  // no derate
  std::string ctsLibrary_;
  MasterCount buffer_count_;
  std::string dummyload_prefix_ = "clkload";
  MasterCount dummy_count_;
  bool repairClockNets_ = false;
};

}  // namespace cts
