// See cube/OptionsWidget.h.
#include "plasma/TextIds.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "cube/widgets/OptionsWidget.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include "plasma/EditFilter.h"

namespace cube {

OptionsWidget::OptionsWidget(Game *game, plasma::Node *node, plasma::Node *leftButton, plasma::Node *rightButton,
                             plasma::Node *button)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_resolutionIndex(0)
{
    // Each setting gets a left and a right button: an instance of the template under node with its own transformation
    // and display, whose "leftbutton" / "rightbutton" widget calls the handler when pressed.
    m_modeLeftButton = leftButton->instance(node);
    m_modeLeftButton->setTransformation(leftButton->transformation()->clone());
    m_modeLeftButton->setDisplay(leftButton->display()->clone());
    m_modeLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &OptionsWidget::FullscreenDecrease, true);
    m_modeRightButton = rightButton->instance(node);
    m_modeRightButton->setTransformation(rightButton->transformation()->clone());
    m_modeRightButton->setDisplay(rightButton->display()->clone());
    m_modeRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &OptionsWidget::FullscreenIncrease, true);
    m_resolutionLeftButton = leftButton->instance(node);
    m_resolutionLeftButton->setTransformation(leftButton->transformation()->clone());
    m_resolutionLeftButton->setDisplay(leftButton->display()->clone());
    m_resolutionLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                          &OptionsWidget::ResolutionDecrease, true);
    m_resolutionRightButton = rightButton->instance(node);
    m_resolutionRightButton->setTransformation(rightButton->transformation()->clone());
    m_resolutionRightButton->setDisplay(rightButton->display()->clone());
    m_resolutionRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                           &OptionsWidget::ResolutionIncrease, true);
    m_refreshRateLeftButton = leftButton->instance(node);
    m_refreshRateLeftButton->setTransformation(leftButton->transformation()->clone());
    m_refreshRateLeftButton->setDisplay(leftButton->display()->clone());
    m_refreshRateLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                           &OptionsWidget::RefreshRateDecrease, true);
    m_refreshRateRightButton = rightButton->instance(node);
    m_refreshRateRightButton->setTransformation(rightButton->transformation()->clone());
    m_refreshRateRightButton->setDisplay(rightButton->display()->clone());
    m_refreshRateRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                            &OptionsWidget::RefreshRateIncrease, true);
    m_vsyncLeftButton = leftButton->instance(node);
    m_vsyncLeftButton->setTransformation(leftButton->transformation()->clone());
    m_vsyncLeftButton->setDisplay(leftButton->display()->clone());
    m_vsyncLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &OptionsWidget::VSyncDecrease, true);
    m_vsyncRightButton = rightButton->instance(node);
    m_vsyncRightButton->setTransformation(rightButton->transformation()->clone());
    m_vsyncRightButton->setDisplay(rightButton->display()->clone());
    m_vsyncRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &OptionsWidget::VSyncIncrease, true);
    m_renderDistanceLeftButton = leftButton->instance(node);
    m_renderDistanceLeftButton->setTransformation(leftButton->transformation()->clone());
    m_renderDistanceLeftButton->setDisplay(leftButton->display()->clone());
    m_renderDistanceLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                              &OptionsWidget::RenderDistanceDecrease, true);
    m_renderDistanceRightButton = rightButton->instance(node);
    m_renderDistanceRightButton->setTransformation(rightButton->transformation()->clone());
    m_renderDistanceRightButton->setDisplay(rightButton->display()->clone());
    m_renderDistanceRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                               &OptionsWidget::RenderDistanceIncrease, true);
    m_cpuUsageLeftButton = leftButton->instance(node);
    m_cpuUsageLeftButton->setTransformation(leftButton->transformation()->clone());
    m_cpuUsageLeftButton->setDisplay(leftButton->display()->clone());
    m_cpuUsageLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                        &OptionsWidget::CPUUsageDecrease, true);
    m_cpuUsageRightButton = rightButton->instance(node);
    m_cpuUsageRightButton->setTransformation(rightButton->transformation()->clone());
    m_cpuUsageRightButton->setDisplay(rightButton->display()->clone());
    m_cpuUsageRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsWidget::CPUUsageIncrease, true);
    m_soundFXVolumeLeftButton = leftButton->instance(node);
    m_soundFXVolumeLeftButton->setTransformation(leftButton->transformation()->clone());
    m_soundFXVolumeLeftButton->setDisplay(leftButton->display()->clone());
    m_soundFXVolumeLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                             &OptionsWidget::SoundVolumeDecrease, true);
    m_soundFXVolumeRightButton = rightButton->instance(node);
    m_soundFXVolumeRightButton->setTransformation(rightButton->transformation()->clone());
    m_soundFXVolumeRightButton->setDisplay(rightButton->display()->clone());
    m_soundFXVolumeRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                              &OptionsWidget::SoundVolumeIncrease, true);
    m_musicVolumeLeftButton = leftButton->instance(node);
    m_musicVolumeLeftButton->setTransformation(leftButton->transformation()->clone());
    m_musicVolumeLeftButton->setDisplay(leftButton->display()->clone());
    m_musicVolumeLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                           &OptionsWidget::MusicVolumeDecrease, true);
    m_musicVolumeRightButton = rightButton->instance(node);
    m_musicVolumeRightButton->setTransformation(rightButton->transformation()->clone());
    m_musicVolumeRightButton->setDisplay(rightButton->display()->clone());
    m_musicVolumeRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                            &OptionsWidget::MusicVolumeIncrease, true);
    m_loopMusicLeftButton = leftButton->instance(node);
    m_loopMusicLeftButton->setTransformation(leftButton->transformation()->clone());
    m_loopMusicLeftButton->setDisplay(leftButton->display()->clone());
    m_loopMusicLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsWidget::LoopMusicDecrease, true);
    m_loopMusicRightButton = rightButton->instance(node);
    m_loopMusicRightButton->setTransformation(rightButton->transformation()->clone());
    m_loopMusicRightButton->setDisplay(rightButton->display()->clone());
    m_loopMusicRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                          &OptionsWidget::LoopMusicIncrease, true);
    m_cameraSpeedLeftButton = leftButton->instance(node);
    m_cameraSpeedLeftButton->setTransformation(leftButton->transformation()->clone());
    m_cameraSpeedLeftButton->setDisplay(leftButton->display()->clone());
    m_cameraSpeedLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                           &OptionsWidget::CameraSpeedDecrease, true);
    m_cameraSpeedRightButton = rightButton->instance(node);
    m_cameraSpeedRightButton->setTransformation(rightButton->transformation()->clone());
    m_cameraSpeedRightButton->setDisplay(rightButton->display()->clone());
    m_cameraSpeedRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                            &OptionsWidget::CameraSpeedIncrease, true);
    m_cameraSmoothnessLeftButton = leftButton->instance(node);
    m_cameraSmoothnessLeftButton->setTransformation(leftButton->transformation()->clone());
    m_cameraSmoothnessLeftButton->setDisplay(leftButton->display()->clone());
    m_cameraSmoothnessLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                                &OptionsWidget::CameraSmoothnessDecrease, true);
    m_cameraSmoothnessRightButton = rightButton->instance(node);
    m_cameraSmoothnessRightButton->setTransformation(rightButton->transformation()->clone());
    m_cameraSmoothnessRightButton->setDisplay(rightButton->display()->clone());
    m_cameraSmoothnessRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                                 &OptionsWidget::CameraSmoothnessIncrease, true);
    m_invertYAxisLeftButton = leftButton->instance(node);
    m_invertYAxisLeftButton->setTransformation(leftButton->transformation()->clone());
    m_invertYAxisLeftButton->setDisplay(leftButton->display()->clone());
    m_invertYAxisLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                           &OptionsWidget::InvertYAxisDecrease, true);
    m_invertYAxisRightButton = rightButton->instance(node);
    m_invertYAxisRightButton->setTransformation(rightButton->transformation()->clone());
    m_invertYAxisRightButton->setDisplay(rightButton->display()->clone());
    m_invertYAxisRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                            &OptionsWidget::InvertYAxisIncrease, true);
    m_fpsLimitLeftButton = leftButton->instance(node);
    m_fpsLimitLeftButton->setTransformation(leftButton->transformation()->clone());
    m_fpsLimitLeftButton->setDisplay(leftButton->display()->clone());
    m_fpsLimitLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                        &OptionsWidget::FPSLimitDecrease, true);
    m_fpsLimitRightButton = rightButton->instance(node);
    m_fpsLimitRightButton->setTransformation(rightButton->transformation()->clone());
    m_fpsLimitRightButton->setDisplay(rightButton->display()->clone());
    m_fpsLimitRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsWidget::FPSLimitIncrease, true);
    m_languageLeftButton = leftButton->instance(node);
    m_languageLeftButton->setTransformation(leftButton->transformation()->clone());
    m_languageLeftButton->setDisplay(leftButton->display()->clone());
    m_languageLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                        &OptionsWidget::LanguageDecrease, true);
    m_languageRightButton = rightButton->instance(node);
    m_languageRightButton->setTransformation(rightButton->transformation()->clone());
    m_languageRightButton->setDisplay(rightButton->display()->clone());
    m_languageRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsWidget::LanguageIncrease, true);
    m_rarityLeftButton = leftButton->instance(node);
    m_rarityLeftButton->setTransformation(leftButton->transformation()->clone());
    m_rarityLeftButton->setDisplay(leftButton->display()->clone());
    m_rarityLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &OptionsWidget::ShowRarityDecrease, true);
    m_rarityRightButton = rightButton->instance(node);
    m_rarityRightButton->setTransformation(rightButton->transformation()->clone());
    m_rarityRightButton->setDisplay(rightButton->display()->clone());
    m_rarityRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                       &OptionsWidget::ShowRarityIncrease, true);
    m_applyButton = button->clone(node);
    m_okButton = button->clone(node);
    m_cancelButton = button->clone(node);
    m_applyButton->widget()->resizeTo(110.0f, 15.0f, true);
    m_applyButton->setText(L"Apply");
    m_okButton->widget()->resizeTo(110.0f, 15.0f, true);
    m_okButton->setText(L"OK");
    m_cancelButton->widget()->resizeTo(110.0f, 15.0f, true);
    m_cancelButton->setText(L"Cancel");
    m_applyButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &OptionsWidget::Apply, true, true);
    m_okButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &OptionsWidget::Ok, true, true);
    m_cancelButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &OptionsWidget::cancel, true, true);
    setFont(L"resource1.dat");
}

void OptionsWidget::draw()
{
    // Records the centre of every child node's shape (the setting buttons) in the interface's list.
    for (std::list<plasma::Node *>::iterator it = m_node->m_children.begin(); it != m_node->m_children.end(); ++it) {
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        (*it)->getShapeBounds(minimum, maximum);
        m_game->m_interface.m_buttonCenters.push_back(plasma::Vector<2, int>((minimum + maximum) * 0.5f));
    }

    std::wstringstream stream;
    // Computed and not used.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);

    // Each row: the setting's name at x 15, then its value centred in the space right of x 240.
    if (m_options.fullscreen)
        stream << L"Fullscreen";
    else
        stream << L"Windowed";
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextFlags(0);
    renderText(L"Mode", 15.0f, 26.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 26.0f);
    stream.str(L"");

    if (!m_resolutions.empty())
        stream << m_resolutions[m_resolutionIndex].first[0] << L" x " << m_resolutions[m_resolutionIndex].first[1];
    setTextFlags(0);
    renderText(L"Resolution", 15.0f, 56.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 56.0f);
    stream.str(L"");

    stream << m_options.refreshRate << L" Hz";
    setTextFlags(0);
    renderText(L"Refresh Rate", 15.0f, 86.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 86.0f);
    stream.str(L"");

    if (m_options.vsync <= 0)
        stream << L"OFF";
    else
        stream << L"ON";
    setTextFlags(0);
    renderText(L"VSync", 15.0f, 116.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 116.0f);
    stream.str(L"");

    stream << m_options.renderDistance << L"%";
    setTextFlags(0);
    renderText(L"Render Distance", 15.0f, 146.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 146.0f);
    stream.str(L"");

    stream << (m_options.cpuPerformance ? L"FULL" : L"REDUCED");
    setTextFlags(0);
    renderText(L"CPU Usage", 15.0f, 176.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 176.0f);
    stream.str(L"");

    stream << m_options.soundVolume << L"%";
    setTextFlags(0);
    renderText(L"Sound FX Volume", 15.0f, 206.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 206.0f);
    stream.str(L"");

    stream << m_options.musicVolume << L"%";
    setTextFlags(0);
    renderText(L"Music Volume", 15.0f, 236.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 236.0f);
    stream.str(L"");

    stream << (m_options.musicLoop ? L"ON" : L"OFF");
    setTextFlags(0);
    renderText(L"Loop Music", 15.0f, 266.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 266.0f);
    stream.str(L"");

    stream << m_options.cameraSpeed << L"%";
    setTextFlags(0);
    renderText(L"Camera Speed", 15.0f, 296.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 296.0f);
    stream.str(L"");

    stream << m_options.cameraSmoothness << L"%";
    setTextFlags(0);
    renderText(L"Camera Smoothness", 15.0f, 326.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 326.0f);
    stream.str(L"");

    stream << (m_options.invertY ? "Yes" : "No");
    setTextFlags(0);
    renderText(L"Invert Y Axis", 15.0f, 356.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 356.0f);
    stream.str(L"");

    // The minimum time step in ms shown as frames per second.
    if (m_options.minTimeStep <= 0)
        stream << "none";
    else
        stream << 1000 / m_options.minTimeStep << " fps";
    setTextFlags(0);
    renderText(L"FPS Limit", 15.0f, 386.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 386.0f);
    stream.str(L"");

    stream << m_options.language;
    setTextFlags(0);
    renderText(L"Language", 15.0f, 416.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 416.0f);
    stream.str(L"");

    if (m_options.showRarity)
        stream << "as stars";
    else
        stream << "color coded";
    setTextFlags(0);
    renderText(L"Rarity", 15.0f, 446.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(stream.str(), (getContentWidth() - 250.0f) * 0.5f + 240.0f, 446.0f);

    // The setting button under the pointer loses its red.
    m_modeLeftButton->display()->m_fillColor = m_modeLeftButton->widget()->isPointerOver()
                                                   ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                   : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_modeRightButton->display()->m_fillColor = m_modeRightButton->widget()->isPointerOver()
                                                    ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                    : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_resolutionLeftButton->display()->m_fillColor = m_resolutionLeftButton->widget()->isPointerOver()
                                                         ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                         : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_resolutionRightButton->display()->m_fillColor = m_resolutionRightButton->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_refreshRateLeftButton->display()->m_fillColor = m_refreshRateLeftButton->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_refreshRateRightButton->display()->m_fillColor = m_refreshRateRightButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_vsyncLeftButton->display()->m_fillColor = m_vsyncLeftButton->widget()->isPointerOver()
                                                    ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                    : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_vsyncRightButton->display()->m_fillColor = m_vsyncRightButton->widget()->isPointerOver()
                                                     ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                     : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_renderDistanceLeftButton->display()->m_fillColor = m_renderDistanceLeftButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_renderDistanceRightButton->display()->m_fillColor = m_renderDistanceRightButton->widget()->isPointerOver()
                                                              ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                              : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cpuUsageLeftButton->display()->m_fillColor = m_cpuUsageLeftButton->widget()->isPointerOver()
                                                       ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                       : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cpuUsageRightButton->display()->m_fillColor = m_cpuUsageRightButton->widget()->isPointerOver()
                                                        ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_soundFXVolumeLeftButton->display()->m_fillColor = m_soundFXVolumeLeftButton->widget()->isPointerOver()
                                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_soundFXVolumeRightButton->display()->m_fillColor = m_soundFXVolumeRightButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_musicVolumeLeftButton->display()->m_fillColor = m_musicVolumeLeftButton->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_musicVolumeRightButton->display()->m_fillColor = m_musicVolumeRightButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_loopMusicLeftButton->display()->m_fillColor = m_loopMusicLeftButton->widget()->isPointerOver()
                                                        ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_loopMusicRightButton->display()->m_fillColor = m_loopMusicRightButton->widget()->isPointerOver()
                                                         ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                         : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cameraSpeedLeftButton->display()->m_fillColor = m_cameraSpeedLeftButton->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cameraSpeedRightButton->display()->m_fillColor = m_cameraSpeedRightButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cameraSmoothnessLeftButton->display()->m_fillColor = m_cameraSmoothnessLeftButton->widget()->isPointerOver()
                                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_cameraSmoothnessRightButton->display()->m_fillColor = m_cameraSmoothnessRightButton->widget()->isPointerOver()
                                                                ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_invertYAxisLeftButton->display()->m_fillColor = m_invertYAxisLeftButton->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_invertYAxisRightButton->display()->m_fillColor = m_invertYAxisRightButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_fpsLimitLeftButton->display()->m_fillColor = m_fpsLimitLeftButton->widget()->isPointerOver()
                                                       ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                       : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_fpsLimitRightButton->display()->m_fillColor = m_fpsLimitRightButton->widget()->isPointerOver()
                                                        ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_languageLeftButton->display()->m_fillColor = m_languageLeftButton->widget()->isPointerOver()
                                                       ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                       : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_languageRightButton->display()->m_fillColor = m_languageRightButton->widget()->isPointerOver()
                                                        ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_rarityLeftButton->display()->m_fillColor = m_rarityLeftButton->widget()->isPointerOver()
                                                     ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                     : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_rarityRightButton->display()->m_fillColor = m_rarityRightButton->widget()->isPointerOver()
                                                      ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                      : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
}

void OptionsWidget::RefreshRateDecrease()
{
    if (m_resolutions.empty())
        return;
    ResolutionInfo &resolution = m_resolutions[m_resolutionIndex % m_resolutions.size()];
    std::set<int>::iterator it = resolution.second.find(m_options.refreshRate);
    if (it != resolution.second.end() && it != resolution.second.begin()) {
        --it;
        m_options.refreshRate = *it;
    }
}

void OptionsWidget::RefreshRateIncrease()
{
    if (m_resolutions.empty())
        return;
    ResolutionInfo &resolution = m_resolutions[m_resolutionIndex % m_resolutions.size()];
    std::set<int>::iterator it = resolution.second.find(m_options.refreshRate);
    if (it != resolution.second.end() && it != --resolution.second.end()) {
        ++it;
        m_options.refreshRate = *it;
    }
}

void OptionsWidget::Apply()
{
    m_game->applyOptions(m_options);
}

void OptionsWidget::CPUUsageDecrease()
{
    m_options.cpuPerformance = 0;
}

void OptionsWidget::CPUUsageIncrease()
{
    m_options.cpuPerformance = 1;
}

void OptionsWidget::CameraSmoothnessDecrease()
{
    m_options.cameraSmoothness -= 5;
    if (m_options.cameraSmoothness < 0)
        m_options.cameraSmoothness = 0;
}

void OptionsWidget::CameraSmoothnessIncrease()
{
    m_options.cameraSmoothness += 5;
    if (m_options.cameraSmoothness > 100)
        m_options.cameraSmoothness = 100;
}

void OptionsWidget::CameraSpeedDecrease()
{
    m_options.cameraSpeed -= 5;
    if (m_options.cameraSpeed < 10)
        m_options.cameraSpeed = 10;
}

void OptionsWidget::CameraSpeedIncrease()
{
    m_options.cameraSpeed += 5;
    if (m_options.cameraSpeed > 100)
        m_options.cameraSpeed = 100;
}

void OptionsWidget::FPSLimitDecrease()
{
    m_options.minTimeStep++;
    if (m_options.minTimeStep > 50)
        m_options.minTimeStep = 50;
}

void OptionsWidget::FPSLimitIncrease()
{
    m_options.minTimeStep--;
    if (m_options.minTimeStep < 0)
        m_options.minTimeStep = 0;
}

void OptionsWidget::FullscreenDecrease()
{
    std::cout << "click " << std::endl;
    m_options.fullscreen = (m_options.fullscreen + 1) % 2;
}

void OptionsWidget::FullscreenIncrease()
{
    m_options.fullscreen = (m_options.fullscreen + 1) % 2;
}

void OptionsWidget::InvertYAxisDecrease()
{
    m_options.invertY = !m_options.invertY;
}

// The same code as InvertYAxisDecrease.
void OptionsWidget::InvertYAxisIncrease()
{
    m_options.invertY = !m_options.invertY;
}

void OptionsWidget::LanguageDecrease()
{
    m_options.language--;
}

void OptionsWidget::LanguageIncrease()
{
    m_options.language++;
}

void OptionsWidget::LoopMusicDecrease()
{
    m_options.musicLoop = !m_options.musicLoop;
}

// The same code as LoopMusicDecrease.
void OptionsWidget::LoopMusicIncrease()
{
    m_options.musicLoop = !m_options.musicLoop;
}

void OptionsWidget::MusicVolumeDecrease()
{
    m_options.musicVolume -= 10;
    if (m_options.musicVolume < 0)
        m_options.musicVolume = 0;
}

void OptionsWidget::MusicVolumeIncrease()
{
    m_options.musicVolume += 10;
    if (m_options.musicVolume > 100)
        m_options.musicVolume = 100;
}

// Hides the dialog.
void OptionsWidget::cancel()
{
    m_node->display()->m_visibility.value() = 0;
}

void OptionsWidget::Ok()
{
    m_game->applyOptions(m_options);
    m_node->display()->m_visibility.value() = 0;
}

void OptionsWidget::RenderDistanceDecrease()
{
    m_options.renderDistance -= 5;
    if (m_options.renderDistance < 5)
        m_options.renderDistance = 5;
}

void OptionsWidget::RenderDistanceIncrease()
{
    m_options.renderDistance += 5;
    if (m_options.renderDistance > 100)
        m_options.renderDistance = 100;
}

void OptionsWidget::resized()
{
    // Rows 30 apart from y 12: left buttons at x 240, right buttons 15 from the right edge.
    m_modeLeftButton->widget()->moveTo(240.0f, 12.0f, true);
    {
        plasma::Widget *button = m_modeRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 12.0f, true);
    }
    m_resolutionLeftButton->widget()->moveTo(240.0f, 42.0f, true);
    {
        plasma::Widget *button = m_resolutionRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 42.0f, true);
    }
    m_refreshRateLeftButton->widget()->moveTo(240.0f, 72.0f, true);
    {
        plasma::Widget *button = m_refreshRateRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 72.0f, true);
    }
    m_vsyncLeftButton->widget()->moveTo(240.0f, 102.0f, true);
    {
        plasma::Widget *button = m_vsyncRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 102.0f, true);
    }
    m_renderDistanceLeftButton->widget()->moveTo(240.0f, 132.0f, true);
    {
        plasma::Widget *button = m_renderDistanceRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 132.0f, true);
    }
    m_cpuUsageLeftButton->widget()->moveTo(240.0f, 162.0f, true);
    {
        plasma::Widget *button = m_cpuUsageRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 162.0f, true);
    }
    m_soundFXVolumeLeftButton->widget()->moveTo(240.0f, 192.0f, true);
    {
        plasma::Widget *button = m_soundFXVolumeRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 192.0f, true);
    }
    m_musicVolumeLeftButton->widget()->moveTo(240.0f, 222.0f, true);
    {
        plasma::Widget *button = m_musicVolumeRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 222.0f, true);
    }
    m_loopMusicLeftButton->widget()->moveTo(240.0f, 252.0f, true);
    {
        plasma::Widget *button = m_loopMusicRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 252.0f, true);
    }
    m_cameraSpeedLeftButton->widget()->moveTo(240.0f, 282.0f, true);
    {
        plasma::Widget *button = m_cameraSpeedRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 282.0f, true);
    }
    m_cameraSmoothnessLeftButton->widget()->moveTo(240.0f, 312.0f, true);
    {
        plasma::Widget *button = m_cameraSmoothnessRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 312.0f, true);
    }
    m_invertYAxisLeftButton->widget()->moveTo(240.0f, 342.0f, true);
    {
        plasma::Widget *button = m_invertYAxisRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 342.0f, true);
    }
    m_fpsLimitLeftButton->widget()->moveTo(240.0f, 372.0f, true);
    {
        plasma::Widget *button = m_fpsLimitRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 372.0f, true);
    }
    m_languageLeftButton->widget()->moveTo(240.0f, 402.0f, true);
    {
        plasma::Widget *button = m_languageRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 402.0f, true);
    }
    m_rarityLeftButton->widget()->moveTo(240.0f, 432.0f, true);
    {
        plasma::Widget *button = m_rarityRightButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 432.0f, true);
    }
    // Apply, OK and Cancel 20 above the bottom: left, centred, right.
    {
        plasma::Widget *button = m_applyButton->widget();
        button->moveTo(20.0f, getViewHeight() - button->getViewHeight() - 20.0f, true);
    }
    {
        plasma::Widget *button = m_okButton->widget();
        float width = viewWidth() - 40.0f;
        float y = getViewHeight() - button->getViewHeight() - 20.0f;
        button->moveTo((width - button->viewWidth()) * 0.5f + 20.0f, y, true);
    }
    {
        // The height is OK's, not Cancel's (the same template).
        plasma::Widget *ok = m_okButton->widget();
        plasma::Widget *button = m_cancelButton->widget();
        float right = viewWidth() - 20.0f;
        float y = getViewHeight() - ok->getViewHeight() - 20.0f;
        button->moveTo(right - button->viewWidth(), y, true);
    }
}

// Picks the refresh rate of the selected resolution closest to the current one (the first of equal distance) when the
// current one is not in its list.
void OptionsWidget::ResolutionDecrease()
{
    m_resolutionIndex--;
    if (m_resolutionIndex < 0)
        m_resolutionIndex = 0;
    if (m_resolutions.empty())
        return;
    m_options.resolutionX = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[0];
    m_options.resolutionY = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[1];
    ResolutionInfo &resolution = m_resolutions[m_resolutionIndex % m_resolutions.size()];
    if (resolution.second.find(m_options.refreshRate) == resolution.second.end()) {
        int bestDistance = -1;
        for (auto &rate : resolution.second) {
            int distance = abs(rate - m_options.refreshRate);
            if (bestDistance < 0 || distance < bestDistance) {
                bestDistance = distance;
                // Stores the distance rather than the refresh rate; ResolutionIncrease stores the rate.
                m_options.refreshRate = distance;
            }
        }
    }
}

void OptionsWidget::ResolutionIncrease()
{
    if (++m_resolutionIndex >= (int)m_resolutions.size())
        m_resolutionIndex = (int)m_resolutions.size() - 1;
    if (m_resolutions.empty())
        return;
    m_options.resolutionX = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[0];
    m_options.resolutionY = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[1];
    ResolutionInfo &resolution = m_resolutions[m_resolutionIndex % m_resolutions.size()];
    if (resolution.second.find(m_options.refreshRate) == resolution.second.end()) {
        int bestDistance = -1;
        for (int rate : resolution.second) {
            int distance = abs(rate - m_options.refreshRate);
            if (bestDistance < 0 || distance < bestDistance) {
                bestDistance = distance;
                m_options.refreshRate = rate;
            }
        }
    }
}

void OptionsWidget::ShowRarityDecrease()
{
    m_options.showRarity = !m_options.showRarity;
}

// The same code as ShowRarityDecrease.
void OptionsWidget::ShowRarityIncrease()
{
    m_options.showRarity = !m_options.showRarity;
}

void OptionsWidget::SoundVolumeDecrease()
{
    m_options.soundVolume -= 10;
    if (m_options.soundVolume < 0)
        m_options.soundVolume = 0;
}

void OptionsWidget::SoundVolumeIncrease()
{
    m_options.soundVolume += 10;
    if (m_options.soundVolume > 100)
        m_options.soundVolume = 100;
}

void OptionsWidget::VSyncDecrease()
{
    m_options.vsync = !m_options.vsync;
}

// The same code as VSyncDecrease.
void OptionsWidget::VSyncIncrease()
{
    m_options.vsync = !m_options.vsync;
}

void OptionsWidget::setOptions(const Options &options)
{
    m_options = options;
    m_resolutionIndex = 0;
    for (int i = 0; i < (int)m_resolutions.size(); i++) {
        if (m_resolutions[i].first[0] == options.resolutionX && m_resolutions[i].first[1] == options.resolutionY)
            m_resolutionIndex = i;
    }
    if (!m_resolutions.empty()) {
        m_options.resolutionX = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[0];
        m_options.resolutionY = m_resolutions[m_resolutionIndex % m_resolutions.size()].first[1];
    }
}

}
