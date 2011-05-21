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

#ifndef __CUSTOM_TEXUTRE_MATRIX_H__
#define __CUSTOM_TEXUTRE_MATRIX_H__

/* interface header */
#include "WorldFileObject.h"

/* common headers */
#include "TextureMatrix.h"

class CustomTextureMatrix : public WorldFileObject {
  public:
    CustomTextureMatrix(const char* texmatName);
    ~CustomTextureMatrix();
    virtual bool read(const char* cmd, std::istream& input);
    virtual void writeToManager() const;
    bool usesManager() { return true; }

  private:
    mutable TextureMatrix* texmat;
};

#endif  /* __CUSTOM_TEXUTRE_MATRIX_H__ */

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8 expandtab
