#pragma once
#include "RenderPass.h"

// https://www.notion.so/2e921c4cb4638000aa7bc4303b216816 참고


class BlurPass : public RenderPass
{
public:
    BlurPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Blur"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return false;
    }

};

