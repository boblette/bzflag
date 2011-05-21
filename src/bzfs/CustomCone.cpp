/* bzflag
 * Copyright (c) 1993-2010 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "common.h"


/* interface header */
#include "CustomCone.h"

/* system headers */
#include <iostream>
#include <math.h>
#include <sstream>
#include <vector>

/* common implementation headers */
#include "ConeObstacle.h"
#include "PhysicsDriver.h"
#include "StateDatabase.h"
#include "ObstacleMgr.h"

/* bzfs implementation headers */
#include "ParseMaterial.h"


const char* CustomCone::sideNames[MaterialCount] = {
  "edge",
  "bottom",
  "startside",
  "endside"
};


CustomCone::CustomCone() {
  // default to a (radius=10, height=10) cylinder
  divisions = 16;
  size = fvec3(10.0f, 10.0f, 10.0f);
  texsize[0] = texsize[1] = -8.0f;
  angle = 360.0f;
  phydrv = -1;
  useNormals = true;
  smoothBounce = false;

  // setup the default textures
  materials[Edge].setTexture("boxwall");
  materials[Bottom].setTexture("roof");
  materials[StartFace].setTexture("wall");
  materials[EndFace].setTexture("wall");

  return;
}


CustomCone::~CustomCone() {
  return;
}


bool CustomCone::read(const char* cmd, std::istream& input) {
  bool materror;

  if (strcasecmp(cmd, "divisions") == 0) {
    if (!(input >> divisions)) {
      return false;
    }
  }
  else if (strcasecmp(cmd, "angle") == 0) {
    if (!(input >> angle)) {
      return false;
    }
  }
  else if (strcasecmp(cmd, "texsize") == 0) {
    if (!(input >> texsize[0] >> texsize[1])) {
      return false;
    }
  }
  else if (strcasecmp(cmd, "phydrv") == 0) {
    std::string drvname;
    if (!(input >> drvname)) {
      std::cout << "missing Physics Driver parameter" << std::endl;
      return false;
    }
    phydrv = PHYDRVMGR.findDriver(drvname);
    if ((phydrv == -1) && (drvname != "-1")) {
      std::cout << "couldn't find PhysicsDriver: " << drvname << std::endl;
    }
  }
  else if (strcasecmp(cmd, "smoothbounce") == 0) {
    smoothBounce = true;
  }
  else if (strcasecmp(cmd, "flatshading") == 0) {
    useNormals = false;
  }
  else if (parseMaterials(cmd, input, materials, MaterialCount, materror)) {
    if (materror) {
      return false;
    }
  }
  else if (parseMaterialsByName(cmd, input, materials, sideNames,
                                MaterialCount, materror)) {
    if (materror) {
      return false;
    }
  }
  else {
    return WorldFileObstacle::read(cmd, input);
  }

  return true;
}


void CustomCone::writeToGroupDef(GroupDefinition* groupdef) const {
  int i;
  const BzMaterial* mats[MaterialCount];
  for (i = 0; i < MaterialCount; i++) {
    mats[i] = MATERIALMGR.addMaterial(&materials[i]);
  }
  ConeObstacle* cone =
    new ConeObstacle(transform, pos, size, rotation, angle,
                     texsize, useNormals, divisions, mats, phydrv,
                     smoothBounce, driveThrough, shootThrough, ricochet);
  cone->setName(name.c_str());

  if (cone->isValid()) {
    groupdef->addObstacle(cone);
  }
  else {
    delete cone;
  }

  return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8 expandtab
