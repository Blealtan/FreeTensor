#include <analyze/find_elementwise.h>
#include <analyze/find_multi_level_tiling.h>
#include <auto_schedule/rules/multi_level_tiling_with_fusion.h>
#include <auto_schedule/utils.h>

namespace freetensor {
RuleStatus MultiLevelTilingWithFusionRule::analyze(const Sketch &sketch) {
    if (sketch.nowSubSketch().hasPart(
            SketchPartType::MultiLevelTilingWithFusion) ||
        sketch.nowSubSketch().hasPart(SketchPartType::MultiLevelTiling)) {
        return RuleStatus::Skip;
    }
    if (auto toFuse = findSingleElementWiseConsumer(
            sketch.schedule().ast(), sketch.nowSubSketch().target);
        toFuse.isValid()) {
        toFuse_ = toFuse;
        return targetType_ == TargetType::CPU ? RuleStatus::Apply
                                              : RuleStatus::ApplyAndSkipRest;
    }
    return RuleStatus::Skip;
}

std::vector<Ref<Sketch>>
MultiLevelTilingWithFusionRule::genPart(const Sketch &sketch) {
    std::vector<Ref<Sketch>> ret;
    for (size_t i = 0; i < fuseLevels_.size(); i++) {
        auto newSketch = sketch.clone();
        newSketch->addPart(Ref<MultiLevelTilingWithFusionPart>::make(
            sketch.nowSubSketch().target, toFuse_, fuseLevels_[i], pat_,
            targetType_, minBlockSize_));
        newSketch->addLog("multi_level_tiling_with_fusion " +
                          std::to_string(fuseLevels_[i]));
        ret.push_back(newSketch);
    }
    return ret;
}

void MultiLevelTilingWithFusionPart::genRandAnnotation(RNG &gen) {
    int spaceLoopLength = target_.spaceLoops.size();
    int reductionLoopLength = target_.reductionLoops.size();
    std::vector<std::vector<int>> spaceLoopTiling(spaceLoopLength);
    std::vector<std::vector<int>> reductionLoopTiling(reductionLoopLength);
    while (true) {
        for (int i = 0; i < spaceLoopLength; i++) {
            spaceLoopTiling[i] = randomFillArray(target_.spaceLoops[i].length,
                                                 spaceLoopTimes_, gen);
        }
        for (int i = 0; i < reductionLoopLength; i++) {
            reductionLoopTiling[i] = randomFillArray(
                target_.reductionLoops[i].length, reductionLoopTimes_, gen);
        }
        if (targetType_ == TargetType::CPU) {
            break;
        }
        int vthread = 1;
        int thread = 1;
        int block = 1;
        for (int i = 0; i < spaceLoopLength; i++) {
            block *= spaceLoopTiling[i][spaceLoopTimes_ - 1];
            vthread *= spaceLoopTiling[i][spaceLoopTimes_ - 2];
            thread *= spaceLoopTiling[i][spaceLoopTimes_ - 3];
        }
        if (vthread != 1 && vthread <= MAX_VTHREAD && thread < 1024 &&
            block >= minBlockSize_) {
            break;
        }
    }

    annotation_.spaceLoopTiling = spaceLoopTiling;
    annotation_.reductionLoopTiling = reductionLoopTiling;
    doCacheRead_ = false;
}

MultiLevelTilingWithFusionPart::MultiLevelTilingWithFusionPart(
    ForsWithDataReuse fors, ElementWiseInfo toFuse, int level, std::string pat,
    TargetType targetType, int minBlockSize)
    : MultiLevelTilingPart(std::move(fors), std::move(pat)),
      targetType_(targetType), level_(level), toFuse_(std::move(toFuse)),
      minBlockSize_(minBlockSize) {
    frontSpaceLoopTimes_ = level_;
}

void MultiLevelTilingWithFusionPart::apply(Schedule &schedule,
                                           SubSketch &subSketch) {
    tiles_ = schedule.multiLevelTilingWithFusion(
        target_, annotation_, pat_, toFuse_, level_, targetType_, doCacheRead_);
}

bool MultiLevelTilingWithFusionPart::mutate(RNG &gen) {
    MultiLevelTilingAnnotation mut = annotation_;
    int mutPart = randomInt(1, gen);
    int spaceSize = target_.spaceLoops.size();
    int reduceSize = target_.reductionLoops.size();
    if (!spaceSize) {
        mutPart = 1;
    } else if (!reduceSize) {
        mutPart = 0;
    }
    if (mutPart == 0) {
        int mut_idx = randomInt(target_.spaceLoops.size() - 1, gen);
        mut.spaceLoopTiling[mut_idx] = randomFillArray(
            target_.spaceLoops[mut_idx].length, spaceLoopTimes_, gen);
        if (targetType_ == TargetType::GPU) {
            int vthread = 1;
            int thread = 1;
            int block = 1;
            for (size_t i = 0; i < target_.spaceLoops.size(); i++) {
                block *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 1];
                vthread *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 2];
                thread *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 3];
            }
            if (vthread == 1 || vthread > MAX_VTHREAD || thread > 1024 ||
                block < minBlockSize_) {
                return false;
            }
        }
    } else {
        int mut_idx = randomInt(target_.reductionLoops.size() - 1, gen);
        mut.reductionLoopTiling[mut_idx] = randomFillArray(
            target_.reductionLoops[mut_idx].length, reductionLoopTimes_, gen);
    }
    annotation_ = mut;
    return true;
}

bool MultiLevelTilingWithFusionPart::crossover(const SketchPart &part,
                                               RNG &gen) {
    if (part->partType() != SketchPartType::MultiLevelTilingWithFusion)
        return false;
    auto p = part.as<MultiLevelTilingWithFusionPart>();
    MultiLevelTilingAnnotation mut = annotation_;
    int mutPart = randomInt(1, gen);
    int spaceSize = target_.spaceLoops.size();
    int reduceSize = target_.reductionLoops.size();
    if (!spaceSize) {
        mutPart = 1;
    } else if (!reduceSize) {
        mutPart = 0;
    }
    if (mutPart == 0) {
        int mutIdx = randomInt(target_.spaceLoops.size() - 1, gen);
        mut.spaceLoopTiling[mutIdx] = p->annotation_.spaceLoopTiling[mutIdx];
        if (targetType_ == TargetType::GPU) {
            int vthread = 1;
            int thread = 1;
            int block = 1;
            for (size_t i = 0; i < target_.spaceLoops.size(); i++) {
                block *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 1];
                vthread *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 2];
                thread *= mut.spaceLoopTiling[i][spaceLoopTimes_ - 3];
            }
            if (vthread == 1 || vthread > MAX_VTHREAD || thread > 1024 ||
                block < minBlockSize_) {
                return false;
            }
        }
    } else {
        int mutIdx = randomInt(target_.reductionLoops.size() - 1, gen);
        mut.reductionLoopTiling[mutIdx] =
            p->annotation_.reductionLoopTiling[mutIdx];
    }
    annotation_ = mut;
    return true;
}

std::vector<int> MultiLevelTilingWithFusionPart::getAnnotation() const {
    std::vector<int> ret;
    for (auto &item : annotation_.spaceLoopTiling) {
        ret.insert(ret.end(), item.begin(), item.end());
    }
    for (auto &item : annotation_.reductionLoopTiling) {
        ret.insert(ret.end(), item.begin(), item.end());
    }
    ret.push_back(doCacheRead_);
    return ret;
}

size_t MultiLevelTilingWithFusionPart::hash() const {
    size_t h = std::hash<ForsWithDataReuse>{}(target_);
    h = hashCombine(h, std::hash<MultiLevelTilingAnnotation>{}(annotation_));
    h = hashCombine(h, std::hash<ElementWiseInfo>{}(toFuse_));
    h = hashCombine(h, std::hash<int>{}(level_));
    h = hashCombine(h, std::hash<bool>{}(doCacheRead_));
    return h;
}

} // namespace freetensor
