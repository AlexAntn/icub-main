/*
 * settings.cpp
 */

/*
 * Copyright (C) 2009 RobotCub Consortium
 * Author: Alessandro Scalzo alessandro.scalzo@iit.it
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 * Based on:
 *
 *   Qavimator
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
 *   Released under the terms of the GNU GPL v2.0.
 */

#include <iostream>

#include "settings.h"

static bool m_fog=true;
static int  m_floorTranslucency=25;

static bool m_easeIn=false;
static bool m_easeOut=false;

Settings::Settings()
{
  // should never be accessed
}

Settings::~Settings()
{
  // should never be accessed
}

void Settings::setFog(bool on)                 { m_fog=on; }
bool Settings::fog()                           { return m_fog; }

void Settings::setFloorTranslucency(int value) { m_floorTranslucency=value; }
int  Settings::floorTranslucency()             { return m_floorTranslucency; }

void Settings::setEaseIn(bool on)              { m_easeIn=on; }
bool Settings::easeIn()                        { return m_easeIn; }
void Settings::setEaseOut(bool on)             { m_easeOut=on; }
bool Settings::easeOut()                       { return m_easeOut; }
