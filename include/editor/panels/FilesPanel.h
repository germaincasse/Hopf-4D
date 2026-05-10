#pragma once

#include "editor/panels/Panel.h"

#include <filesystem>
#include <string>

namespace hopf::editor {

class FilesPanel : public Panel {
public:
    FilesPanel();

    const char* name() const override { return "Files"; }
    void        render(EditorContext& ctx) override;

private:
    void renderEntry(const std::filesystem::path& path);

    std::filesystem::path m_root;
    std::string           m_rootBuf;
};

} // namespace hopf::editor
