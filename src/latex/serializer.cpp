#include "serializer.h"

#include <poincare/code_point_layout.h>
#include <poincare/matrix_layout.h>
#include <poincare/vertical_offset_layout.h>
#include <string.h>

namespace Latex {
namespace {

using Poincare::Layout;
using Type = Poincare::LayoutNode::Type;

class Writer {
public:
  Writer(char *buffer, size_t capacity) : m_buffer(buffer), m_remaining(capacity) {}

  bool text(const char *text) {
    const size_t length = strlen(text);
    if (length >= m_remaining) {
      return false;
    }
    memcpy(m_buffer, text, length + 1);
    m_buffer += length;
    m_remaining -= length;
    return true;
  }

  bool layout(Layout layout);

private:
  bool symbol(uint32_t value);
  bool children(Layout layout);
  bool matrix(Layout layout);

  char *m_buffer;
  size_t m_remaining;
};

bool Writer::symbol(uint32_t value) {
  switch (value) {
    case 0x03B1: return text("\\alpha ");
    case 0x03B2: return text("\\beta ");
    case 0x03B3: return text("\\gamma ");
    case 0x03B4: return text("\\delta ");
    case 0x03B5: return text("\\varepsilon ");
    case 0x03B8: return text("\\theta ");
    case 0x03BB: return text("\\lambda ");
    case 0x03BC: return text("\\mu ");
    case 0x03C0: return text("\\pi ");
    case 0x03C1: return text("\\rho ");
    case 0x03C3: return text("\\sigma ");
    case 0x03C4: return text("\\tau ");
    case 0x03C6: return text("\\varphi ");
    case 0x03C9: return text("\\omega ");
    case 0x00D7: return text("\\times ");
    case 0x00B7: return text("\\cdot ");
    case 0x00F7: return text("\\div ");
    case 0x2264: return text("\\le ");
    case 0x2265: return text("\\ge ");
    case 0x2260: return text("\\ne ");
    case 0x2212: return text("-");
    case 0x221E: return text("\\infty ");
    case 0x00B0: return text("^{\\circ}");
    case 0x2148: return text("i");
    case 0x212F: return text("e");
    case 0x1D07: return text("\\mathrm{E}");
    case '\\': return text("\\backslash ");
    case '^': return text("\\hat{}");
    case '~': return text("\\sim ");
    case '%': case '$': case '#': case '&': case '_': case '{': case '}': {
      const char escaped[] = {'\\', static_cast<char>(value), 0};
      return text(escaped);
    }
    default:
      if (value < 32 || value > 126) {
        return false;
      }
      const char plain[] = {static_cast<char>(value), 0};
      return text(plain);
  }
}

bool Writer::children(Layout group) {
  if (group.numberOfChildren() == 0) {
    return false;
  }
  constexpr const char *functions[] = {"sin", "cos", "tan", "ln", "log", "exp"};
  for (int i = 0; i < group.numberOfChildren(); ++i) {
    for (const char *name : functions) {
      const int length = strlen(name);
      if (i + length >= group.numberOfChildren() ||
          group.childAtIndex(i + length).type() != Type::LeftParenthesisLayout) {
        continue;
      }
      int j = 0;
      for (; j < length; ++j) {
        auto child = group.childAtIndex(i + j);
        if (child.type() != Type::CodePointLayout ||
            static_cast<Poincare::CodePointLayoutNode *>(child.node())->codePoint() !=
              CodePoint(name[j])) {
          break;
        }
      }
      if (j == length) {
        if (!text("\\") || !text(name) || !text(" ")) {
          return false;
        }
        i += length;
        break;
      }
    }
    if (!layout(group.childAtIndex(i))) {
      return false;
    }
  }
  return true;
}

bool Writer::matrix(Layout grid) {
  auto *node = static_cast<Poincare::MatrixLayoutNode *>(grid.node());
  const bool hasPlaceholders = Poincare::MatrixLayout(node).hasGraySquares();
  const int columns = node->numberOfColumns() - hasPlaceholders;
  const int rows = node->numberOfRows() - hasPlaceholders;
  if (columns < 1 || rows < 1 || !text("\\begin{bmatrix}")) {
    return false;
  }
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < columns; ++col) {
      if (col > 0 && !text(" & ")) {
        return false;
      }
      if (!layout(grid.childAtIndex(row * node->numberOfColumns() + col))) {
        return false;
      }
    }
    if (row + 1 < rows && !text(" \\\\ ")) {
      return false;
    }
  }
  return text("\\end{bmatrix}");
}

bool Writer::layout(Layout node) {
  const auto item = [this, &node](int i) { return layout(node.childAtIndex(i)); };
  const auto wrap = [this, &item](const char *left, const char *right) {
    return text(left) && item(0) && text(right);
  };
  switch (node.type()) {
    case Type::HorizontalLayout: return children(node);
    case Type::CodePointLayout:
      return symbol(static_cast<Poincare::CodePointLayoutNode *>(node.node())->codePoint());
    case Type::EmptyLayout: return false;
    case Type::FractionLayout: return wrap("\\frac{", "}{") && item(1) && text("}");
    case Type::BinomialCoefficientLayout: return wrap("\\binom{", "}{") && item(1) && text("}");
    case Type::NthRootLayout:
      return text("\\sqrt") &&
        (node.numberOfChildren() == 1 || (text("[") && item(1) && text("]"))) &&
        wrap("{", "}");
    case Type::VerticalOffsetLayout:
      return wrap(static_cast<Poincare::VerticalOffsetLayoutNode *>(node.node())->position() ==
        Poincare::VerticalOffsetLayoutNode::Position::Subscript ? "_{" : "^{", "}");
    case Type::LeftParenthesisLayout: return text("(");
    case Type::RightParenthesisLayout: return text(")");
    case Type::LeftSquareBracketLayout: return text("[");
    case Type::RightSquareBracketLayout: return text("]");
    case Type::BracketPairLayout: return wrap("\\left[", "\\right]");
    case Type::AbsoluteValueLayout: return wrap("\\left|", "\\right|");
    case Type::VectorNormLayout: return wrap("\\left\\lVert ", "\\right\\rVert ");
    case Type::CeilingLayout: return wrap("\\left\\lceil ", "\\right\\rceil ");
    case Type::FloorLayout: return wrap("\\left\\lfloor ", "\\right\\rfloor ");
    case Type::ConjugateLayout: return wrap("\\overline{", "}");
    case Type::VectorLayout: return wrap("\\overrightarrow{", "}");
    case Type::SumLayout: case Type::ProductLayout:
      return text(node.type() == Type::SumLayout ? "\\sum_{" : "\\prod_{") &&
        item(1) && text("=") && item(2) && text("}^{") && item(3) && text("}{") &&
        item(0) && text("}");
    case Type::IntegralLayout:
      return text("\\int_{") && item(2) && text("}^{") && item(3) && text("}{") &&
        item(0) && text("}\\,\\mathrm{d}") && item(1);
    case Type::MatrixLayout: return matrix(node);
    default: return false;
  }
}

}

bool serialize(Poincare::Layout layout, char *buffer, size_t capacity) {
  return Writer(buffer, capacity).layout(layout);
}

}
