#include "gameplay_options.h"

#include "../controls/controller.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../savefile/savefile.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define GAMEPLAY_Y      54
#define GAMEPLAY_WIDTH  252
#define GAMEPLAY_HEIGHT 124

#define SCROLL_TICKS        PORTAL_RENDER_DEPTH_MAX - 1
#define SCROLL_INTERVALS    (SCROLL_TICKS - 1)

#define GAMEPLAY_X      ((SCREEN_WD - GAMEPLAY_WIDTH) / 2)

#define FULL_SCROLL_TIME    2.0f
#define SCROLL_MULTIPLIER   (int)(0xFFFF * FIXED_DELTA_TIME / (80 * FULL_SCROLL_TIME))
#define SCROLL_CHUNK_SIZE   (0x10000 / SCROLL_INTERVALS)

void gameplayOptionsHandleSlider(unsigned short* settingValue, float* sliderValue) {
    OSContPad* pad = controllersGetControllerData(0);

    int newValue = (int)*settingValue + pad->stick_x * SCROLL_MULTIPLIER;

    if (controllerGetButtonDown(0, A_BUTTON | R_JPAD)) {
        if (newValue >= 0xFFFF && controllerGetButtonDown(0, A_BUTTON)) {
            newValue = 0;
        } else {
            newValue = newValue + SCROLL_CHUNK_SIZE;
            newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
        }
    }

    if (controllerGetButtonDown(0, L_JPAD)) {
        newValue = newValue - 1;
        newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
    }

    if (newValue < 0) {
        newValue = 0;
    }

    if (newValue > 0xFFFF) {
        newValue = 0xFFFF;
    }

    *settingValue = newValue;
    *sliderValue = (float)newValue / 0xFFFF;
}

void gameplayOptionsInit(struct GameplayOptions* gameplayOptions) {
    gameplayOptions->selectedItem = GameplayOptionMovingPortals;

    gameplayOptions->movingPortals = menuBuildCheckbox(&gDejaVuSansFont, "Movable Portals", GAMEPLAY_X + 8, GAMEPLAY_Y + 8);
    gameplayOptions->wideScreen = menuBuildCheckbox(&gDejaVuSansFont, "Wide Screen", GAMEPLAY_X + 8, GAMEPLAY_Y + 28);

    gameplayOptions->portalRenderDepthText = menuBuildText(&gDejaVuSansFont, "Portal Render Depth", GAMEPLAY_X + 8, GAMEPLAY_Y + 48);
    gameplayOptions->portalRenderDepth = menuBuildSlider(GAMEPLAY_X + 126, GAMEPLAY_Y + 48, 126, SCROLL_TICKS);

    gameplayOptions->movingPortals.checked = (gSaveData.controls.flags & ControlSaveMoveablePortals) != 0;
    gameplayOptions->wideScreen.checked = (gSaveData.controls.flags & ControlSaveWideScreen) != 0;
    gameplayOptions->portalRenderDepth.value = (float)(gSaveData.controls.portalRenderDepth / PORTAL_RENDER_DEPTH_MAX);
    gameplayOptions->render_depth = (0xFFFF/PORTAL_RENDER_DEPTH_MAX)* gSaveData.controls.portalRenderDepth;
    gameplayOptionsHandleSlider(&gameplayOptions->render_depth, &gameplayOptions->portalRenderDepth.value);
}

enum MenuDirection gameplayOptionsUpdate(struct GameplayOptions* gameplayOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionDown) {
        ++gameplayOptions->selectedItem;

        if (gameplayOptions->selectedItem == GameplayOptionCount) {
            gameplayOptions->selectedItem = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        if (gameplayOptions->selectedItem == 0) {
            gameplayOptions->selectedItem = GameplayOptionCount - 1;
        } else {
            --gameplayOptions->selectedItem;
        }
    }

    switch (gameplayOptions->selectedItem) {
        case GameplayOptionMovingPortals:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                gameplayOptions->movingPortals.checked = !gameplayOptions->movingPortals.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (gameplayOptions->movingPortals.checked) {
                    gSaveData.controls.flags |= ControlSaveMoveablePortals;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveMoveablePortals;
                }
            }

            break;
        case GameplayOptionWideScreen:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                gameplayOptions->wideScreen.checked = !gameplayOptions->wideScreen.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (gameplayOptions->wideScreen.checked) {
                    gSaveData.controls.flags |= ControlSaveWideScreen;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveWideScreen;
                }
            }
        case GameplayOptionPortalRenderDepth:
            
            gameplayOptionsHandleSlider(&gameplayOptions->render_depth, &gameplayOptions->portalRenderDepth.value);
            gSaveData.controls.portalRenderDepth = (int)((gameplayOptions->render_depth * (1.0f/0xFFFF) * PORTAL_RENDER_DEPTH_MAX));
            break;
        break;
    }

    if (gameplayOptions->selectedItem == GameplayOptionPortalRenderDepth){
        if ((controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG))) {
            return MenuDirectionLeft;
        }
        if ((controllerGetButtonDown(0, R_TRIG))) {
            return MenuDirectionRight;
        }
    }
    else{
        if (controllerDir & ControllerDirectionLeft || controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG)) {
            return MenuDirectionLeft;
        }
        if (controllerDir & ControllerDirectionRight || controllerGetButtonDown(0, R_TRIG)) {
            return MenuDirectionRight;
        }
    }

    return MenuDirectionStay;
}

void gameplayOptionsRender(struct GameplayOptions* gameplayOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    
    gSPDisplayList(renderState->dl++, gameplayOptions->movingPortals.outline);
    renderState->dl = menuCheckboxRender(&gameplayOptions->movingPortals, renderState->dl);
    
    gSPDisplayList(renderState->dl++, gameplayOptions->wideScreen.outline);
    renderState->dl = menuCheckboxRender(&gameplayOptions->wideScreen, renderState->dl);

    gSPDisplayList(renderState->dl++, gameplayOptions->portalRenderDepth.back);
    renderState->dl = menuSliderRender(&gameplayOptions->portalRenderDepth, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, gameplayOptions->selectedItem == GameplayOptionMovingPortals, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, gameplayOptions->movingPortals.text);

    menuSetRenderColor(renderState, gameplayOptions->selectedItem == GameplayOptionWideScreen, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, gameplayOptions->wideScreen.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, gameplayOptions->selectedItem == GameplayOptionPortalRenderDepth, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, gameplayOptions->portalRenderDepthText);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}