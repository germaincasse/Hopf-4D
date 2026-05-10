#pragma once

#include "editor/EditorContext.h"

namespace hopf::editor {

class Panel {
public:
    virtual ~Panel() = default;
    virtual const char* name() const = 0;
    virtual void render(EditorContext& ctx) = 0;

    bool isOpen() const     { return m_open; }
    void setOpen(bool open) { m_open = open; }

protected:
    bool m_open = true;
};

} // namespace hopf::editor
