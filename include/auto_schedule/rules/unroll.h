#ifndef FREE_TENSOR_AUTO_SCHEDULE_UNROLL_H
#define FREE_TENSOR_AUTO_SCHEDULE_UNROLL_H

#include <auto_schedule/rule.h>

namespace freetensor {

class UnrollRule : public Rule {
    TargetType targetType_;

  public:
    UnrollRule(TargetType targetType) : targetType_(targetType) {}
    RuleStatus analyze(const Sketch &sketch) override;
    std::vector<Ref<Sketch>> genPart(const Sketch &sketch) override;
};

class UnrollPart : public SketchPartNode {
    TargetType targetType_;
    int maxSize_ = 0;

  public:
    UnrollPart(TargetType targetType) : targetType_(targetType) {}
    void genRandAnnotation(RNG &gen) override;
    void genFakeAnnotation(RNG &gen) override;
    bool mutate(RNG &gen) override;
    bool crossover(const SketchPart &part, RNG &gen) override;
    void apply(Schedule &schedule, SubSketch &subSketch) override;
    SketchPartType partType() override { return SketchPartType::Unroll; }
    [[nodiscard]] std::vector<int> getAnnotation() const override {
        return {maxSize_};
    };
    [[nodiscard]] size_t hash() const override {
        return hashCombine(std::hash<std::string>{}("unroll"),
                           std::hash<int>{}(maxSize_));
    }
    [[nodiscard]] SketchPart clone() const override {
        return Ref<UnrollPart>::make(*this);
    };
};

} // namespace freetensor

#endif // FREE_TENSOR_AUTO_SCHEDULE_UNROLL_H
