// OptionsWidget edits a copy of Game's options. Apply and OK pass the copy to Game::SetOptions; Cancel hides the
// panel.
#pragma once
#include <map>
#include <vector>
#include "cube/widgets/BaseWidget.h"
#include "cube/Options.h"
#include "cube/ResolutionInfo.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class OptionsWidget : public BaseWidget {
public:
    OptionsWidget(Game *game, plasma::Node *node, plasma::Node *leftButton, plasma::Node *rightButton,
                  plasma::Node *button);

    // Draws the settings' names and values.
    virtual void draw();
    // Places the 30 setting buttons in rows 30 apart (left at x 240, right 15 from the right edge, y 12..432) and
    // Apply, OK and Cancel 20 above the bottom (left, centre, right).
    virtual void resized();

    // Setting handlers: Decrease is the left button and Increase the right. Both buttons toggle an on/off setting.
    void FullscreenDecrease();
    void FullscreenIncrease();
    void ResolutionDecrease();
    void ResolutionIncrease();
    void RefreshRateDecrease();
    void RefreshRateIncrease();
    void VSyncDecrease();
    void VSyncIncrease();
    void RenderDistanceDecrease(); // (-5, at least 5)
    void RenderDistanceIncrease(); // (+5, at most 100)
    void CPUUsageDecrease();       // (0)
    void CPUUsageIncrease();       // (1)
    void SoundVolumeDecrease();    // (-10, at least 0)
    void SoundVolumeIncrease();
    void MusicVolumeDecrease(); // (-10, at least 0)
    void MusicVolumeIncrease();
    void LoopMusicDecrease();
    void LoopMusicIncrease();
    void CameraSpeedDecrease(); // (-5, at least 10)
    void CameraSpeedIncrease();
    void CameraSmoothnessDecrease(); // (-5, at least 0)
    void CameraSmoothnessIncrease();
    void InvertYAxisDecrease();
    void InvertYAxisIncrease();
    void FPSLimitDecrease();
    void FPSLimitIncrease();
    void LanguageDecrease();
    void LanguageIncrease();
    void ShowRarityDecrease();
    void ShowRarityIncrease();
    void ShowFPSToggle();
    void Apply(); // (Game::SetOptions with m_options)
    // Game::SetOptions, then hides the widget.
    void Ok();
    // Hides the widget.
    void cancel();
    // Takes a copy of options and selects the resolution list entry that matches its resolution (index 0 otherwise),
    // then writes that entry's size back into the copy.
    void setOptions(const Options &options);

    std::vector<ResolutionInfo> m_resolutions; // Destroyed with the widget.

protected:
    Game *m_game;
    plasma::Node *m_modeLeftButton;
    plasma::Node *m_modeRightButton;
    plasma::Node *m_resolutionLeftButton;
    plasma::Node *m_resolutionRightButton;
    plasma::Node *m_refreshRateLeftButton;
    plasma::Node *m_refreshRateRightButton;
    plasma::Node *m_vsyncLeftButton;
    plasma::Node *m_vsyncRightButton;
    plasma::Node *m_renderDistanceLeftButton;
    plasma::Node *m_renderDistanceRightButton;
    plasma::Node *m_cpuUsageLeftButton;
    plasma::Node *m_cpuUsageRightButton;
    plasma::Node *m_soundFXVolumeLeftButton;
    plasma::Node *m_soundFXVolumeRightButton;
    plasma::Node *m_musicVolumeLeftButton;
    plasma::Node *m_musicVolumeRightButton;
    plasma::Node *m_loopMusicLeftButton;
    plasma::Node *m_loopMusicRightButton;
    plasma::Node *m_cameraSpeedLeftButton;
    plasma::Node *m_cameraSpeedRightButton;
    plasma::Node *m_cameraSmoothnessLeftButton;
    plasma::Node *m_cameraSmoothnessRightButton;
    plasma::Node *m_invertYAxisLeftButton;
    plasma::Node *m_invertYAxisRightButton;
    plasma::Node *m_fpsLimitLeftButton;
    plasma::Node *m_fpsLimitRightButton;
    plasma::Node *m_languageLeftButton;
    plasma::Node *m_languageRightButton;
    plasma::Node *m_rarityLeftButton;
    plasma::Node *m_rarityRightButton;
    plasma::Node *m_showFPSLeftButton;
    plasma::Node *m_showFPSRightButton;
    // Copies of the button template.
    plasma::Node *m_applyButton;
    plasma::Node *m_okButton;
    plasma::Node *m_cancelButton;
    // The edited copy.
    Options m_options;
    int m_resolutionIndex;
};

}
