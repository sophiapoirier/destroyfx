/*---------------------------------------------------------------
Copyright (C) 2009-2025  Tom Murphy 7

This file is part of Firefly.

Firefly is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Firefly is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Firefly.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
---------------------------------------------------------------*/

#ifndef DFX_FIREFLY_EDITOR_H
#define DFX_FIREFLY_EDITOR_H

#include "dfxgui.h"
#include "multikick.hpp"
#include "vstchunk.h"
#include "indexbitmap.hpp"

#define FIRST_RESOURCE 128

const CColor kGreenTextCColor = {75, 151, 71, 0};

//--------------------------------------------------------------------------
class FireflyEditor : public AEffGUIEditor, public CControlListener {
public:
  FireflyEditor(AudioEffect *effect);
  virtual ~FireflyEditor();

protected:

  virtual long getRect(ERect **rect);
  virtual long open(void *ptr);
  virtual void close();

  virtual void setParameter(long index, float value);
  virtual void valueChanged(CDrawContext* context, CControl* control);

  virtual void idle();

private:

  /* ---graphics--- */
  CBitmap *g_background;

  VstChunk *chunk;
  bool setGlowing(long index, bool glow = true);
};

#endif
