//
// Created by cpasjuste on 30/01/18.
//

#include "c2dui.h"
#include "c2dui_ui_menu.h"

#define OPTION_ID_QUIT (-1)
#define OPTION_ID_STATES (-2)
#define OPTION_ID_OTHER (-3)

class MenuLine : public c2d::RectangleShape {

public:

    MenuLine(UiMain *u, FloatRect &rect, Skin::TextGroup &tg) : RectangleShape(rect) {

        ui = u;
        textGroup = tg;
        Font *font = ui->getSkin()->font;

        name = new Text("OPTION NAME", textGroup.size, font);
        name->setFillColor(textGroup.color);
        name->setOutlineThickness(textGroup.outlineSize);
        name->setOutlineColor(textGroup.outlineColor);
        name->setOrigin(Origin::Left);
        name->setPosition(0, MenuLine::getSize().y / 2);
        name->setSizeMax((MenuLine::getSize().x * 0.6f), 0);
        MenuLine::add(name);

        value = new Text("OPTION VALUE", textGroup.size, font);
        value->setFillColor(textGroup.color);
        value->setOutlineThickness(textGroup.outlineSize);
        value->setOutlineColor(textGroup.outlineColor);
        value->setOrigin(Origin::Left);
        value->setPosition((MenuLine::getSize().x * 0.6f), MenuLine::getSize().y / 2);
        value->setSizeMax(MenuLine::getSize().x * 0.3f, 0);
        MenuLine::add(value);

        sprite = new Sprite();
        MenuLine::add(sprite);
    }

    void set(const Option &opt) {
        option = opt;

        setVisibility(Visibility::Visible);
        sprite->setVisibility(Visibility::Hidden);

        // reset
        name->setString(option.getName());
        value->setVisibility(Visibility::Visible);
        setFillColor(Color::Transparent);

        if (option.getFlags() & Option::Flags::INPUT) {
            Skin::Button *button = ui->getSkin()->getButton(option.getValueInt());
            if (button && option.getId() < Option::Id::JOY_DEADZONE) {
                if (button->texture) {
                    sprite->setTexture(button->texture, true);
                    sprite->setVisibility(Visibility::Visible);
                    value->setVisibility(Visibility::Hidden);
                    float scaling = std::min(
                            getSize().x / (float) sprite->getTextureRect().width,
                            getSize().y / (float) sprite->getTextureRect().height);
                    sprite->setScale(scaling, scaling);
                    sprite->setPosition((MenuLine::getSize().x * 0.6f), MenuLine::getSize().y / 2);
                    sprite->setOrigin(Origin::Left);
                } else {
                    sprite->setVisibility(Visibility::Hidden);
                    value->setVisibility(Visibility::Visible);
                    value->setString(button->name);
                }
            } else {
                char btn[16];
                snprintf(btn, 16, "%i", option.getValueInt());
                value->setVisibility(Visibility::Visible);
                value->setString(btn);
            }
        } else if (option.getFlags() & Option::Flags::MENU) {
            value->setVisibility(Visibility::Hidden);
            setFillColor(ui->getUiMenu()->getOutlineColor());
        } else {
            value->setVisibility(Visibility::Visible);
            value->setString(option.getValueString());
        }
    }

    UiMain *ui = nullptr;
    Text *name = nullptr;
    Text *value = nullptr;
    Sprite *sprite = nullptr;
    Skin::TextGroup textGroup;
    Option option;
};

UiMenu::UiMenu(UiMain *uiMain) : SkinnedRectangle(uiMain->getSkin(), {"OPTIONS_MENU"}) {

    ui = uiMain;
    alpha = UiMenu::getAlpha();

    // menu title
    title = new SkinnedText(uiMain->getSkin(), {"OPTIONS_MENU", "TITLE_TEXT"});
    UiMenu::add(title);

    // retrieve skin config for options items
    textGroup = ui->getSkin()->getText({"OPTIONS_MENU", "ITEMS_TEXT"});
    // calculate number of items shown
    float height = UiMenu::getSize().y - textGroup.rect.top;
    lineHeight = (float) (textGroup.rect.height);
    maxLines = (int) (height / lineHeight);
    if ((float) maxLines * lineHeight < height) {
        lineHeight = height / (float) maxLines;
    }

    // add selection rectangle (highlight)
    highlight = new RectangleShape({16, 16});
    ui->getSkin()->loadRectangleShape(highlight, {"SKIN_CONFIG", "HIGHLIGHT"});
    highlight->setSize(UiMenu::getSize().x, lineHeight);
    UiMenu::add(highlight);

    // add options items
    for (unsigned int i = 0; i < (unsigned int) maxLines; i++) {
        FloatRect rect = {textGroup.rect.left, (lineHeight * (float) i) + textGroup.rect.top,
                          UiMenu::getSize().x, lineHeight};
        auto line = new MenuLine(ui, rect, textGroup);
        lines.push_back(line);
        UiMenu::add(line);
    }

    // tween
    Vector2f targetPos = {UiMenu::getPosition().x - UiMenu::getSize().x,
                          UiMenu::getPosition().y};
    tweenPosition = new TweenPosition({UiMenu::getPosition()}, targetPos, 0.2f);
    tweenPosition->setState(TweenState::Stopped);
    UiMenu::add(tweenPosition);

    // hide by default
    UiMenu::setVisibility(Visibility::Hidden);
}

void UiMenu::load(bool isRom) {

    isRomMenu = isRom;
    isEmuRunning = ui->getUiEmu()->isVisible();
    Game game = ui->getUiRomList()->getSelection();

    if (isRomMenu) {
        ui->getConfig()->load(game);
        title->setString(game.getName().text);
    } else {
        title->setString("MAIN OPTIONS");
    }

    // set options items (remove hidden options)
    optionIndex = highlightIndex = 0;
    options.clear();
    std::vector<Option> *opts = ui->getConfig()->get(isRomMenu);
    remove_copy_if(opts->begin(), opts->end(),
                   back_inserter(options), [this](Option &opt) {
                return isOptionHidden(&opt) || opt.getFlags() & Option::Flags::HIDDEN;
            });

    options.push_back({"OTHER", {}, 0, OPTION_ID_OTHER, Option::Flags::MENU});
    if (isRomMenu) options.push_back({"STATES", {"GO"}, 0, OPTION_ID_STATES, Option::Flags::STRING});
    options.push_back({"QUIT", {"GO"}, 0, OPTION_ID_QUIT, Option::Flags::STRING});

    setAlpha(isEmuRunning ? (uint8_t) (alpha - 50) : (uint8_t) alpha);

    // update options lines/items (first option should always be a menu option)
    onKeyDown();

    // finally, show me
    if (!isVisible()) {
        setLayer(1);
        setVisibility(Visibility::Visible, true);
    }
}

void UiMenu::updateLines() {
    for (unsigned int i = 0; i < (unsigned int) maxLines; i++) {
        if (optionIndex + i >= options.size()) {
            lines[i]->setVisibility(Visibility::Hidden);
            continue;
        }
        // set line data
        const Option option = options.at(optionIndex + i);
        lines[i]->set(option);
        // set highlight position and color
        if ((int) i == highlightIndex) {
            highlight->setPosition({highlight->getPosition().x, lines[i]->getPosition().y});
            lines[i]->value->setOutlineColor(getOutlineColor());
        } else {
            lines[i]->value->setOutlineColor(textGroup.outlineColor);
        }
    }
}

void UiMenu::onKeyUp() {

    int index = optionIndex + highlightIndex;
    int middle = maxLines / 2;

    if (highlightIndex <= middle && index - middle > 0) {
        optionIndex--;
    } else {
        highlightIndex--;
    }

    if (highlightIndex < 0) {
        highlightIndex = maxLines - 1;
        highlightIndex = (int) options.size() < maxLines - 1 ? (int) options.size() - 1 : maxLines - 1;
        optionIndex = ((int) options.size() - 1) - highlightIndex;
    }

    // skip menus
    if (options.at(optionIndex + highlightIndex).getFlags() & Option::Flags::MENU) {
        return onKeyUp();
    }

    updateLines();
}

void UiMenu::onKeyDown() {

    int index = optionIndex + highlightIndex;
    int middle = maxLines / 2;

    if (highlightIndex >= middle && index + middle < (int) options.size()) {
        optionIndex++;
    } else {
        highlightIndex++;
    }

    if (highlightIndex >= maxLines || optionIndex + highlightIndex >= (int) options.size()) {
        optionIndex = 0;
        highlightIndex = 0;
    }

    // skip menus
    if (options.at(optionIndex + highlightIndex).getFlags() & Option::Flags::MENU) {
        return onKeyDown();
    }

    updateLines();
}

bool UiMenu::onInput(c2d::Input::Player *players) {

    unsigned int keys = players[0].keys;

    if (ui->getUiStateMenu()->isVisible()) {
        return C2DObject::onInput(players);
    }

    // UP
    if (keys & Input::Key::Up) {
        onKeyUp();
    }

    // DOWN
    if (keys & Input::Key::Down) {
        onKeyDown();
    }

    // LEFT /RIGHT
    if (keys & Input::Key::Left || keys & Input::Key::Right) {
        Option option = lines.at(highlightIndex)->option;
        if (option.getValues()->size() <= 1) {
            return true;
        }
        needSave = true;
        if (keys & Input::Key::Left) {
            option.prev();
        } else {
            option.next();
        }
        // update option everywhere (todo: simplify)
        options.at(optionIndex + highlightIndex).set(option);
        lines.at(highlightIndex)->set(option);
        ui->getConfig()->get(option.getId(), isRomMenu)->set(option);

        if (!option.getInfo().empty()) {
            ui->getUiStatusBox()->show(option.getInfo());
        }

        switch (option.getId()) {
            case Option::Id::GUI_SHOW_ALL:
            case Option::Id::GUI_SHOW_ZIP_NAMES:
            case Option::Id::GUI_FILTER_CLONES:
            case Option::Id::GUI_FILTER_SYSTEM:
            case Option::Id::GUI_FILTER_EDITOR:
            case Option::Id::GUI_FILTER_DEVELOPER:
            case Option::Id::GUI_FILTER_PLAYERS:
            case Option::Id::GUI_FILTER_RATING:
            case Option::Id::GUI_FILTER_ROTATION:
            case Option::Id::GUI_FILTER_RESOLUTION:
            case Option::Id::GUI_FILTER_DATE:
            case Option::Id::GUI_FILTER_GENRE:
                ui->getUiRomList()->updateRomList();
                break;

            case Option::ROM_ROTATION:
            case Option::Id::ROM_SCALING:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->updateScaling();
                }
                break;
            case Option::Id::ROM_FILTER:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->setFilter((Texture::Filter) option.getIndex());
                }
                break;
            case Option::Id::ROM_SHADER:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->setShader(option.getIndex());
                }
                break;
            case Option::Id::GUI_VIDEO_SNAP_DELAY:
                ui->getUiRomList()->setVideoSnapDelay(option.getValueInt());
                break;
#ifdef __SWITCH__
                case Option::Id::JOY_SINGLEJOYCON:
                    ((SWITCHInput *) ui->getInput())->setSingleJoyconMode(option.getValueBool());
                    break;
#endif
            default:
                break;
        }
    }

    // FIRE1 (ENTER)
    if (keys & Input::Key::Fire1) {
        Option option = lines.at(highlightIndex)->option;
        if (option.getFlags() == Option::Flags::INPUT) {
            int new_key = 0;
            int res = ui->getUiMessageBox()->show("NEW INPUT", "PRESS A BUTTON", "", "", &new_key, 9);
            if (res != MessageBox::TIMEOUT) {
                needSave = true;
                // update option everywhere (todo: simplify)
                option.setValueInt(new_key);
                options.at(optionIndex + highlightIndex).setValueInt(new_key);
                lines.at(highlightIndex)->set(option);
                ui->getConfig()->get(option.getId(), isRomMenu)->set(option);
            }
        } else if (option.getId() == OPTION_ID_QUIT) {
            if (isEmuRunning) {
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->stop();
                ui->getUiRomList()->setVisibility(Visibility::Visible);
                ui->getInput()->clear();
            } else {
                ui->done = true;
            }
        } else if (option.getId() == OPTION_ID_STATES) {
            setVisibility(Visibility::Hidden, true);
            ui->getUiStateMenu()->setVisibility(Visibility::Visible, true);
        }
    }

    // FIRE2 (BACK)
    if (keys & Input::Key::Menu1 || keys & Input::Key::Menu2 || keys & Input::Key::Fire2) {
        setVisibility(Visibility::Hidden, true);
        if (isEmuRunning) {
            ui->getUiEmu()->resume();
        }
    }

    return true;
}

void UiMenu::setVisibility(Visibility visibility, bool tweenPlay) {
    if (ui->getUiRomList() && ui->getUiRomList()->isVisible()) {
        ui->getUiRomList()->getBlur()->setVisibility(visibility, true);
    }
    if (visibility == Visibility::Hidden && needSave) {
        ui->getConfig()->save(isRomMenu ? ui->getUiRomList()->getSelection() : ss_api::Game());
        needSave = false;
    }

    RectangleShape::setVisibility(visibility, tweenPlay);
}

UiMenu::~UiMenu() {
    printf("~UIMenuNew\n");
}

