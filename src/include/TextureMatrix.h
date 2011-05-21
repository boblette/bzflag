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

#ifndef _TEXTURE_MATRIX_H_
#define _TEXTURE_MATRIX_H_

#include "common.h"

/* system interface headers */
#include <string>
#include <vector>
#include <set>
#include <iostream>


class TextureMatrix {

    friend class TextureMatrixManager;

  public:
    TextureMatrix();
    ~TextureMatrix();

    void finalize();

    const float* getMatrix() const;
    bool setName(const std::string& name);

    // the static parameters
    void setStaticSpin(float rotation);
    void setStaticShift(float u, float v);
    void setStaticScale(float u, float v);
    void setStaticCenter(float u, float v);

    // the dynamic parameters
    void setDynamicSpin(float freq);
    void setDynamicShift(float uFreq, float vFreq);
    void setDynamicScale(float uFreq, float vFreq,
                         float uSize, float vSize);
    void setDynamicCenter(float u, float v);

    // dynamic parameter variables
    void setDynamicSpinVar(const std::string& var);
    void setDynamicShiftVar(const std::string& var);
    void setDynamicScaleVar(const std::string& var);

    void update(double time);
    void setMatrix(const float matrix[4][4]);

    const std::string& getName() const;

    int packSize() const;
    void* pack(void*) const;
    void* unpack(void*);

    void print(std::ostream& out, const std::string& indent) const;

  private:
    static void staticSpinCallback(const std::string& name, void* data);
    static void staticShiftCallback(const std::string& name, void* data);
    static void staticScaleCallback(const std::string& name, void* data);
    void spinCallback(const std::string& name);
    void shiftCallback(const std::string& name);
    void scaleCallback(const std::string& name);

  private:
    std::string name;
    // time invariant
    bool useStatic;
    float staticMatrix[3][2];
    float rotation;
    float uFixedShift, vFixedShift;
    float uFixedScale, vFixedScale;
    float uFixedCenter, vFixedCenter;
    // time varying
    bool useDynamic;
    float spinFreq;
    float uShiftFreq, vShiftFreq;
    float uScaleFreq, vScaleFreq;
    float uScale, vScale;
    float uCenter, vCenter;
    std::string spinVar;
    std::string shiftVar;
    std::string scaleVar;
    // the final result
    float matrix[4][4];
};

inline const float* TextureMatrix::getMatrix() const {
  return (const float*) matrix;
}


class TextureMatrixManager {
  public:
    TextureMatrixManager();
    ~TextureMatrixManager();

    void update();
    void clear();

    int addMatrix(TextureMatrix* matrix);
    int findMatrix(const std::string& name) const;
    const TextureMatrix* getMatrix(int id) const;

    void getVariables(std::set<std::string>& vars) const;

    int packSize() const;
    void* pack(void*) const;
    void* unpack(void*);

    void print(std::ostream& out, const std::string& indent) const;

  private:
    std::vector<TextureMatrix*> matrices;
};

extern TextureMatrixManager TEXMATRIXMGR;

#endif //_TEXTURE_MATRIX_H_

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8