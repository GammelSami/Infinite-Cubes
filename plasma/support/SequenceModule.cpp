// Sequence-panel support implementations.
#include "OnlineModule.h"
#include "ShadowModule.h"
#include "SequenceModule.h"
#include "PaletteModule.h"
#include "DrawFreehandModule.h"
#include "TextModule.h"
#include "ColorModule.h"
#include "FormatModule.h"
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/Display.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/GenericWidget.h"
#include <cmath>
#include <sstream>
#include "abstr/Method.h"

SequenceModule::SequenceModule()
    : m_editWidget(0)
{}

void SequenceModule::build() {}
