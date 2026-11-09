// SynTHBloK-Poly: registers this plugin's modules.
#include "plugin.hpp"

Plugin* pluginInstance;

extern Model* modelSynTHBloKPoly;

void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(modelSynTHBloKPoly);
}
