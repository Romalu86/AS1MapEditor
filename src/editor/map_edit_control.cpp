#include "mapedit/runtime.hpp"

namespace {
constexpr float kNoDragCoordinate = -999.0f;
constexpr float kSelectionDragThreshold = 10.0f;
constexpr float kFillDragThreshold = 20.0f;

float g_dragBeginX = kNoDragCoordinate; // original 0x004BCBE4
float g_dragBeginY = kNoDragCoordinate; // original 0x004BCBE8
float g_tacticInsertX = 0.0f;           // original 0x004BCBEC
float g_tacticInsertY = 0.0f;           // original 0x004BCBF0
float g_dragZ = 0.0f;                   // original 0x004CE360
float g_tacticInsertZ = 0.0f;           // original 0x004CE364
float g_rotateBeginX = 0.0f;            // original 0x004CE344
float g_rotateBeginY = 0.0f;            // original 0x004CE238
unsigned long g_lastAirBrushTime = 0;   // original 0x004CE240
int g_railFinished = 0;                 // original 0x004CE348

void ResetDrag()
{
    g_dragBeginX = kNoDragCoordinate;
    g_dragBeginY = kNoDragCoordinate;
}

float MinFloat(float a, float b) { return a < b ? a : b; }
float AbsDiff(float a, float b) { return fabsf(a - b); }
}

//
// This is the central editor input controller. The code is intentionally kept
// in the same four-mode structure as the retail implementation rather than
// flattened into a generic tool framework: tactic editing, rails, regions and
// ordinary sprite placement have materially different undo/mouse semantics.
void MAP_EDIT::Control(INPUT* input)
{
    static POLYGON polygon;
    polygon.Draw(GRAPH::GREEN);

    if (optTacticMode) {
        SPRITE* unitUnderCursor = GetSpriteScr(spriteType << 20, input->mouseX, input->mouseY);

        if (!selectedSprites.No() && input->ctrl && input->vkKey == 0x2D) {
            g_tacticInsertX = input->mouseX;
            g_tacticInsertY = input->mouseY;
            g_tacticInsertZ = Mouse->Z();
        }

        if (selectedSprites.No() && !input->ctrl) {
            if ((input->key >> 8) == 0x2E) {
                undo.Begin();
                for (int i = 0; i < selectedSprites.No(); ++i)
                    undo.AddRemove(*selectedSprites[i]);
                undo.End();
            }

            if (input->key == 0x72) {
                for (int i = 0; i < selectedSprites.No(); ++i)
                    (*selectedSprites[i])->AddAction(0x26, 0, 0, 0);
            } else if (input->key == 0x6C) {
                for (int i = 0; i < selectedSprites.No(); ++i)
                    (*selectedSprites[i])->AddAction(0x0C, 0, 0, 0);
            }

            SPRITE* firstSelected = *selectedSprites[0];
            if ((unitUnderCursor && firstSelected->IsEnemy(unitUnderCursor)) || input->shift) {
                Mouse->ChangeAnimation(5);
            } else if (firstSelected->CanPlace(input->mouseX, input->mouseY, firstSelected->Z())) {
                Mouse->ChangeAnimation(8);
            } else if (Mouse->Animation() != 2) {
                Mouse->ChangeAnimation(2);
            }

            if (Mouse->Animation() == 2 || Mouse->Animation() == 5) {
                if (firstSelected->AskCycleAction(0x21,
                                                  static_cast<int>(input->mouseX),
                                                  static_cast<int>(input->mouseY), 0)) {
                    Mouse->ChangeAnimation(9);
                }
            }
        } else {
            Mouse->ChangeAnimation(0);
        }

        if (input->lClick) {
            if (!input->ctrl)
                selectedSprites.Release();
            polygon.Release();
            polygon.AddLinedPoint(input->mouseX, input->mouseY);
            g_dragBeginX = input->mouseX;
            g_dragBeginY = input->mouseY;
            g_dragZ = Mouse->Z();
        }

        if (input->lDown) {
            if (input->shift)
                polygon.AddLinedPoint(input->mouseX, input->mouseY);
            else
                polygon.CreateBox(g_dragBeginX, g_dragBeginY, input->mouseX, input->mouseY);
        } else if (g_dragBeginX != kNoDragCoordinate) {
            if (polygon.NoPoint() &&
                (input->shift ||
                 AbsDiff(input->mouseX, g_dragBeginX) > kSelectionDragThreshold ||
                 AbsDiff(input->mouseY, g_dragBeginY) > kSelectionDragThreshold)) {
                polygon.Closed();
                FindSpritesInsidePolygon(spriteType << 20, &polygon, &selectedSprites);
            } else if (selectedSprites.InsertUnique(unitUnderCursor)) {
                selectedSprites.Delete(unitUnderCursor);
            }
            polygon.Release();
            ResetDrag();
        }

        if (input->rClick) {
            for (int i = 0; i < selectedSprites.No(); ++i) {
                SPRITE* selected = *selectedSprites[i];
                if (input->shift) {
                    selected->AddCycleAction(0x25,
                                             static_cast<int>(input->mouseX),
                                             static_cast<int>(input->mouseY), 0);
                } else if (unitUnderCursor && unitUnderCursor != *selectedSprites[0]) {
                    selected->AddAction(0x20, reinterpret_cast<int>(unitUnderCursor), 0, 0);
                } else {
                    const float groundZ = GetGroundZ(input->mouseX, input->mouseY);
                    selected->AddCycleAction(0x21,
                                             static_cast<int>(input->mouseX),
                                             static_cast<int>(input->mouseY + groundZ),
                                             static_cast<int>(groundZ));
                }
            }
        }

        for (int i = 0; i < selectedSprites.No(); ++i) {
            (*selectedSprites[i])->DrawRectangle();
            (*selectedSprites[i])->DrawActionStack();
        }

        if (spriteType == 0x40 && selectedSprites.No() &&
            (*selectedSprites.First())->IsSpriteClass(0x17)) {
            curRegion = static_cast<REGION*>(*selectedSprites.First());
        }
        return;
    }

    // Rail editing has its own press/release transaction. Right click commits
    // the multi-segment rail and removes the temporary selected rail sprites
    // from the pending undo batch, exactly as the original editor does.
    if (spriteType == 0x20) {
        if (input->lDown)
            Mouse->HardwareOn();
        else
            Mouse->HardwareOff();

        if (optDelete) {
            if (input->lClick) {
                undo.Begin();
                DeleteUnit(Mouse->X(), Mouse->Y(), spriteType);
                undo.End();
            }
            return;
        }

        if (input->lClick) {
            selectedSprites.Release();
            undo.Begin();
            g_railFinished = 0;
            g_dragBeginX = input->mouseX;
            g_dragBeginY = input->mouseY;
            g_dragZ = Mouse->Z();
        } else if (input->rClick) {
            for (int i = selectedSprites.No(); --i >= 0;)
                undo.DeleteLast();
            selectedSprites.DeleteAll();
            undo.End();
            g_railFinished = 1;
        } else if (input->lDown && !g_railFinished &&
                   (g_dragBeginX != input->mouseX || g_dragBeginY != input->mouseY)) {
            CreateNewRail();
        } else if (input->lUp) {
            if (g_dragBeginX == input->mouseX && g_dragBeginY == input->mouseY) {
                InsertUnit(Mouse->Vid(), Mouse->X(), Mouse->Y(), Mouse->Z(), Mouse->Direction());
            }
            undo.End();
        }
        return;
    }

    // Region mode drags a rectangle, creates a REGION at its centre and then
    // invokes the original REGION_PROPERTY modal dialog. A cancelled dialog
    // destroys the just-created sprite rather than recording it in undo.
    if (spriteType == 0x40) {
        if (optDelete) {
            if (input->lClick) {
                undo.Begin();
                DeleteUnit(Mouse->X(), Mouse->Y(), spriteType);
                undo.End();
            }
            return;
        }

        if (input->lClick) {
            g_dragBeginX = input->mouseX;
            g_dragBeginY = input->mouseY;
            g_dragZ = Mouse->Z();
            return;
        }

        if (input->lDown && g_dragBeginX != kNoDragCoordinate) {
            if (input->rClick) {
                ResetDrag();
                polygon.Release();
            } else {
                polygon.CreateBox(g_dragBeginX, g_dragBeginY, input->mouseX, input->mouseY);
            }
            return;
        }

        if (g_dragBeginX != kNoDragCoordinate) {
            const float x = MinFloat(g_dragBeginX, input->mouseX);
            const float y = MinFloat(g_dragBeginY, input->mouseY);
            const float sizeX = AbsDiff(g_dragBeginX, input->mouseX);
            const float sizeY = AbsDiff(g_dragBeginY, input->mouseY);

            if (Mouse->IsSpriteClass(0x17) && sizeX != 0.0f && sizeY != 0.0f) {
                undo.Begin();
                curRegion = static_cast<REGION*>(CreateSprite(
                    Mouse->Vid(), x + sizeX / 2.0f, y + sizeY / 2.0f + g_dragZ,
                    g_dragZ, ANGLE(static_cast<uint8_t>(0)), 0));

                if (curRegion) {
                    curRegion->SetSize(sizeX, sizeY);
                    const STRING dialogName("REGION_PROPERTY");
                    if (!CallDialogBox(&dialogName, AppRegionProperty)) {
                        curRegion->ScalarDeletingDestructor(1);
                        curRegion = 0;
                    } else {
                        undo.AddInsert(curRegion);
                    }
                }
                undo.End();
            }

            polygon.Release();
            ResetDrag();
        }
        return;
    }

    // Ordinary sprite placement. While dragging a box/diamond the original
    // editor undoes the previous preview and recreates it as one fresh undo
    // transaction. Airbrush mode bypasses that preview path and rate-limits
    // individual inserts/deletes by optAirBrushDensity.
    if (input->lClick) {
        g_dragBeginX = Mouse->X();
        g_dragBeginY = Mouse->Y();
        g_dragZ = Mouse->Z();
        undo.Begin();
    }

    bool updatedAreaPreview = false;
    if (input->lDown && !optAirBrush && g_dragBeginX != kNoDragCoordinate &&
        (AbsDiff(g_dragBeginX, Mouse->X()) > kFillDragThreshold ||
         AbsDiff(g_dragBeginY, Mouse->Y()) > kFillDragThreshold)) {
        if (input->rClick) {
            ResetDrag();
            undo.End();
            undo.Undo();
            undo.Reset();
        } else {
            undo.End();
            undo.Undo();
            undo.Reset();
            undo.Begin();
            if (input->shift)
                FillRomb(g_dragBeginX, g_dragBeginY, input->mouseX, input->mouseY);
            else
                FillBox(g_dragBeginX, g_dragBeginY, input->mouseX, input->mouseY);
        }
        updatedAreaPreview = true;
    }

    if (!updatedAreaPreview) {
        bool applySinglePoint = input->lClick != 0;
        if (!applySinglePoint && input->lDown && optAirBrush) {
            // The retail code performs the division unconditionally once this
            // branch is entered; do not silently invent a zero-density guard.
            applySinglePoint = CurrentTime - g_lastAirBrushTime > 1000u / optAirBrushDensity;
        }

        if (applySinglePoint) {
            g_lastAirBrushTime = CurrentTime;
            if (!optDelete)
                InsertUnit(Mouse->Vid(), Mouse->X(), Mouse->Y(), Mouse->Z(), Mouse->Direction());
            else
                DeleteUnit(Mouse->X(), Mouse->Y(), spriteType);
        }
    }

    if (input->lUp) {
        undo.End();
        ResetDrag();
    }

    if (Mouse->Vid()->m_noDirections > 1) {
        if (input->rClick) {
            g_rotateBeginX = Mouse->X();
            g_rotateBeginY = Mouse->Y();
        }
        if (input->rDown) {
            ANGLE base = Mouse->DirectionTo(g_rotateBeginX, g_rotateBeginY);
            ANGLE offset(Mouse->Vid()->m_editorDirectionOffset);
            Mouse->ChangeDirection(base + &offset);
        }
    }
}
