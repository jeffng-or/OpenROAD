// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "lefiUnits.hpp"

#include <cstdlib>
#include <cstring>

#include "lefiDebug.hpp"
#include "lex.h"

BEGIN_LEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiUnits
// *****************************************************************************

lefiUnits::lefiUnits()
{
  Init();
}

void lefiUnits::Init()
{
  clear();
}

void lefiUnits::Destroy()
{
  clear();
}

lefiUnits::~lefiUnits()
{
  Destroy();
}

void lefiUnits::setDatabase(const char* name, double num)
{
  int len = strlen(name) + 1;
  databaseName_ = (char*) lefMalloc(len);
  strcpy(databaseName_, CASE(name));
  databaseNumber_ = num;
  hasDatabase_ = 1;
}

void lefiUnits::clear()
{
  if (databaseName_) {
    lefFree(databaseName_);
  }
  hasTime_ = 0;
  hasCapacitance_ = 0;
  hasResistance_ = 0;
  hasPower_ = 0;
  hasCurrent_ = 0;
  hasVoltage_ = 0;
  hasDatabase_ = 0;
  hasFrequency_ = 0;
  databaseName_ = nullptr;
}

void lefiUnits::setTime(double num)
{
  hasTime_ = 1;
  time_ = num;
}

void lefiUnits::setCapacitance(double num)
{
  hasCapacitance_ = 1;
  capacitance_ = num;
}

void lefiUnits::setResistance(double num)
{
  hasResistance_ = 1;
  resistance_ = num;
}

void lefiUnits::setPower(double num)
{
  hasPower_ = 1;
  power_ = num;
}

void lefiUnits::setCurrent(double num)
{
  hasCurrent_ = 1;
  current_ = num;
}

void lefiUnits::setVoltage(double num)
{
  hasVoltage_ = 1;
  voltage_ = num;
}

void lefiUnits::setFrequency(double num)
{
  hasFrequency_ = 1;
  frequency_ = num;
}

int lefiUnits::hasDatabase() const
{
  return hasDatabase_;
}

int lefiUnits::hasCapacitance() const
{
  return hasCapacitance_;
}

int lefiUnits::hasResistance() const
{
  return hasResistance_;
}

int lefiUnits::hasPower() const
{
  return hasPower_;
}

int lefiUnits::hasCurrent() const
{
  return hasCurrent_;
}

int lefiUnits::hasVoltage() const
{
  return hasVoltage_;
}

int lefiUnits::hasFrequency() const
{
  return hasFrequency_;
}

int lefiUnits::hasTime() const
{
  return hasTime_;
}

const char* lefiUnits::databaseName() const
{
  return databaseName_;
}

double lefiUnits::databaseNumber() const
{
  return databaseNumber_;
}

double lefiUnits::capacitance() const
{
  return capacitance_;
}

double lefiUnits::resistance() const
{
  return resistance_;
}

double lefiUnits::power() const
{
  return power_;
}

double lefiUnits::current() const
{
  return current_;
}

double lefiUnits::time() const
{
  return time_;
}

double lefiUnits::voltage() const
{
  return voltage_;
}

double lefiUnits::frequency() const
{
  return frequency_;
}

void lefiUnits::print(FILE* f) const
{
  fprintf(f, "Units:\n");
  if (hasTime()) {
    fprintf(f, "  %g nanoseconds\n", time());
  }
  if (hasCapacitance()) {
    fprintf(f, "  %g picofarads\n", capacitance());
  }
  if (hasResistance()) {
    fprintf(f, "  %g ohms\n", resistance());
  }
  if (hasPower()) {
    fprintf(f, "  %g milliwatts\n", power());
  }
  if (hasCurrent()) {
    fprintf(f, "  %g milliamps\n", current());
  }
  if (hasVoltage()) {
    fprintf(f, "  %g volts\n", voltage());
  }
  if (hasFrequency()) {
    fprintf(f, "  %g frequency\n", frequency());
  }
  if (hasDatabase()) {
    fprintf(f, "  %s %g\n", databaseName(), databaseNumber());
  }
}

END_LEF_PARSER_NAMESPACE
