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

/*
 * ShotPath:
 *  Encapsulates the path a shot follows.  Most paths can
 *  be computed at the instant of firing (though they may
 *  terminate early because of a hit).  Some paths need
 *  to be updated continuously during flight.
 *
 * RemoteShotPath:
 *  A ShotPath acting as a proxy for a remote ShotPath.
 *  Created by a LocalPlayer on behalf of a RemotePlayer.
 */

#ifndef __SHOTPATH_H__
#define __SHOTPATH_H__

#include "common.h"

/* common interface headers */
#include "BzTime.h"
#include "Flag.h"
#include "ShotUpdate.h"
#include "GfxBlock.h"
#include "vectors.h"

/* local interface headers */
#include "ShotStrategy.h"
#include "SceneDatabase.h"


class ShotStrategy;
class ShotCollider;


class ShotPath {
  public:
    ShotPath(const FiringInfo&);
    virtual ~ShotPath();

    inline bool isExpiring() const { return expiring; }
    inline bool isExpired()  const { return expired;  }
    inline bool isReloaded() const {
      return (currentTime - startTime >= reloadTime);
    }
    inline const PlayerId& getPlayer() const {
      return firingInfo.shot.player;
    }
    inline const fvec3&    getPosition()    const { return firingInfo.shot.pos;  }
    inline const fvec3&    getVelocity()    const { return firingInfo.shot.vel;  }
    inline uint16_t        getShotId()      const { return firingInfo.shot.id;   }
    inline ShotType        getShotType()    const { return firingInfo.shotType;  }
    inline FlagType*       getFlagType()    const { return firingInfo.flagType;  }
    inline TeamColor       getTeam()        const { return firingInfo.shot.team; }
    inline float           getLifetime()    const { return firingInfo.lifetime;  }
    inline float           getReloadTime()  const { return reloadTime;  }
    inline BzTime          getStartTime()   const { return startTime;   }
    inline BzTime          getCurrentTime() const { return currentTime; }


    inline FiringInfo&       getFiringInfo()       { return firingInfo; }
    inline const FiringInfo& getFiringInfo() const { return firingInfo; }

    float checkHit(const ShotCollider&, fvec3& hitPos) const;
    void setExpiring();
    void setExpired();
    bool isStoppedByHit() const;
    void boostReloadTime(float dt);
    void setLocal(bool loc) {local = loc;}
    bool isLocal(void) {return local;}

    void addShot(SceneDatabase*, bool colorblind);

    void radarRender() const;

    virtual void update(float);

    //This function can be used to predict the position of the shot after a given time dt. Function returns true iff. the shot is still alive.
    bool predictPosition(float dt, fvec3& p) const;
    bool predictVelocity(float dt, fvec3& p) const;

    GfxBlock&       getGfxBlock()       { return gfxBlock; }
    const GfxBlock& getGfxBlock() const { return gfxBlock; }
    GfxBlock&       getRadarGfxBlock()       { return radarGfxBlock; }
    const GfxBlock& getRadarGfxBlock() const { return radarGfxBlock; }

  protected:
    void updateShot(float dt);

    inline ShotStrategy*       getStrategy()       { return strategy; }
    inline const ShotStrategy* getStrategy() const { return strategy; }

    friend class ShotStrategy;
    void setReloadTime(float);
    void setPosition(const fvec3&);
    void setVelocity(const fvec3&);

  private:
    ShotStrategy* strategy; // strategy for moving shell
    FiringInfo firingInfo; // shell information
    float reloadTime; // time to reload
    BzTime startTime; // time of firing
    BzTime currentTime; // current time
    bool expiring; // shot has almost terminated
    bool expired; // shot has terminated
    bool local; // shot is local, and must be ended localy, REMOVE ME WHEN THE SERVER DOES THIS
    GfxBlock gfxBlock;
    GfxBlock radarGfxBlock;
};


class LocalShotPath : public ShotPath {
  public:
    LocalShotPath(const FiringInfo&);
    ~LocalShotPath();

    void update(float dt);
};


class RemoteShotPath : public ShotPath {
  public:
    RemoteShotPath(const FiringInfo&);
    ~RemoteShotPath();

    void update(float dt);
    void update(const ShotUpdate& shot, void* msg);
};


#endif /* __SHOTPATH_H__ */

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8 expandtab expandtab
