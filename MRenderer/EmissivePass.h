#pragma once
#include "RenderPass.h"

// https://www.notion.so/2e921c4cb4638000aa7bc4303b216816 참고


class EmissivePass : public RenderPass
{
public:
    EmissivePass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "EmissivePass"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const override
    {
        
        return layer == RenderData::EmissiveItems;
    }

};


