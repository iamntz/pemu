//
// Created by cpasjuste on 28/09/18.
//

#include "c2dui.h"
#include "pgen_ui_statemenu.h"

extern "C" {
#include "shared.h"
}

bool PGENUIStateMenu::loadStateCore(const char *path) {
    unsigned char *buf = nullptr;
    bool success = getUi()->getIo()->read(path, (char **) &buf, STATE_SIZE);
    if (buf && success) {
        success = state_load(buf);
    }

    if (buf) {
        free(buf);
    }

    return success;
}

bool PGENUIStateMenu::saveStateCore(const char *path) {
    auto buf = (uint8 *) malloc(STATE_SIZE);
    bool ret = false;
    if (buf) {
        int len = state_save(buf);
        ret = getUi()->getIo()->write(path, (const char *) buf, len);
        free(buf);
    }

    return ret;
}
