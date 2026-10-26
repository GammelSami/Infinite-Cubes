// StatisticsWidget is the unused statistics panel.
#include "cube/widgets/StatisticsWidget.h"

#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"
namespace cube {

StatisticsWidget::StatisticsWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{}
// The statistics panel draws nothing in this version.
void StatisticsWidget::draw() {}
}
