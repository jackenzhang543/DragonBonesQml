#pragma once

#include <QtCore/qglobal.h>

#if defined(LEGENDDRAGONBONES_LIBRARY)
#  define LEGENDDRAGONBONES_EXPORT Q_DECL_EXPORT
#else
#  define LEGENDDRAGONBONES_EXPORT Q_DECL_IMPORT
#endif
