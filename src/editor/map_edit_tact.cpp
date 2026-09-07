#include "mapedit/runtime.hpp"

// Original data owner at 0x004CE354. It is set when delete mode is entered by
// keyboard and consumed when Ctrl is released, preserving the retail editor's
// momentary-delete behaviour.
int g_editorDeleteLatch = 0;

namespace {
constexpr unsigned int kToolbarEnableButton = 0x0401;
constexpr unsigned int kToolbarCheckButton  = 0x0402;

void SetToolbarState(HWND__* toolbar,unsigned int message,unsigned int command,int value)
{
    SendMessageA(toolbar,message,command,value);
}
}

//
// This is the main editor frame/tick. The structure intentionally follows the
// retail source-line groups derived from debug metadata: frame begin, cursor/snap,
// render and primitive ticks, minimap/input, tactical vs ordinary keyboard
// control, toolbar state and frame present.
int MAP_EDIT::Tact()
{
    if (StartTact())
        return 1;

    // Base MAP flag bit 3 is the original draw/active gate. When drawing is
    // disabled, or the renderer is paused, the editor blocks for a message.
    if (!(m_flags & 0x08u) || Graph->IsPaused()) {
        WaitMessage();
        return 0;
    }

    const int draw = (m_flags >> 3) & 1u;
    if (draw && Graph->PreTact())
        return 0;

    ++m_noTact;

    if (Graph->InViewPort(m_input.screenMouseX,m_input.screenMouseY) &&
        (optSnap || spriteType == 0x20) && !optTacticMode) {
        Mouse->ChangeCoor(m_input.mouseX,m_input.mouseY,Mouse->Z());
        ChangeMouseCoorWithSnap();
    }

    float z = (optGround0 ? Mouse->GetGroundZ() : 0.0f) + insertZ;
    if (spriteType == 0x20) {
        const float step = Mouse->Vid()->m_hitVerticalOffset;
        // Keep the original x87 arithmetic literally. There is no integer
        // conversion between the divide and multiply in the retail code.
        if (z >= 0.0f)
            z = ((z + step - 1.0f) / step) * step;
        else
            z = ((z - step + 1.0f) / step) * step;
    }
    Mouse->ChangeZCoor(z);

    SPRITE* spriteUnderCursor = GetSpriteScr(spriteType << 20,m_input.mouseX,m_input.mouseY);
    if (optTacticMode) {
        if (spriteUnderCursor) {
            GAMMA hoverGamma(-32,64,-32);
            spriteUnderCursor->SetGamma(&hoverGamma);
        }
        for (int i=0;i<selectedSprites.No();++i) {
            GAMMA selectedGamma(64,-32,-32);
            (*selectedSprites[i])->SetGamma(&selectedGamma);
        }
    }

    Graph->Tact(draw);

    if (optTacticMode) {
        if (spriteUnderCursor) {
            GAMMA normalGamma;
            spriteUnderCursor->SetGamma(&normalGamma);
        }
        for (int i=0;i<selectedSprites.No();++i) {
            GAMMA normalGamma;
            (*selectedSprites[i])->SetGamma(&normalGamma);
        }
    }

    // The original loop is 0..16 inclusive here (17 layers), even though MAP
    // owns 18 layer lists. Do not "fix" this to the storage count.
    for (int layer=0;layer<17;++layer) {
        int index=0;
        for (SPRITE* sprite=FirstSprite(layer,&index); sprite;
             sprite=NextSprite(layer,&index)) {
            sprite->PrimitiveTact();
        }
    }

    if (optDrawMap) {
        DRAW_MAP drawMap(Graph->ViewXMax()-120.0f,Graph->ViewYMax()-80.0f,120.0f,80.0f);
        drawMap.Draw();
        drawMap.Control(&m_input);
    }

    ControlShiftCoor();
    Control(&m_input);

    switch (m_input.key) {
    case 9:
        optDrawMap = (optDrawMap == 0);
        break;
    case 'O':
        m_flags ^= (1u << 15);
        m_flags ^= (1u << 11);
        break;
    case 'R':
        m_flags ^= (1u << 13);
        break;
    }

    if (optTacticMode) {
        if (m_input.key || m_input.mouseWheel) {
            for (int i=0;i<selectedSprites.No();++i) {
                SPRITE* sprite=*selectedSprites[i];

                if (m_input.shift) {
                    switch (m_input.key) {
                    case 0x2600: sprite->ChangeYCoor(sprite->Y()-1.0f); break;
                    case 0x2700: sprite->ChangeXCoor(sprite->X()+1.0f); break;
                    case 0x2800: sprite->ChangeYCoor(sprite->Y()+1.0f); break;
                    case 0x2500: sprite->ChangeXCoor(sprite->X()-1.0f); break;
                    case '{': sprite->ChangeZCoor(sprite->Z()-static_cast<float>(optBigStepZ)); break;
                    case '}': sprite->ChangeZCoor(sprite->Z()+static_cast<float>(optBigStepZ)); break;
                    }
                } else {
                    switch (m_input.key) {
                    case '[':
                        sprite->ChangeZCoor(sprite->Z()-1.0f);
                        break;
                    case ']':
                        sprite->ChangeZCoor(sprite->Z()+1.0f);
                        break;
                    case ',':
                    case '.': {
                        int direction=sprite->RealDirection();
                        if (m_input.key==',') {
                            --direction;
                            if (direction<0)
                                direction=static_cast<int>(sprite->Vid()->m_noDirections)-1;
                        } else {
                            ++direction;
                            if (direction>=static_cast<int>(sprite->Vid()->m_noDirections))
                                direction=0;
                        }
                        sprite->ChangeRealDirection(static_cast<unsigned int>(direction));
                        break;
                    }
                    }
                }

                if (m_input.mouseWheel) {
                    float step;
                    if (spriteType != 0x20)
                        step=m_input.shift ? static_cast<float>(optBigStepZ) : 1.0f;
                    else
                        step=sprite->Vid()->m_hitVerticalOffset;
                    sprite->ChangeZCoor(sprite->Z()+static_cast<float>(m_input.mouseWheel)*step);
                }
            }
        }
    } else {
        if (m_input.shift) {
            switch (m_input.key) {
            case 0x2600:
                if (optSnap) --optShiftSnapY;
                else SetShiftCoor(FromScreenX(Graph->SizeX()/2.0f),
                                  FromScreenY(Graph->SizeY()/2.0f-1.0f),0);
                break;
            case 0x2700:
                if (optSnap) ++optShiftSnapX;
                else SetShiftCoor(FromScreenX(Graph->SizeX()/2.0f+1.0f),
                                  FromScreenY(Graph->SizeY()/2.0f),0);
                break;
            case 0x2800:
                if (optSnap) ++optShiftSnapY;
                else SetShiftCoor(FromScreenX(Graph->SizeX()/2.0f),
                                  FromScreenY(Graph->SizeY()/2.0f+1.0f),0);
                break;
            case 0x2500:
                if (optSnap) --optShiftSnapX;
                else SetShiftCoor(FromScreenX(Graph->SizeX()/2.0f-1.0f),
                                  FromScreenY(Graph->SizeY()/2.0f),0);
                break;
            case '{': insertZ-=static_cast<float>(optBigStepZ); break;
            case '}': insertZ+=static_cast<float>(optBigStepZ); break;
            }
        } else {
            switch (m_input.key) {
            case 0x2600: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0x00))); break;
            case 0x2100: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0x20))); break;
            case 0x2700: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0x40))); break;
            case 0x2200: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0x60))); break;
            case 0x2800: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0x80))); break;
            case 0x2300: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0xA0))); break;
            case 0x2500: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0xC0))); break;
            case 0x2400: Mouse->ChangeDirection(ANGLE(static_cast<uint8_t>(0xE0))); break;

            // The original has two distinct routes into delete-on and one
            // delete-off route. The 0x11xx route additionally records that the
            // mode must be cleared again when Ctrl is released.
            case 0x1100:
                if (!optDelete)
                    g_editorDeleteLatch=1;
                optDelete=1;
                break;
            case 0x2E00:
                optDelete=1;
                break;
            case 0x2D00:
                optDelete=0;
                break;

            case '[':
                insertZ-=spriteType==0x20 ? Mouse->Vid()->m_hitVerticalOffset : 1.0f;
                break;
            case ']':
                insertZ+=spriteType==0x20 ? Mouse->Vid()->m_hitVerticalOffset : 1.0f;
                break;

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                insertZ=static_cast<float>(m_input.key-'0')*10.0f;
                break;

            case ',':
            case '.': {
                int direction=Mouse->RealDirection();
                if (m_input.key==',') {
                    --direction;
                    if (direction<0)
                        direction=static_cast<int>(Mouse->Vid()->m_noDirections)-1;
                } else {
                    ++direction;
                    if (direction>=static_cast<int>(Mouse->Vid()->m_noDirections))
                        direction=0;
                }
                Mouse->ChangeRealDirection(static_cast<unsigned int>(direction));
                break;
            }
            }
        }

        if (m_input.mouseWheel) {
            const float step = spriteType != 0x20
                ? (m_input.shift ? static_cast<float>(optBigStepZ) : 1.0f)
                : Mouse->Vid()->m_hitVerticalOffset;
            insertZ+=static_cast<float>(m_input.mouseWheel)*step;
        }
    }

    if (!m_input.ctrl && g_editorDeleteLatch) {
        optDelete=0;
        g_editorDeleteLatch=0;
    }

    Sound->Tact();

    if (draw)
        DrawSecondaryInfo();

    SetToolbarState(hToolBar,kToolbarEnableButton,0x9C7D,IsLeftPossible());
    SetToolbarState(hToolBar,kToolbarEnableButton,0x9C7E,IsRightPossible());
    SetToolbarState(hToolBar,kToolbarEnableButton,0x9C8E,undo.IsUndo());
    SetToolbarState(hToolBar,kToolbarEnableButton,0xB024,undo.IsRedo());
    SetToolbarState(hToolBar,kToolbarCheckButton, 0x9C75,optDelete);

    if (optTacticMode) {
        SetToolbarState(hToolBar,kToolbarEnableButton,0x9C89,selectedSprites.No());
        SetToolbarState(hToolBar,kToolbarEnableButton,0x9C83,selectedSprites.No());
        m_groups.DrawNumber();

        GROUP* firstGroup=m_groups.First();
        if (selectedSprites.IsEqual(firstGroup)) {
            firstGroup->Draw();
            SetToolbarState(hToolBar,kToolbarCheckButton,0x9C83,1);
        } else {
            SetToolbarState(hToolBar,kToolbarCheckButton,0x9C83,0);
        }
    }

    if (draw)
        Graph->PostTact(1);
    return 0;
}
