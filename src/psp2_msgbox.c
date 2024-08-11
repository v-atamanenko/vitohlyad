/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "psp2_msgbox.h"
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/message_dialog.h>
#include <vitaGL.h>
#include <stdio.h>

static const int MAX_TEXT_LEGTH = 512;

void msgbox_ok(const char* fmt, ...) {
	va_list list;
	char string[MAX_TEXT_LEGTH];

	va_start(list, fmt);
		sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
	va_end(list);

	msgbox_ok_async("%s", string);

	while (!msgbox_ok_async_done(true)) {
		sceKernelDelayThread(1000); // 1 ms
	}
}

void msgbox_ok_async(const char* fmt, ...) {
	va_list list;
	char string[MAX_TEXT_LEGTH];

	va_start(list, fmt);
		sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
	va_end(list);

	SceMsgDialogParam param;
	sceMsgDialogParamInit(&param);
	param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;

	SceMsgDialogUserMessageParam msg_param;
	sceClibMemset(&msg_param, 0, sizeof(msg_param));
	msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
	msg_param.msg = (SceChar8 *)string;
	
	param.userMsgParam = &msg_param;

    sceMsgDialogInit(&param);
}

bool msgbox_ok_async_done(bool swap_buffers) {
	if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
		if (swap_buffers) {
            vglSwapBuffers(GL_TRUE);
		}
		return false;
	} else {
		sceMsgDialogTerm();
		return true;
	}
}

// =============================================================================

bool msgbox_yesno(const char* fmt, ...) {
    va_list list;
    char string[MAX_TEXT_LEGTH];

    va_start(list, fmt);
    sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
    va_end(list);

    msgbox_yesno_async("%s", string);

    bool result;
    while (!msgbox_yesno_async_done(true, &result)) {
        sceKernelDelayThread(1000); // 1 ms
    }

    return result;
}

void msgbox_yesno_async(const char* fmt, ...) {
    va_list list;
    char string[MAX_TEXT_LEGTH];

    va_start(list, fmt);
    sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
    va_end(list);

    SceMsgDialogParam param;
    sceMsgDialogParamInit(&param);
    param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;

    SceMsgDialogUserMessageParam msg_param;
    sceClibMemset(&msg_param, 0, sizeof(msg_param));
    msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_YESNO;
    msg_param.msg = (SceChar8 *) string;

    param.userMsgParam = &msg_param;

    sceMsgDialogInit(&param);
}

bool msgbox_yesno_async_done(bool swap_buffers, bool * result) {
    if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
        if (swap_buffers) {
            vglSwapBuffers(GL_TRUE);
        }
        return false;
    } else {
        SceMsgDialogResult res;
        sceClibMemset(&res, 0, sizeof(res));

        sceMsgDialogGetResult(&res);
        if (res.buttonId == SCE_MSG_DIALOG_BUTTON_ID_YES) {
            *result = true;
        } else {
            *result = false;
        }

        sceMsgDialogTerm();
        return true;
    }
}

// =============================================================================

short int msgbox_3buttons(const char* btn1, const char* btn2, const char * btn3, const char* fmt, ...) {
    va_list list;
    char string[MAX_TEXT_LEGTH];

    va_start(list, fmt);
    sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
    va_end(list);

    msgbox_3buttons_async(btn1, btn2, btn3, "%s", string);

    short int result;
    while (!msgbox_3buttons_async_done(true, &result)) {
        sceKernelDelayThread(1000); // 1 ms
    }

    return result;
}

void msgbox_3buttons_async(const char* btn1, const char* btn2, const char * btn3, const char* fmt, ...) {
    va_list list;
    char string[MAX_TEXT_LEGTH];

    va_start(list, fmt);
    sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
    va_end(list);

    SceMsgDialogParam param;
    sceMsgDialogParamInit(&param);
    param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;

    SceMsgDialogUserMessageParam msg_param;
    sceClibMemset(&msg_param, 0, sizeof(msg_param));
    msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_3BUTTONS;
    msg_param.msg = (SceChar8 *) string;

    SceMsgDialogButtonsParam btn_param;
    sceClibMemset(&btn_param, 0, sizeof(btn_param));
    btn_param.msg1 = (SceChar8 *) btn1;
    btn_param.fontSize1 = SCE_MSG_DIALOG_FONT_SIZE_DEFAULT;
    btn_param.msg2 = (SceChar8 *) btn2;
    btn_param.fontSize2 = SCE_MSG_DIALOG_FONT_SIZE_DEFAULT;
    btn_param.msg3 = (SceChar8 *) btn3;
    btn_param.fontSize3 = SCE_MSG_DIALOG_FONT_SIZE_DEFAULT;

    msg_param.buttonParam = &btn_param;
    param.userMsgParam = &msg_param;

    sceMsgDialogInit(&param);
}

bool msgbox_3buttons_async_done(bool swap_buffers, short int * result) {
    if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
        if (swap_buffers) {
            vglSwapBuffers(GL_TRUE);
        }
        return false;
    } else {
        SceMsgDialogResult res;
        sceClibMemset(&res, 0, sizeof(res));

        sceMsgDialogGetResult(&res);
        switch (res.buttonId) {
            case SCE_MSG_DIALOG_BUTTON_ID_BUTTON1:
                *result = 1; break;
            case SCE_MSG_DIALOG_BUTTON_ID_BUTTON2:
                *result = 2; break;
            case SCE_MSG_DIALOG_BUTTON_ID_BUTTON3:
                *result = 3; break;
            default:
                *result = 0; break;
        }

        sceMsgDialogTerm();
        return true;
    }
}

// =============================================================================

void msgbox_progress_async(const char* fmt, ...) {
    char string[MAX_TEXT_LEGTH];

    if (fmt != NULL) {
        va_list list;

        va_start(list, fmt);
        sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
        va_end(list);
    }

    SceMsgDialogParam param;
    sceMsgDialogParamInit(&param);
    param.mode = SCE_MSG_DIALOG_MODE_PROGRESS_BAR;

    SceMsgDialogProgressBarParam progressBarParam;
    sceClibMemset(&progressBarParam, 0, sizeof(progressBarParam));
    progressBarParam.barType = SCE_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;

    if (fmt != NULL) {
        progressBarParam.msg = (SceChar8 *) string;
    } else {
        progressBarParam.sysMsgParam.sysMsgType = SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT;
    }

    param.progBarParam = &progressBarParam;

    sceMsgDialogInit(&param);
}

void msgbox_progress_async_increment(unsigned int delta, const char* fmt, ...) {
    if (fmt != NULL) {
        char string[MAX_TEXT_LEGTH];
        va_list list;

        va_start(list, fmt);
        sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
        va_end(list);

        sceMsgDialogProgressBarSetMsg(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (SceChar8 *) string);
    }
    sceMsgDialogProgressBarInc(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, delta);
}

void msgbox_progress_async_set(unsigned int value, const char* fmt, ...) {
    if (fmt != NULL) {
        char string[MAX_TEXT_LEGTH];
        va_list list;

        va_start(list, fmt);
        sceClibVsnprintf(string, MAX_TEXT_LEGTH, fmt, list);
        va_end(list);

        sceMsgDialogProgressBarSetMsg(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (SceChar8 *) string);
    }
    sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, value);
}

void msgbox_progress_async_done() {
    SceCommonDialogStatus stat = sceMsgDialogGetStatus();
    if (stat == SCE_COMMON_DIALOG_STATUS_NONE) {
        return;
    }

    if (stat == SCE_COMMON_DIALOG_STATUS_RUNNING) {
        sceMsgDialogClose();

        while (stat == SCE_COMMON_DIALOG_STATUS_RUNNING) {
            vglSwapBuffers(GL_TRUE);
            stat = sceMsgDialogGetStatus();
            sceKernelDelayThread(16);
        }
    }

    if (stat == SCE_COMMON_DIALOG_STATUS_FINISHED) {
        sceMsgDialogTerm();
    }
}
