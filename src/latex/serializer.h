#ifndef AOS_LATEX_SERIALIZER_H
#define AOS_LATEX_SERIALIZER_H

#include <stddef.h>
#include <poincare/layout.h>

namespace Latex {

bool serialize(Poincare::Layout layout, char *buffer, size_t capacity);

}

#endif
