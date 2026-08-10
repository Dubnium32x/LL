// written by diskodev
// game/screens/dialogue_test.c
#include "dialogue_test.h"
#include <engine/engine_core.h>

extern PlaydateAPI* pd;

#define SPEAKER_X (SCR_W / 2)
#define SPEAKER_Y (SCR_H / 2)

static i32 s_lastChoice = -1;

static void OnChoiceMade(i32 value) {
    s_lastChoice = value;
}

static const DialogueLine k_lines[] = {
    { "Lincoln",  "", "What is this place...?" },
    { "Lincoln",  "", "My house. It's all wrong. The halls are too long, the doors lead nowhere." },
    { "???",      "", "You shouldn't be here, Lincoln." },
    { "Lincoln",  "", "Who's there?! Show yourself!" },
    { "???",      "", "You already know me. You've always known me." },
    { "Lincoln",  "", "I don't understand any of this. I just want to go home." },
    { "???",      "", "Then you have a choice to make. Press { to continue, or } to turn back." },
};

static const DialogueChoice k_choices[] = {
    { "Keep going. I'm not afraid.",  0 },
    { "Turn back. This isn't real.",  1 },
};

static const GameNote k_testNote = {
    "NOTE #0",
    {
        "This is a test note.",
        "It has multiple lines of text.",
        "",
        "You can scroll through it using",
        "the crank or the D-pad.",
        "Or if youre feeling quirky, you",
        "can use the crank to scroll up",
        "and down.",
        "",
        "We need to make sure that the",
        "notes can",
        "be scrolled and that the text is",
        "readable.",
        "For example, we can test the text",
        "wrapping and line spacing.",
        "We can also test the font rendering",
        "and the overall layout of the note.",
        "",
        "This is the last line of the test",
        "note.",
    },
    21
};

void DialogueTestScreen_Init(void) {
    s_lastChoice = -1;
    DialogueBox_Show(k_lines, 7, SPEAKER_X, SPEAKER_Y);
    DialogueBox_PushChoices(k_choices, 2, OnChoiceMade);
}

void DialogueTestScreen_Update(f32 deltaTime) {
    DialogueBox_Update(deltaTime);
    Signage_Update(deltaTime);
    Note_Update(deltaTime);
    Notify_Update(deltaTime);

    // D-pad shortcuts to test each UI type (available any time)
    if (!Signage_IsActive() && !Note_IsActive() && !Notify_IsActive()) {
        if (Input_IsPressed(&inputManager, INPUT_UP))
            Signage_Show("NOTICE", "This area is dangerous. ^ ^ to jump over obstacles. Use { to interact with objects.");
        if (Input_IsPressed(&inputManager, INPUT_DOWN))
            Note_Show(&k_testNote);
        if (Input_IsPressed(&inputManager, INPUT_LEFT))
            Notify_Show("Key obtained.", 2.5f);
        if (Input_IsPressed(&inputManager, INPUT_RIGHT))
            SwitchScreen(&screenManager, SS_TEST_4);
    }

    if (Input_IsPressed(&inputManager, INPUT_B)
        && !Signage_IsActive() && !Note_IsActive() && !Notify_IsActive())
        SwitchScreen(&screenManager, SS_TITLE);
}

void DialogueTestScreen_Draw(void) {
    pd->graphics->clear(kColorWhite);

    pd->graphics->fillRect(SPEAKER_X - 8, SPEAKER_Y - 16, 16, 16, kColorBlack);

    DialogueBox_Draw();
    Signage_Draw();
    Note_Draw();
    Notify_Draw();

    if (DialogueBox_IsDone() && s_lastChoice >= 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Choice: %s",
                 s_lastChoice == 0 ? "Keep going" : "Turn back");
        DrawText(msg, 10, SCR_H - 20, 4, kColorBlack);
    }

    // legend shown when nothing is blocking
    if (!DialogueBox_IsActive() && !Signage_IsActive() && !Note_IsActive() && !Notify_IsActive()) {
        DrawIconText("^ sign  ` note  | notify  ~ physics  } back", 8, SCR_H - 14, 2, kColorBlack, 12);
    }
}

void DialogueTestScreen_Unload(void) {
    DialogueBox_Dismiss();
    Signage_Dismiss();
    Note_Dismiss();
    Notify_Dismiss();
}
