#pragma once
class FinalAction {
public:
    FinalAction(std::function<void()> action) : action_(action) {}
    ~FinalAction() {
        action_();
    }

private:
    std::function<void()> action_;
};