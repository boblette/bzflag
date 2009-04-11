/* bzflag
 * Copyright (c) 1993 - 2009 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// interface header
#include "TargetingUtils.h"

// system headers
#include <math.h>

// common headers
#include "Ray.h"

// local headers
#include "ShotStrategy.h"

// These routines are 2 dimensional

float TargetingUtils::normalizeAngle(float ang)
{
  if (ang < -1.0f * M_PI) ang += (float)(2.0 * M_PI);
  if (ang > 1.0f * M_PI) ang -= (float)(2.0 * M_PI);
  return ang;
}

void TargetingUtils::getUnitVector(const fvec3& src, const fvec3& target, fvec3& unitVector )
{
  unitVector[0] = target[0] - src[0];
  unitVector[1] = target[1] - src[1];
  unitVector[2] = 0.0f;

  float len = (float) sqrt(unitVector[0] * unitVector[0] +
			   unitVector[1] * unitVector[1]);

  if (len == 0.0f)
	  return;

  unitVector[0] /= len;
  unitVector[1] /= len;
}

void TargetingUtils::get3DUnitVector( const fvec3& src, const fvec3& target, fvec3& unitVector )
{
  unitVector[0] = target[0] - src[0];
  unitVector[1] = target[1] - src[1];
  unitVector[2] = target[2] - src[1];

  float len = (float) sqrt(unitVector[0] * unitVector[0] +
			   unitVector[1] * unitVector[1] +
			     unitVector[2] * unitVector[2]);

  if (len == 0.0f)
	  return;

  unitVector[0] /= len;
  unitVector[1] /= len;
  unitVector[2] /= len;
}

float TargetingUtils::getTargetDistance( const fvec3& src, const fvec3& target )
{
  float vec[2];

  vec[0] = target[0] - src[0];
  vec[1] = target[1] - src[1];

  return (float) sqrt(vec[0] * vec[0] +
		      vec[1] * vec[1]);
}

float TargetingUtils::getTargetAzimuth( const fvec3& src, const fvec3& target )
{
  return atan2f((target[1] - src[1]), (target[0] - src[0]));
}

float TargetingUtils::getTargetRotation( const float startAzimuth, float targetAzimuth )
{
  float targetRotation = targetAzimuth - startAzimuth;
  if (targetRotation < -1.0f * M_PI) targetRotation += (float)(2.0 * M_PI);
  if (targetRotation > 1.0f * M_PI) targetRotation -= (float)(2.0 * M_PI);

  return targetRotation;
}

float TargetingUtils::getTargetAngleDifference( const fvec3& src, float srcAzimuth, const fvec3& target )
{
  fvec3 targetUnitVector;
  fvec3 srcUnitVector;
  getUnitVector(src, target, targetUnitVector);

  srcUnitVector[0] = cosf(srcAzimuth);
  srcUnitVector[1] = sinf(srcAzimuth);
  srcUnitVector[2] = 0.0f;

  return acos( targetUnitVector[0]*srcUnitVector[0] + targetUnitVector[1]*srcUnitVector[1] );
}

bool TargetingUtils::isLocationObscured( const fvec3& src, const fvec3& target )
{
  fvec3 dir;
  getUnitVector(src, target, dir);

  Ray tankRay(src, dir);
  float targetDistance = getTargetDistance(src, target);
  const Obstacle *building = ShotStrategy::getFirstBuilding(tankRay, -0.5f, targetDistance);
  return building != NULL;
}

float TargetingUtils::getOpenDistance( const fvec3& src, const float azimuth )
{
  float t = MAXFLOAT; //Some constant?

  fvec3 dir(cosf(azimuth), sinf(azimuth), 0.0f);
  *((float *) &src[2]) += 0.1f; //Don't hit building because your sitting on one
  Ray tankRay(src, dir);
  *((float *) &src[2]) -= 0.1f;
  ShotStrategy::getFirstBuilding(tankRay, -0.5f, t);
  return t;
}

bool TargetingUtils::getFirstCollisionPoint( const fvec3& src, const fvec3& target, fvec3& collisionPt )
{
  float t = MAXFLOAT;
  fvec3 dir;
  get3DUnitVector(src, target, dir);

  Ray tankRay(src, dir);
  const Obstacle *building = ShotStrategy::getFirstBuilding(tankRay, 0.0f, t);
  if (building == NULL)
	  return false;

  collisionPt[0] = src[0] + dir[0] * t;
  collisionPt[1] = src[1] + dir[1] * t;
  collisionPt[2] = src[2] + dir[2] * t;
  return true;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
