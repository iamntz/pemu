//
// Created by cpasjuste on 08/12/18.
//

#ifndef C2DUI_STATUS_BOX_H
#define C2DUI_STATUS_BOX_H

namespace c2dui {

    class UiStatusBox : public SkinnedRectangle {

    public:

        explicit UiStatusBox(UiMain *main);

        ~UiStatusBox() override;

        void show(const std::string &title, const std::string &message, bool infinite = false, bool drawNow = false);

        void hide();

    private:

        virtual void onDraw(c2d::Transform &transform, bool draw = true) override;

        UiMain *main;
        SkinnedRectangle *icon;
        c2d::Text *titleText;
        c2d::Text *messageText;
        c2d::TweenAlpha *tween;
        c2d::Clock *clock;
        bool infinite = false;
    };
}

#endif //C2DUI_STATUS_BOX_H
