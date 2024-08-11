/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// This hack allows CommonDialog/MessageDialog to function after you remount the vs0: partition
// Technically, you only need the `str_ends_with()` part in the ifs, but there is also `strstr()`
// for this particular app (Vitohlyad) because we need to access the same paths on app0:

#include <vitasdkkern.h>
#include <taihen.h>

static int hooks_uid[2];
static tai_hook_ref_t ref_hooks[2];

static inline int str_ends_with(const char * str, const char * suffix) {
  int str_len = (int)strlen(str);
  int suffix_len = (int)strlen(suffix);

  return (str_len >= suffix_len) &&
         (0 == strcmp(str + (str_len-suffix_len), suffix));
}

static int ksceIoOpenForPid_patched(SceUID pid, const char *filename, int flag, SceIoMode mode) {
	int ret;
	if (str_ends_with(filename, "libcdlg_main.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, "vs0:sys/external/libcdlg_main.suprx", flag, mode);
	} else if (str_ends_with(filename, "libcdlg_msg.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, "vs0:sys/external/libcdlg_msg.suprx", flag, mode);
	} else if (str_ends_with(filename, "libcdlg.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, "vs0:sys/external/libcdlg.suprx", flag, mode);
	} else if (str_ends_with(filename, "common/common_resource.rco") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, "vs0:data/external/common/common_resource.rco", flag, mode);
	} else if (str_ends_with(filename, "cdlg/msg_dialog_plugin.rco") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, "vs0:data/external/cdlg/msg_dialog_plugin.rco", flag, mode);
	} else {
		ret = TAI_CONTINUE(int, ref_hooks[0], pid, filename, flag, mode);
	}
	
	return ret;
}

static int ksceIoOpen_patched(const char *filename, int flag, SceIoMode mode) {
	int ret;
	if (str_ends_with(filename, "libcdlg_main.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[1], "vs0:sys/external/libcdlg_main.suprx", flag, mode);
	} else if (str_ends_with(filename, "libcdlg_msg.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[1], "vs0:sys/external/libcdlg_msg.suprx", flag, mode);
	} else if (str_ends_with(filename, "libcdlg.suprx") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[1], "vs0:sys/external/libcdlg.suprx", flag, mode);
	} else if (str_ends_with(filename, "common/common_resource.rco") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[1], "vs0:data/external/common/common_resource.rco", flag, mode);
	} else if (str_ends_with(filename, "cdlg/msg_dialog_plugin.rco") && strstr(filename, "data/data") == NULL && strstr(filename, "data/vsh") == NULL) {
		ret = TAI_CONTINUE(int, ref_hooks[1], "vs0:data/external/cdlg/msg_dialog_plugin.rco", flag, mode);
	} else {
		ret = TAI_CONTINUE(int, ref_hooks[1], filename, flag, mode);
	}

	return ret;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	hooks_uid[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceIofilemgr", TAI_ANY_LIBRARY, 0xC3D34965, ksceIoOpenForPid_patched);
	hooks_uid[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[1], "SceIofilemgr", TAI_ANY_LIBRARY, 0x75192972, ksceIoOpen_patched);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	taiHookReleaseForKernel(hooks_uid[0], ref_hooks[0]);
	taiHookReleaseForKernel(hooks_uid[1], ref_hooks[1]);
	return SCE_KERNEL_STOP_SUCCESS;
}
