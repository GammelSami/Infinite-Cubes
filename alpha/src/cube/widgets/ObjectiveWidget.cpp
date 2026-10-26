// ObjectiveWidget holds the unused objective panel.
#include "cube/widgets/ObjectiveWidget.h"

#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"

namespace cube {

ObjectiveWidget::ObjectiveWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{}
}
