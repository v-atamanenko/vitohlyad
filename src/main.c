/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr/thread.h>
#include <psp2/power.h>
#include <psp2/vshbridge.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <taihen.h>
#include <vitaGL.h>
#include <psp2/shellutil.h>

#include "utils.h"
#include "psp2_msgbox.h"

// These two are regenerated automatically during the build process
#include "data.h"
#include "pic0.png.h"

#define $ sceClibPrintf
//#define $ log_to_file

typedef enum {
	AppState_Loading, 	      // Verify data integrity, initial state
	
	AppState_CleanInstall,    // No previous installation detected, offer to install
	AppState_CompleteInstall, // Detected valid installation, offer to uninstall
	AppState_BrokenInstall,   // Detected invalid installation, offer to reinstall or uninstall

    AppState_Quit
} AppState;

AppState g_appState = AppState_Loading;
char g_exitMessage[512] = {'\0'};

void state_loading();
void state_cleanInstall();
void state_completeInstall();
void state_brokenInstall();

int main() {
    $("====================\n"
      "Vitohlyad is booting\n"
      "====================\n");

    // Disable the PS button, shell is rather unusable anyway after remounting vs0:
    sceShellUtilInitEvents(0);
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    // Disable sleep
    int res = taiLoadStartKernelModule("ux0:/app/VUKR00001/nosleep.skprx", 0, NULL, 0);
    if (res < 0) {
        $("taiLoadStartKernelModule(nosleep) res: 0x%x\n", res);
    }

    // Hack to make cdlg work with remounted vs0:
    res = taiLoadStartKernelModule("ux0:/app/VUKR00001/cdlg-fix.skprx", 0, NULL, 0);
    if (res < 0) {
        $("taiLoadStartKernelModule(cdlg-fix) res: 0x%x\n", res);
        msgbox_ok("taiLoadStartKernelModule(cdlg-fix) error: 0x%x", res);
        return 1;
    }

    // Oooh, danger zone
    $("About to remount vs0: as RW\n");
    int vshret;
    void * buf = malloc(0x100);
    memset(buf, 0, 0x100);
    vshret = vshIoUmount(0x300, 0, 0, 0);
    $("vshIoUmount ret: %i (0x%x)\n", vshret, vshret);
    if (vshret != 0) {
        vshret = vshIoUmount(0x300, 1, 0, 0);
        $("vshIoUmount(force) ret: %i (0x%x)\n", vshret, vshret);
    }
    vshret = _vshIoMount(0x300, NULL, 2, buf);
    $("vshIoMount ret: %i (0x%x)\n", vshret, vshret);
    $("Remounted, hopefully\n");

    vglInit(0x10000);

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		$("SDL_Init error: %s\n", SDL_GetError());
        msgbox_ok("SDL_Init error: %s", SDL_GetError());
		return 1;
	}

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    window = SDL_CreateWindow("Vitohlyad",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              960, 544,
                              SDL_WINDOW_FULLSCREEN);

    if (window == NULL) {
        $("SDL_CreateWindow error: %s\n", SDL_GetError());
        msgbox_ok("SDL_CreateWindow error: %s", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        $("SDL_CreateRenderer error: %s\n", SDL_GetError());
        msgbox_ok("SDL_CreateRenderer error: %s", SDL_GetError());
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, 960, 544);

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        $("IMG_Init error: %s\n", IMG_GetError());
        msgbox_ok("IMG_Init error: %s", IMG_GetError());
        return 1;
    }

    // Load image as SDL_Surface
    SDL_RWops* tex_rwops = SDL_RWFromConstMem(pic0_png, sizeof(pic0_png));
    SDL_Texture* bg = IMG_LoadTexture_RW(renderer, tex_rwops, 0);
    if (bg == NULL) {
        $("Failed to load bg texture: %s\n", SDL_GetError());
        msgbox_ok("Failed to load bg texture: %s\n", SDL_GetError());
        return 1;
    }

    while (g_appState != AppState_Quit) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bg, NULL, NULL);

        switch (g_appState) {
            case AppState_Loading:
                state_loading();
                break;
            case AppState_CleanInstall:
                state_cleanInstall();
                break;
            case AppState_CompleteInstall:
                state_completeInstall();
                break;
            case AppState_BrokenInstall:
                state_brokenInstall();
                break;
        }

        SDL_RenderPresent(renderer);
    }

    if (strlen(g_exitMessage) > 0) {
        $("Exit message: %s\n", g_exitMessage);
        msgbox_ok("%s", g_exitMessage);
    }

    $("==========================\n"
      "Vitohlyad is shutting down\n"
      "==========================\n");

    // Always reboot because we remounted vsh0:, can't just leave the user in that state
    scePowerRequestColdReset();

	SDL_Quit();
    sceKernelDelayThread(2 * 1000 * 1000);
	return 0;
}

/*
 * Worker threads
 */

const bool skipSourceFiles = false;
bool _checksum_checking_in_progress = false;
short int _checksum_checking_result = -1; // -1 - unset, 0 - fail, 1 - partial success, 2 - success, 3 - corrupted source data
void * _checksum_check() {
    const unsigned int length = sizeof(data) / sizeof(TranslationFile);
    const unsigned int total_checks = (skipSourceFiles) ? length : length * 2;
    unsigned int matches = 0;
    unsigned int matches_sourcedata = 0;

    unsigned int progress_rounded = 0;

    $("checksum analysis started.\n");

    if (!skipSourceFiles) {
        char pathname_local[1024];

        for (unsigned int i = 0; i < length; ++i) {
            str_replace(data[i].pathname, pathname_local, "vs0:", "app0:data/");
            if (sha1sum_file_check(pathname_local, data[i].sha1sum))
                matches_sourcedata++;

            float progress = (float)i / (float)total_checks * 100;
            unsigned int floored = floorf(progress);
            if (floored > progress_rounded) {
                msgbox_progress_async_increment(floored - progress_rounded, NULL);
                progress_rounded = floored;
            }
        }
    }

    for (unsigned int i = 0; i < length; ++i) {
        if (sha1sum_file_check(data[i].pathname, data[i].sha1sum))
            matches++;
        else
            $("sha1sum mismatch for pathname %s\n", data[i].pathname);

        float progress = (float)(i + length) / (float)total_checks * 100;
        unsigned int floored = floorf(progress);
        if (floored > progress_rounded) {
            msgbox_progress_async_increment(floored - progress_rounded, NULL);
            progress_rounded = floored;
        }
    }

    msgbox_progress_async_set(100, NULL);

    $("checksum analysis complete:\n");
    if (!skipSourceFiles)
        $("%i/%i checks passed for source data\n", matches_sourcedata, length);
    $("%i/%i checks passed for system data\n", matches, length);

    if (matches_sourcedata != length && !skipSourceFiles) {
        strncpy(g_exitMessage, "Файли локалізації ушкоджено! Будь ласка, завантажте й інсталюйте цю програму знову.", sizeof(g_exitMessage)-1);
        _checksum_checking_result = 3;
        return NULL;
    }

    if (matches == 0 || matches == 1) {
        _checksum_checking_result = 0;
        return NULL;
    } else if (matches < length) {
        _checksum_checking_result = 1;
        return NULL;
    } else if (matches == length) {
        _checksum_checking_result = 2;
        return NULL;
    }

    // This should never happen.
    strncpy(g_exitMessage, "Сталося щось несподіване. Будь ласка, спробуйте ще раз.", sizeof(g_exitMessage)-1);
    _checksum_checking_result = 3;
    return NULL;
}

bool _clean_install_in_progress = false;
short int _do_install_res = -1;
void * _do_install() {
    const unsigned int length = sizeof(data) / sizeof(TranslationFile);
    const unsigned int total_operations = length * 2;
    unsigned int progress_rounded = 0;
    char srcpath[1024];
    char buffer[64 * 1024];

    $("do_install started\n");

    char backuppath[1024];
    for (unsigned int i = 0; i < length; ++i) {
        snprintf(backuppath, sizeof(backuppath) - 1, "%s.bak", data[i].pathname);
        SceUID fd_bak = sceIoOpen(backuppath, SCE_O_RDONLY, 0777);

        // First, let's do a backup of original files
        if (fd_bak > 0) {
            // backup already exists
            sceIoClose(fd_bak);
        } else {
            SceUID fd_src = sceIoOpen(data[i].pathname, SCE_O_RDONLY, 0777);
            if (fd_src < 0) {
                $("sceIoOpen failed for srcpath %s, return code 0x%x (backup)\n", srcpath, fd_src);
                strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
                _do_install_res = 1;
                return NULL;
            }

            SceUID fd_dst = sceIoOpen(backuppath, SCE_O_WRONLY|SCE_O_CREAT, 0777);
            if (fd_dst < 0) {
                $("sceIoOpen failed for dstpath %s, return code 0x%x (backup)\n", data[i].pathname, fd_dst);
                strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
                _do_install_res = 1;
                return NULL;
            }

            SceSSize bytes_read;
            do {
                bytes_read = sceIoRead(fd_src, buffer, sizeof(buffer));
                if (bytes_read > 0)
                    sceIoWrite(fd_dst, buffer, bytes_read);
            } while (bytes_read > 0);

            sceIoClose(fd_src);

            sceIoSyncByFd(fd_dst, 0);
            sceIoClose(fd_dst);

            char * og_checksum = file_sha1sum(data[i].pathname);
            if (!sha1sum_file_check(backuppath, og_checksum)) {
                free(og_checksum);
                sceIoRemove(backuppath);
                $("sha1sum mismatch while making backups for backuppath %s\n", backuppath);
                strncpy(g_exitMessage, "Сталася помилка під час збереження резервної копії оригінальних файлів.", sizeof(g_exitMessage)-1);
                _do_install_res = 1;
                return NULL;
            }
            free(og_checksum);
        }

        float progress = (float)i / (float)total_operations * 100;
        unsigned int floored = floorf(progress);
        if (floored > progress_rounded) {
            msgbox_progress_async_increment(floored - progress_rounded, "Створення резервних копій оригінальних файлів");
            progress_rounded = floored;
        }
    }


    for (unsigned int i = 0; i < length; ++i) {
        str_replace(data[i].pathname, srcpath, "vs0:", "app0:data/");

        SceUID fd_src = sceIoOpen(srcpath, SCE_O_RDONLY, 0777);
        if (fd_src < 0) {
            $("sceIoOpen failed for srcpath %s, return code 0x%x\n", srcpath, fd_src);
            strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
            _do_install_res = 1;
            return NULL;
        }

        SceUID fd_dst = sceIoOpen(data[i].pathname, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
        if (fd_dst < 0) {
            $("sceIoOpen failed for dstpath %s, return code 0x%x\n", data[i].pathname, fd_dst);
            $("Trying again after sceIoRemove:\n");

            sceIoRemove(data[i].pathname);
            fd_dst = sceIoOpen(data[i].pathname, SCE_O_WRONLY|SCE_O_CREAT, 0777);
            if (fd_dst < 0) {
                $("sceIoOpen failed AGAIN for dstpath %s, return code 0x%x\n", data[i].pathname, fd_dst);
                strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
                _do_install_res = 1;
                return NULL;
            } else {
                $("It helped, continuing.\n");
            }
        }

        SceSSize bytes_read;
        do {
            bytes_read = sceIoRead(fd_src, buffer, sizeof(buffer));
            if (bytes_read > 0)
                sceIoWrite(fd_dst, buffer, bytes_read);
        } while (bytes_read > 0);

        sceIoClose(fd_src);

        sceIoSyncByFd(fd_dst, 0);
        sceIoClose(fd_dst);

        float progress = (float)(i+length) / (float)total_operations * 100;
        unsigned int floored = floorf(progress);
        if (floored > progress_rounded) {
            msgbox_progress_async_increment(floored - progress_rounded, "Копіювання нових файлів");
            progress_rounded = floored;
        }
    }

    msgbox_progress_async_set(100, NULL);
    _do_install_res = 0;

    // Force database rebuild to propagate changes
    sceIoRemove("ur0:shell/db/app.db");

    $("do_install finished\n")

    return NULL;
}

bool _uninstall_in_progress = false;
short int _do_uninstall_res = -1;
void * _do_uninstall() {
    const unsigned int length = sizeof(data) / sizeof(TranslationFile);
    unsigned int progress_rounded = 0;
    char srcpath[1024];
    char buffer[64 * 1024];

    $("_do_uninstall started\n");

    for (unsigned int i = 0; i < length; ++i) {
        snprintf(srcpath, sizeof(srcpath) - 1, "%s.bak", data[i].pathname);

        SceUID fd_bak;
        if ((fd_bak = sceIoOpen(srcpath, SCE_O_RDONLY, 0777)) > 0) {
            // backup already exists
            sceIoClose(fd_bak);
        } else {
            $("Failed to find backup file %s\n", srcpath);
            strncpy(g_exitMessage, "Неможливо деінсталювати переклад - файл(и) резервної копії не знайдено.", sizeof(g_exitMessage)-1);
            _do_uninstall_res = 1;
            return NULL;
        }

        SceUID fd_src = sceIoOpen(srcpath, SCE_O_RDONLY, 0777);
        if (fd_src < 0) {
            $("sceIoOpen failed for srcpath %s, return code 0x%x (uninstall)\n", srcpath, fd_src);
            strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
            _do_uninstall_res = 1;
            return NULL;
        }

        SceUID fd_dst = sceIoOpen(data[i].pathname, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
        if (fd_dst < 0) {
            $("sceIoOpen failed for dstpath %s, return code 0x%x (uninstall)\n", data[i].pathname, fd_dst);
            $("Trying again after sceIoRemove:\n");

            sceIoRemove(data[i].pathname);
            fd_dst = sceIoOpen(data[i].pathname, SCE_O_WRONLY|SCE_O_CREAT, 0777);
            if (fd_dst < 0) {
                $("sceIoOpen failed AGAIN for dstpath %s, return code 0x%x (uninstall)\n", data[i].pathname, fd_dst);
                strncpy(g_exitMessage, "Сталася помилка під час відкриття файла для копіювання.", sizeof(g_exitMessage)-1);
                _do_uninstall_res = 1;
                return NULL;
            } else {
                $("It helped, continuing.\n");
            }
        }

        SceSSize bytes_read;
        do {
            bytes_read = sceIoRead(fd_src, buffer, sizeof(buffer));
            if (bytes_read > 0)
                sceIoWrite(fd_dst, buffer, bytes_read);
        } while (bytes_read > 0);

        sceIoClose(fd_src);

        sceIoSyncByFd(fd_dst, 0);
        sceIoClose(fd_dst);

        float progress = (float)i / (float)length * 100;
        unsigned int floored = floorf(progress);
        if (floored > progress_rounded) {
            msgbox_progress_async_increment(floored - progress_rounded, "Відновлення резервних копій оригінальних файлів");
            progress_rounded = floored;
        }
    }

    msgbox_progress_async_set(100, NULL);

    // Force database rebuild to propagate changes
    sceIoRemove("ur0:shell/db/app.db");

    $("do_uninstall finished\n")

    _do_uninstall_res = 0;
    return NULL;
}

/*
 * App states
 */

void state_loading() {
    if (!_checksum_checking_in_progress) {
        _checksum_checking_in_progress = true;
        _checksum_checking_result = -1;

        msgbox_progress_async("Перевірка стану системних файлів локалізації. Будь ласка, зачекайте.");

        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 128 * 1024);
        pthread_create(&t, &attr, _checksum_check, NULL);
        pthread_detach(t);
    } else if (_checksum_checking_result >= 0) {
        msgbox_progress_async_done();

        _checksum_checking_in_progress = false;
        switch (_checksum_checking_result) {
            case 0:
                // No replaced translation files detected on the system, offer
                // to install.
                g_appState = AppState_CleanInstall; break;
            case 1:
                // *Some* replaced translation files detected on the system,
                // offer to reinstall. (Possibly old or corrupted installation)
                g_appState = AppState_BrokenInstall; break;
            case 2:
                // Complete translation files detected on the system, offer to
                // uninstall.
                g_appState = AppState_CompleteInstall; break;
            default:
                // Something bad happened, exit.
                g_appState = AppState_Quit; break;
        }
    }
}

void state_cleanInstall() {
    if (!_clean_install_in_progress) {
        bool res = msgbox_yesno("Встановлення перекладу замінить English (United Kingdom) на Українську. "
                     "\n\n"
                     "Продовжити встановлення?");

        if (!res) {
            g_appState = AppState_Quit;
            return;
        }

        _clean_install_in_progress = true;
        _do_install_res = -1;
        msgbox_progress_async("Встановлення файлів локалізації. Будь ласка, зачекайте.");

        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 128 * 1024);
        pthread_create(&t, &attr, _do_install, NULL);
        pthread_detach(t);
    } else if (_do_install_res >= 0) {
        msgbox_progress_async_done();
        _clean_install_in_progress = false;

        switch (_do_install_res) {
            case 0: {
                // Install completed, offer to perform integrity check.
                bool res = msgbox_yesno("Встановлення файлів локалізації завершено! Чи ви бажаєте провести додаткову"
                             "перевірку цілісності встановлених файлів?");
                g_appState = res ? AppState_Loading : AppState_CompleteInstall;
                break;
            }
            default:
                g_appState = AppState_Quit;
                break;
        }
    }
}

void state_completeInstall() {
    if (!_uninstall_in_progress) {
        short int res = msgbox_3buttons("Видалити", "Перевірити", "Вихід", "Переклад успішно встановлено!\n\n"
                         "Щоб відновити первинний стан системи (відновити оригінальні файли локалізації English (United Kingdom)), "
                         "натисніть “Видалити”.");

        switch (res) {
            case 1:
                _uninstall_in_progress = true;
                _do_uninstall_res = -1;

                msgbox_progress_async("Відновлення оригінальних файлів локалізації. Будь ласка, зачекайте.");

                pthread_t t;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 128 * 1024);
                pthread_create(&t, &attr, _do_uninstall, NULL);
                pthread_detach(t);
                break;
            case 2:

                g_appState = AppState_Loading;
                break;
            default:
                g_appState = AppState_Quit;
                break;
        }
    } else if (_do_uninstall_res >= 0) {
        msgbox_progress_async_done();
        _uninstall_in_progress = false;

        switch (_do_uninstall_res) {
            case 0:
                // Uninstall completed
                msgbox_ok("Відновлення оригінальних файлів локалізації завершено!");
                g_appState = AppState_Quit;
                break;
            default:
                g_appState = AppState_Quit;
                break;
        }
    }
}

void state_brokenInstall() {
    if (!_uninstall_in_progress && !_clean_install_in_progress) {
        short int res = msgbox_3buttons("Видалити", "Перевстановити", "Вихід", "Встановлені файли перекладу ушкоджені або застарілі!\n\n"
                                                                           "Щоб відновити первинний стан системи (відновити оригінальні файли локалізації English (United Kingdom)), "
                                                                           "натисніть “Видалити”. Щоб перевстановити або оновити переклад, натисніть “Перевстановити”.");

        switch (res) {
            case 1: {
                _uninstall_in_progress = true;
                _do_uninstall_res = -1;
                _do_install_res = -1;

                msgbox_progress_async("Відновлення оригінальних файлів локалізації. Будь ласка, зачекайте.");

                pthread_t t;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 128 * 1024);
                pthread_create(&t, &attr, _do_uninstall, NULL);
                pthread_detach(t);
                break;
            }
            case 2: {
                _clean_install_in_progress = true;
                _do_uninstall_res = -1;
                _do_install_res = -1;
                msgbox_progress_async("Встановлення файлів локалізації. Будь ласка, зачекайте.");

                pthread_t t;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 128 * 1024);
                pthread_create(&t, &attr, _do_install, NULL);
                pthread_detach(t);
                break;
            }
            default:
                g_appState = AppState_Quit;
                break;
        }
    } else if (_do_uninstall_res >= 0) {
        msgbox_progress_async_done();
        _uninstall_in_progress = false;

        switch (_do_uninstall_res) {
            case 0:
                // Uninstall completed
                msgbox_ok("Відновлення оригінальних файлів локалізації завершено!");
                g_appState = AppState_Quit;
                break;
            default:
                g_appState = AppState_Quit;
                break;
        }
    } else if (_do_install_res >= 0) {
        msgbox_progress_async_done();
        _clean_install_in_progress = false;

        switch (_do_install_res) {
            case 0: {
                // Install completed, offer to perform integrity check.
                bool res = msgbox_yesno("Встановлення файлів локалізації завершено! Чи ви бажаєте провести додаткову"
                                        "перевірку цілісності встановлених файлів?");
                g_appState = res ? AppState_Loading : AppState_CompleteInstall;
                break;
            }
            default:
                g_appState = AppState_Quit;
                break;
        }
    }
}
