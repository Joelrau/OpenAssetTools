#pragma once

#include "ILoadingStep.h"
#include "Zone/Zone.h"
#include "Zone/XBlock.h"
#include "StreamProcessor.h"

#include <vector>
#include <istream>
#include <memory>

class ILoadingStep;

class ZoneLoader
{
    std::vector<std::unique_ptr<ILoadingStep>> m_steps;
    std::vector<std::unique_ptr<StreamProcessor>> m_processors;

    bool m_processor_chain_dirty;

    std::unique_ptr<Zone> m_zone;

    ILoadingStream* BuildLoadingChain(ILoadingStream* rootStream);

public:
    std::vector<XBlock*> m_blocks;

    explicit ZoneLoader(std::unique_ptr<Zone> zone);

    void AddXBlock(std::unique_ptr<XBlock> block);
    void AddLoadingStep(std::unique_ptr<ILoadingStep> step);
    void AddStreamProcessor(std::unique_ptr<StreamProcessor> streamProcessor);

    void RemoveStreamProcessor(StreamProcessor* streamProcessor);

    std::unique_ptr<Zone> LoadZone(std::istream& stream);
};
